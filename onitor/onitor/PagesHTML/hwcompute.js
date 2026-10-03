"use strict";
/* ============================================================================
   hwcompute.js — hardware-GPU compute on the Xbox GPU, via the stack that works
   ============================================================================
   WHY THIS EXISTS
   ---------------
   Measured on the console (Xbox Series X, SystemOS 26100.9438, Edge WebView2
   150.0.7871.212), 2026-10-03:

     * WebGPU on Dawn's D3D12 backend cannot execute compute. The device is
       created, the compute PSO is created, buffers are created, the command
       buffer is submitted - and the GPU process dies ~22 ms later, when the
       driver actually runs the work:
           ID3D12Device::CreateHeap failed with DXGI_ERROR_DEVICE_REMOVED
           Device removed reason: DXGI_ERROR_DRIVER_INTERNAL_ERROR (0x887A0020)
           GPU process exited unexpectedly: exit_code=34
     * Dawn's D3D11 backend yields no device at all on the console's D3D11On12
       layer (probe verdict falls through to 'fallback:swiftshader google').
     * The OpenGLES backend is compiled out of the win-UWP WebView2 runtime.

   So WebGPU compute is a dead end on this platform. But the console GPU is
   NOT unreachable: ANGLE runs WebGL on it through D3D11/D3D11On12 in hardware
   (chromium.log: "IDCompositionTexture is not supported on 11on12 devices",
   and the WebGL badge reports hardware). That stack works.

   This module therefore implements GPU compute the way it CAN be done on this
   console: as general-purpose fragment-shader passes on a hardware WebGL2
   context. It is real GPU compute on the real console GPU - it is simply not
   reached through navigator.gpu.

   WHAT IT IS / IS NOT
   -------------------
   IS  : element-wise and gather-style kernels over large f32 arrays, executed
         on the GPU, with ping-pong buffers and readback.
   IS NOT: a WGSL implementation. There is no workgroup shared memory and no
         workgroupBarrier(), because a fragment-shader pass has no equivalent.
         Kernels that need intra-workgroup communication cannot be expressed.

   Buffer layout: data is stored 4 floats per RGBA32F texel, which guarantees
   readPixels(RGBA, FLOAT) support (the spec only guarantees readback for
   RGBA32F, not R32F).
   ============================================================================ */

(function (global) {
  var VERT_SRC = [
    "#version 300 es",
    "in vec2 a_pos;",
    "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }"
  ].join("\n");

  // Prelude injected ahead of every user kernel body.
  //   u_a / u_b   : two RGBA32F input textures (u_b may be unused)
  //   u_count     : number of valid floats
  //   u_width     : texel width, so element i can be located
  // Kernel body sets `outValue` for a single element index `i`:
  //   void kernel(inout vec4 io, int i)   <-- user supplies this function
  //
  // The prelude is split into a fixed head and the two named accessors so that
  // runEx() can inject caller-supplied GLSL (extra samplers, accessors, extra
  // uniforms) between them. run() emits exactly the same program text as
  // before: FRAG_HEAD + FRAG_AB + kernelBody + FRAG_EPILOGUE.
  var FRAG_HEAD = [
    "#version 300 es",
    "precision highp float;",
    "precision highp int;",
    "precision highp sampler2D;",
    "uniform int u_count;",
    "uniform int u_width;",
    "out vec4 fragColor;",
    ""
  ].join("\n") + "\n";

  var FRAG_AB = [
    "uniform sampler2D u_a;",
    "uniform sampler2D u_b;",
    "",
    "float elemA(int i) {",
    "  ivec2 t = ivec2(i >> 2, 0);",
    "  t.x = t.x % u_width;",
    "  t.y = (i >> 2) / u_width;",
    "  vec4 v = texelFetch(u_a, t, 0);",
    "  int c = i & 3;",
    "  return c == 0 ? v.x : (c == 1 ? v.y : (c == 2 ? v.z : v.w));",
    "}",
    "float elemB(int i) {",
    "  ivec2 t = ivec2(i >> 2, 0);",
    "  t.x = t.x % u_width;",
    "  t.y = (i >> 2) / u_width;",
    "  vec4 v = texelFetch(u_b, t, 0);",
    "  int c = i & 3;",
    "  return c == 0 ? v.x : (c == 1 ? v.y : (c == 2 ? v.z : v.w));",
    "}",
    ""
  ].join("\n");

  // The classic two-input prelude, byte-for-byte what run() has always used.
  var FRAG_PRELUDE = FRAG_HEAD + FRAG_AB;

  var FRAG_EPILOGUE = [
    "",
    "void main() {",
    "  ivec2 p = ivec2(gl_FragCoord.xy);",
    "  int base = ((p.y * u_width) + p.x) << 2;",
    "  vec4 o = vec4(0.0);",
    "  for (int k = 0; k < 4; k++) {",
    "    int i = base + k;",
    "    if (i < u_count) {",
    "      vec4 io = vec4(0.0);",
    "      kernel(io, i);",
    "      o[k] = io.x;",
    "    }",
    "  }",
    "  fragColor = o;",
    "}"
  ].join("\n");

  function compile(gl, type, src) {
    var s = gl.createShader(type);
    gl.shaderSource(s, src);
    gl.compileShader(s);
    if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) {
      var log = gl.getShaderInfoLog(s);
      gl.deleteShader(s);
      throw new Error("shader compile failed: " + log);
    }
    return s;
  }

  // fragSrc is the COMPLETE fragment shader source (head + kernel + epilogue).
  function buildProgram(gl, fragSrc) {
    var vs = compile(gl, gl.VERTEX_SHADER, VERT_SRC);
    var fs = compile(gl, gl.FRAGMENT_SHADER, fragSrc);
    var p = gl.createProgram();
    gl.attachShader(p, vs);
    gl.attachShader(p, fs);
    gl.bindAttribLocation(p, 0, "a_pos");
    gl.linkProgram(p);
    gl.deleteShader(vs);
    gl.deleteShader(fs);
    if (!gl.getProgramParameter(p, gl.LINK_STATUS)) {
      var log = gl.getProgramInfoLog(p);
      gl.deleteProgram(p);
      throw new Error("program link failed: " + log);
    }
    return p;
  }

  function HWCompute() {
    this.canvas = document.createElement("canvas");
    this.canvas.width = 16;
    this.canvas.height = 16;
    var opts = {
      antialias: false,
      depth: false,
      stencil: false,
      alpha: false,
      preserveDrawingBuffer: false,
      powerPreference: "high-performance",
      failIfMajorPerformanceCaveat: false
    };
    var gl = this.canvas.getContext("webgl2", opts);
    if (!gl) throw new Error("WebGL2 unavailable");
    this.gl = gl;

    this.floatRenderable =
      !!(gl.getExtension("EXT_color_buffer_float") ||
         gl.getExtension("WEBGL_color_buffer_float"));
    gl.getExtension("OES_texture_float_linear");

    var dbg = gl.getExtension("WEBGL_debug_renderer_info");
    this.renderer = dbg ? String(gl.getParameter(dbg.UNMASKED_RENDERER_WEBGL) || "")
                        : String(gl.getParameter(gl.RENDERER) || "");
    this.vendor = dbg ? String(gl.getParameter(dbg.UNMASKED_VENDOR_WEBGL) || "")
                      : String(gl.getParameter(gl.VENDOR) || "");
    this.maxTexture = gl.getParameter(gl.MAX_TEXTURE_SIZE);
    this.isHardware = !/swiftshader|software|llvmpipe|basic render|microsoft basic/i
      .test(this.renderer + " " + this.vendor);

    // Fullscreen triangle strip
    this.quad = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, this.quad);
    gl.bufferData(gl.ARRAY_BUFFER,
      new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), gl.STATIC_DRAW);

    this._vbo = gl.createVertexArray();
    gl.bindVertexArray(this._vbo);
    gl.bindBuffer(gl.ARRAY_BUFFER, this.quad);
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 0, 0);
    gl.bindVertexArray(null);
  }

  HWCompute.prototype._tex = function (data, count, width, height) {
    var gl = this.gl;
    // 4 floats per texel -> ceil(count/4) texels -> 2D grid
    var texels = Math.max(1, Math.ceil(count / 4));
    if (!width) width = Math.min(texels, Math.min(2048, this.maxTexture));
    width = Math.max(1, width);
    if (!height) height = Math.ceil(texels / width);
    // A caller-supplied grid must still be big enough for the data.
    if (height * width < texels) height = Math.ceil(texels / width);
    if (height > this.maxTexture) throw new Error("input too large for one texture");

    var padded = new Float32Array(width * height * 4);
    if (data) padded.set(data.subarray(0, Math.min(count, data.length)));

    var t = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, t);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA32F, width, height, 0,
      gl.RGBA, gl.FLOAT, padded);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    return { tex: t, width: width, height: height };
  };

  /**
   * Generalized entry point (used by wgpu-shim.js; run() is now a thin wrapper
   * around it, so there is exactly one execution path).
   *
   * spec = {
   *   count     : number of floats to evaluate (becomes u_count, and the
   *               element index `i` passed to kernel() always stays < count)
   *   gridCount : optional. Size of the shared texel grid in floats. Use when
   *               an input texture holds MORE data than the elements being
   *               evaluated (the WebGPU shim sizes the grid from the largest
   *               bound buffer). Defaults to count.
   *   inputs    : [{ name: "u_a", data: Float32Array }, ...]  sampler uniforms
   *               the caller's GLSL declares; values are uploaded as raw f32
   *               bit patterns, so u32/i32 data round-trips bit-exactly.
   *   uniforms  : { u_name: intValue, ... }  extra int uniforms to set.
   *   glsl      : GLSL defining `void kernel(inout vec4 io, int i)`, emitted
   *               after the fixed head (which declares u_count/u_width).
   *   cacheKey  : optional string. When given, the compiled program is cached
   *               on the instance and reused (shader compilation is the single
   *               most expensive part of a small dispatch).
   * }
   *
   * Returns a Float32Array of length count holding the raw f32 bit pattern of
   * io.x for each element.
   */
  HWCompute.prototype.runEx = function (spec) {
    var gl = this.gl;
    if (!this.floatRenderable) {
      throw new Error("EXT_color_buffer_float unavailable - cannot render to float");
    }
    var n = spec.count >>> 0;
    var inputs = spec.inputs || [];
    if (n === 0) return new Float32Array(0);
    if (inputs.length > 8) throw new Error("runEx: more than 8 input textures");

    // The texel grid is shared by every input so one accessor can locate
    // element j in any of them; it is sized by the largest buffer bound.
    var gridCount = Math.max(n, spec.gridCount >>> 0);
    var texels = Math.max(1, Math.ceil(gridCount / 4));
    var width = Math.max(1, Math.min(texels, Math.min(2048, this.maxTexture)));
    var height = Math.ceil(texels / width);
    if (height > this.maxTexture) throw new Error("input too large for one texture");

    var tex = [], created = [];
    for (var k = 0; k < inputs.length; k++) {
      // `data: null` reuses the previous input's texture (cheap aliasing).
      if (!inputs[k].data && k > 0) {
        tex.push(tex[k - 1]);
        created.push(false);
        continue;
      }
      tex.push(this._tex(inputs[k].data || null, gridCount, width, height));
      created.push(true);
    }

    var target = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, target);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA32F, width, height, 0,
      gl.RGBA, gl.FLOAT, null);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);

    var fbo = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0,
      gl.TEXTURE_2D, target, 0);
    var st = gl.checkFramebufferStatus(gl.FRAMEBUFFER);
    if (st !== gl.FRAMEBUFFER_COMPLETE) {
      throw new Error("incomplete float framebuffer: 0x" + st.toString(16));
    }

    var prog = null;
    if (spec.cacheKey) {
      if (!this._progCache) this._progCache = {};
      prog = this._progCache[spec.cacheKey];
    }
    if (!prog) {
      prog = buildProgram(gl, FRAG_HEAD + spec.glsl + FRAG_EPILOGUE);
      if (spec.cacheKey) this._progCache[spec.cacheKey] = prog;
    }
    gl.useProgram(prog);
    gl.bindVertexArray(this._vbo);

    for (var u = 0; u < inputs.length; u++) {
      gl.activeTexture(gl.TEXTURE0 + u);
      gl.bindTexture(gl.TEXTURE_2D, tex[u].tex);
      gl.uniform1i(gl.getUniformLocation(prog, inputs[u].name), u);
    }
    gl.uniform1i(gl.getUniformLocation(prog, "u_count"), n);
    gl.uniform1i(gl.getUniformLocation(prog, "u_width"), width);
    var extra = spec.uniforms || {};
    for (var un in extra) {
      if (Object.prototype.hasOwnProperty.call(extra, un)) {
        gl.uniform1i(gl.getUniformLocation(prog, un), extra[un] | 0);
      }
    }

    // Only the rows that can hold elements < n need reading back.
    var rows = Math.min(height, Math.max(1, Math.ceil(Math.ceil(n / 4) / width)));
    gl.viewport(0, 0, width, height);
    gl.disable(gl.DEPTH_TEST);
    gl.disable(gl.BLEND);
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

    var buf = new Float32Array(width * rows * 4);
    gl.readPixels(0, 0, width, rows, gl.RGBA, gl.FLOAT, buf);

    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.deleteFramebuffer(fbo);
    gl.deleteTexture(target);
    for (var d = 0; d < tex.length; d++) {
      if (created[d]) gl.deleteTexture(tex[d].tex);
    }
    if (!spec.cacheKey) gl.deleteProgram(prog);
    gl.bindVertexArray(null);

    return buf.subarray(0, n);
  };

  /** Drop every program cached by runEx({cacheKey:...}). */
  HWCompute.prototype.clearProgramCache = function () {
    var gl = this.gl;
    if (!this._progCache) return;
    for (var k in this._progCache) {
      if (Object.prototype.hasOwnProperty.call(this._progCache, k)) {
        gl.deleteProgram(this._progCache[k]);
      }
    }
    this._progCache = {};
  };

  /**
   * Run a compute kernel over `count` floats.
   * kernelBody is GLSL defining:
   *     void kernel(inout vec4 io, int i) { io.x = ...; }
   * using elemA(i) / elemB(i) to read inputs (and arrayLength-style bounds
   * checks are unnecessary - i is always < u_count).
   *
   * Returns a Float32Array of length count.
   */
  HWCompute.prototype.run = function (kernelBody, inputA, count, inputB) {
    return this.runEx({
      count: count >>> 0,
      inputs: [
        { name: "u_a", data: inputA },
        { name: "u_b", data: inputB || null }
      ],
      glsl: FRAG_AB + kernelBody
    });
  };

  /* ------------------------------------------------------------------ *
   * Kernel library. Each entry is a GLSL `kernel` body.
   * ------------------------------------------------------------------ */
  var KERNELS = {
    // out[i] = a[i] * 2 + 1   (the ComputeProbe / webgpucheck shape)
    scale2p1:
      "void kernel(inout vec4 io, int i) { io.x = elemA(i) * 2.0 + 1.0; }",

    // out[i] = a[i] * b[i] + a[i]
    axpby:
      "void kernel(inout vec4 io, int i) { float a = elemA(i); io.x = a * elemB(i) + a; }",

    // out[i] = sin(a[i]) * sqrt(abs(a[i])) - a cheap transcendental load
    transcendentals:
      "void kernel(inout vec4 io, int i) { float a = elemA(i);" +
      " io.x = sin(a) * sqrt(abs(a)) + cos(a * 0.5); }",

    // 9-point stencil over a flat array (gather-style, non element-wise)
    stencil9:
      "void kernel(inout vec4 io, int i) {" +
      " float s = 0.0;" +
      " for (int d = -4; d <= 4; d++) {" +
      "   int j = clamp(i + d, 0, u_count - 1);" +
      "   s += elemA(j);" +
      " }" +
      " io.x = s / 9.0; }"
  };

  global.HWCompute = HWCompute;
  global.HWCompute.KERNELS = KERNELS;

  /* Self-test: proves the engine computes correctly, and reports which GPU
     the work actually ran on. Used by HwCompute.html on the console. */
  global.HWCompute.selfTest = function (elements) {
    var n = elements || 262144;
    var out = { ok: false };
    try {
      var hw = new HWCompute();
      out.renderer = hw.renderer;
      out.vendor = hw.vendor;
      out.isHardware = hw.isHardware;
      out.floatRenderable = hw.floatRenderable;
      out.glVersion = hw.gl.getParameter(hw.gl.VERSION);

      // --- correctness: out[i] = a[i]*2 + 1
      var a = new Float32Array(n);
      for (var i = 0; i < n; i++) a[i] = (i % 977) * 0.5;
      var t0 = performance.now();
      var r = hw.run(KERNELS.scale2p1, a, n);
      var t1 = performance.now();
      var bad = -1;
      for (var k = 0; k < n; k++) {
        if (Math.abs(r[k] - (a[k] * 2 + 1)) > 1e-3) { bad = k; break; }
      }
      out.count = n;
      out.firstBad = bad;
      out.correct = bad < 0;
      out.msScale2p1 = Math.round((t1 - t0) * 100) / 100;

      // --- throughput: heavier transcendental kernel, several passes
      if (out.correct) {
        var passes = 20;
        var t2 = performance.now();
        for (var p = 0; p < passes; p++) hw.run(KERNELS.transcendentals, a, n);
        var t3 = performance.now();
        var ms = t3 - t2;
        out.msTranscendentals = Math.round(ms * 100) / 100;
        // Under a virtual-time budget (headless testing) performance.now() does
        // not advance during synchronous GPU work and reports 0 ms, which would
        // make this Infinity. Report null instead of a bogus number.
        out.mflops = ms > 0
          ? Math.round((n * passes * 6) / (ms / 1000) / 1e6)
          : null;
      }
      out.ok = out.correct;
    } catch (e) {
      out.error = String((e && e.message) || e);
    }
    return out;
  };
})(typeof window !== "undefined" ? window : this);
