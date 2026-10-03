"use strict";
/* ============================================================================
   ComputeProbe.js — WebGPU compute fault-boundary isolation (v1.6.2.0)
   ============================================================================
   Runs automatically on the Home page AFTER the app's own startup probe
   (window.__onitorWgpuProbe) has settled on a hardware ('gpu:...') verdict,
   so the adaptive ladder is never corrupted by a compute-induced GPU-process
   crash (the app poller is finished by then; a 'gpu' verdict never restarts).

   Stage ladder (fresh adapter + device per stage, breadcrumbs at every API
   boundary, halt on first fault):
     none        adapter + device + lost-hook only
     psocreate   + shader module, layouts, SYNC createComputePipeline
     upload      + buffers + queue.writeBuffer + EMPTY submit
                 (DynamicUploader staging: memcpy now, CopyBufferRegion
                 deferred to the first submit)
     macupload   + mappedAtCreation storage buffer, write, unmap, EMPTY
                 submit (Dawn's mapped-at-creation deferred staging path —
                 this is the upload webgpucheck.com actually uses)
     dispatch    + bind group + dispatchWorkgroups + submit (no readback)
     copy        + copyBufferToBuffer in the SAME encoder + submit
     signal      + queue.onSubmittedWorkDone (fence, no map)
     full        + mapAsync + read + verify (boring u32 shader)
     webgpucheck EXACT replica of webgpucheck.com's compute test: f32
                 read_write storage, workgroup_size(1), dispatchWorkgroups(4),
                 mappedAtCreation upload, copy, mapAsync, verify x2

   Breadcrumbs go to:
     1. console.log  ("PROBE ..." -> chromium.log CONSOLE lines)
     2. the DOM trail (renderer survives GPU-process death)
     3. ntfy.sh beacon (instant remote readout for debugging sessions)

   Remote steering: fetches probe-config.json from the repo (cache-busted);
   on fetch failure the built-in defaults below run. Set "enabled": false in
   the repo copy to disable the probe without a rebuild.
   ============================================================================ */

(function () {
  if (window.__onitorComputeProbeStarted) return;
  window.__onitorComputeProbeStarted = true;
  try { if (sessionStorage.getItem("computeProbeDone") === "1") return; } catch (e) { /* private mode etc. */ }

  /* ---------- built-in defaults (remote config overrides) ---------------- */
  var DEFAULT_CONFIG = {
    enabled: true,
    version: 1,
    stages: ["none", "psocreate", "upload", "macupload", "dispatch", "copy", "signal", "full", "webgpucheck"],
    haltOnFault: true,
    settleMs: 800,
    beaconTopic: "onitor-probe-k3v9xqz7",
    note: "defaults (remote config unreachable)"
  };
  var CONFIG_URL = "https://raw.githubusercontent.com/http-hadi/xbox-win32-runner/onitor-webview2-build/onitor/onitor/PagesHTML/probe-config.json";

  /* ---------- boring shader (infrastructure faults first) ----------------- */
  var PROBE_WGSL = [
    "@group(0) @binding(0) var<storage, read> inputBuf : array<u32>;",
    "@group(0) @binding(1) var<storage, read_write> outputBuf : array<u32>;",
    "",
    "@compute @workgroup_size(64)",
    "fn main(@builtin(global_invocation_id) gid : vec3<u32>) {",
    "  let i = gid.x;",
    "  if (i >= arrayLength(&inputBuf)) { return; }",
    "  outputBuf[i] = inputBuf[i] * 2u + 1u;",
    "}"
  ].join("\n");
  var ELEMENTS = 256;    // 4 workgroups x 64 invocations
  var WORKGROUPS = 4;

  /* ---------- EXACT webgpucheck.com compute test ------------------------- */
  var WGC_WGSL = [
    "",
    "    @group(0) @binding(0) var<storage, read_write> numbers: array<f32>;",
    "    ",
    "    @compute @workgroup_size(1)",
    "    fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {",
    "      let index = global_id.x;",
    "      numbers[index] = numbers[index] * 2.0;",
    "    }",
    "  "
  ].join("\n");
  var WGC_INPUT = [12.5, 45, 100.5, 0.25];

  /* ---------- ui ---------------------------------------------------------- */
  var panel = document.createElement("div");
  panel.id = "onitorComputeProbe";
  panel.style.cssText = "position:fixed;left:8px;bottom:8px;z-index:2147483000;" +
    "max-width:560px;max-height:42vh;overflow:auto;background:rgba(16,20,24,.93);" +
    "color:#cfd8dc;font:11px/1.45 Consolas,'Courier New',monospace;" +
    "border:1px solid #37474f;border-radius:6px;padding:8px 10px;display:none;" +
    "white-space:pre-wrap;word-break:break-word;text-align:left;";
  var head = document.createElement("div");
  head.style.cssText = "font-weight:bold;color:#7ec8ff;margin-bottom:4px;";
  head.textContent = "ComputeProbe v" + DEFAULT_CONFIG.version;
  var banner = document.createElement("div");
  banner.style.cssText = "font-weight:bold;margin-bottom:6px;color:#fff;";
  banner.textContent = "waiting for startup probe\u2026";
  var trail = document.createElement("div");
  panel.appendChild(head); panel.appendChild(banner); panel.appendChild(trail);
  function mount() {
    if (!document.body) { setTimeout(mount, 300); return; }
    document.body.appendChild(panel);
    panel.style.display = "block";
  }

  var lastCrumb = "(none yet)";
  var lastSurvived = "(none)";
  function crumb(msg) {
    lastCrumb = msg;
    var line = "[" + new Date().toISOString().slice(11, 23) + "] " + msg;
    try { console.log("PROBE " + line); } catch (e) { /* renderer dying */ }
    trail.textContent += line + "\n";
    if (panel.scrollHeight > panel.clientHeight) panel.scrollTop = panel.scrollHeight;
  }
  function setBanner(text, color) {
    banner.textContent = text;
    banner.style.color = color || "#fff";
  }
  var wait = function (ms) { return new Promise(function (r) { setTimeout(r, ms); }); };

  /* ---------- beacon (ntfy.sh) — best-effort, never fatal ----------------- */
  var cfg = DEFAULT_CONFIG;
  var beaconOk = 0, beaconFail = 0;
  function beacon(msg) {
    try {
      fetch("https://ntfy.sh/" + cfg.beaconTopic, {
        method: "POST",
        body: "[" + new Date().toISOString() + "] " + msg,
        headers: { "Title": "Onitor ComputeProbe" }
      }).then(function () { beaconOk++; }, function () { beaconFail++; });
    } catch (e) { beaconFail++; }
  }

  /* ---------- device acquisition (fresh adapter + device every stage) ----- */
  async function makeDevice(stage) {
    crumb(stage + ": navigator.gpu present = " + (!!navigator.gpu));
    if (!navigator.gpu) throw new Error("navigator.gpu missing - WebGPU unavailable");
    crumb(stage + ": requestAdapter...");
    var adapter = await navigator.gpu.requestAdapter();
    if (!adapter) throw new Error("requestAdapter returned null");
    var info = {};
    try { info = adapter.info || (adapter.requestAdapterInfo ? (await adapter.requestAdapterInfo()) : {}); }
    catch (e) { /* best effort */ }
    crumb(stage + ": adapter vendor=" + (info.vendor || "?") +
      " architecture=" + (info.architecture || "?") +
      " description=" + (info.description || "?"));
    var vendorish = String(info.vendor || "") + " " + String(info.description || "");
    if (vendorish.toLowerCase().indexOf("swiftshader") !== -1 && cfg.runOnFallback !== true) {
      throw new Error("RUN INVALID: adapter is SwiftShader (software) - expected the hardware adapter");
    }
    if (vendorish.toLowerCase().indexOf("swiftshader") !== -1 && cfg.runOnFallback === true) {
      crumb(stage + ": swiftshader adapter ACCEPTED (cfg.runOnFallback=true - gen6 swiftshader verification run)");
    }
    crumb(stage + ": requestDevice...");
    var device = await adapter.requestDevice();
    if (!device) throw new Error("requestDevice returned null");
    var state = { lost: false, reason: "", errorSeen: false };
    device.lost.then(function (why) {
      state.lost = true;
      state.reason = String(why && why.reason || "?") + " " + String(why && why.message || "");
      crumb("DEVICE_LOST (" + stage + ") reason=" + (why && why.reason) + " message=" + (why && why.message));
    });
    device.onuncapturederror = function (ev) {
      state.errorSeen = true;
      crumb("UNCAPTURED_ERROR (" + stage + ") " + (ev && ev.error && ev.error.message ? ev.error.message : "(no message)"));
    };
    await wait(200);
    return { device: device, state: state };
  }

  /* ---------- building blocks -------------------------------------------- */
  async function buildPipelineParts(stage, device) {
    crumb(stage + ": createShaderModule");
    var module = device.createShaderModule({ code: PROBE_WGSL });
    crumb(stage + ": createBindGroupLayout (read-only-storage + storage)");
    var bgl = device.createBindGroupLayout({ entries: [
      { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: "read-only-storage" } },
      { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: "storage" } }
    ]});
    crumb(stage + ": createPipelineLayout");
    var layout = device.createPipelineLayout({ bindGroupLayouts: [bgl] });
    return { module: module, bgl: bgl, layout: layout };
  }
  function createPipelineSync(stage, device, layout, module) {
    crumb(stage + ": createComputePipeline (SYNC)...");
    var pipeline = device.createComputePipeline({ layout: layout, compute: { module: module, entryPoint: "main" } });
    crumb(stage + ": PSO created (sync)");
    return pipeline;
  }
  function makeBuffers(stage, device) {
    var byteSize = ELEMENTS * 4;
    crumb(stage + ": createBuffer input (STORAGE|COPY_DST " + byteSize + "B)");
    var input = device.createBuffer({ size: byteSize, usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST });
    crumb(stage + ": createBuffer output (STORAGE|COPY_SRC " + byteSize + "B)");
    var output = device.createBuffer({ size: byteSize, usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC });
    crumb(stage + ": createBuffer readback (MAP_READ|COPY_DST " + byteSize + "B)");
    var readback = device.createBuffer({ size: byteSize, usage: GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST });
    return { input: input, output: output, readback: readback, byteSize: byteSize };
  }
  function writeInput(stage, device, input) {
    var data = new Uint32Array(ELEMENTS);
    for (var i = 0; i < ELEMENTS; i++) data[i] = i;
    crumb(stage + ": queue.writeBuffer(input)...");
    device.queue.writeBuffer(input, 0, data);
    crumb(stage + ": writeBuffer returned (memcpy now, CopyBufferRegion deferred to submit)");
  }
  function makeBindGroup(stage, device, bgl, input, output) {
    crumb(stage + ": createBindGroup");
    return device.createBindGroup({ layout: bgl, entries: [
      { binding: 0, resource: { buffer: input } },
      { binding: 1, resource: { buffer: output } }
    ]});
  }

  /* ---------- stages ------------------------------------------------------ */
  var STAGES = {
    async none() {
      var d = await makeDevice("none");
      crumb("none: device ready + lost-hook armed. No pipeline, no buffers, no submit.");
      await wait(cfg.settleMs);
      return d;
    },
    async psocreate() {
      var d = await makeDevice("psocreate");
      var parts = await buildPipelineParts("psocreate", d.device);
      createPipelineSync("psocreate", d.device, parts.layout, parts.module);
      crumb("psocreate: stopping here - no encoder, no submit");
      await wait(cfg.settleMs);
      return d;
    },
    async modlive() {
      var d = await makeDevice("modlive");
      crumb("modlive: createShaderModule (Tint WGSL->HLSL happens here)");
      var module = d.device.createShaderModule({ code: PROBE_WGSL });
      crumb("modlive: createBindGroupLayout + createPipelineLayout (NO pipeline)");
      var bgl = d.device.createBindGroupLayout({ entries: [
        { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: "read-only-storage" } },
        { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: "storage" } }
      ]});
      var layout = d.device.createPipelineLayout({ bindGroupLayouts: [bgl] });
      crumb("modlive: module + layouts alive, NO createComputePipeline - holding 5s...");
      await wait(5000);
      crumb("modlive: held 5s clean? (any DEVICE_LOST above answers module-vs-PSO) - destroying");
      d.device.destroy();
      await wait(2000);
      crumb("modlive: 2s after destroy");
      return d;
    },
    async psolive() {
      var d = await makeDevice("psolive");
      var parts = await buildPipelineParts("psolive", d.device);
      createPipelineSync("psolive", d.device, parts.layout, parts.module);
      crumb("psolive: PSO created - keeping device ALIVE 8s, NO destroy (isolation: creation vs teardown poison)");
      await wait(8000);
      crumb("psolive: held 8s - destroying now");
      d.device.destroy();
      await wait(2000);
      crumb("psolive: 2s after destroy");
      return d;
    },
    async psocreateasync() {
      var d = await makeDevice("psocreateasync");
      var parts = await buildPipelineParts("psocreateasync", d.device);
      crumb("psocreateasync: createComputePipelineAsync...");
      var pipeline = await d.device.createComputePipelineAsync({ layout: parts.layout, compute: { module: parts.module, entryPoint: "main" } });
      crumb("psocreateasync: PSO created (async path) - holding 4s");
      await wait(4000);
      d.device.destroy();
      await wait(2000);
      crumb("psocreateasync: 2s after destroy");
      return d;
    },
    async upload() {
      var d = await makeDevice("upload");
      var bufs = makeBuffers("upload", d.device);
      writeInput("upload", d.device, bufs.input);
      crumb("upload: submit EMPTY command encoder (flushes deferred staging copy)...");
      var enc = d.device.createCommandEncoder();
      d.device.queue.submit([enc.finish()]);
      crumb("upload: submitted (empty encoder)");
      await wait(cfg.settleMs);
      return d;
    },
    async macupload() {
      var d = await makeDevice("macupload");
      var data = new Uint32Array(ELEMENTS);
      for (var i = 0; i < ELEMENTS; i++) data[i] = i;
      var byteSize = ELEMENTS * 4;
      crumb("macupload: createBuffer storage (STORAGE|COPY_SRC|COPY_DST, mappedAtCreation:true, " + byteSize + "B)");
      var buf = d.device.createBuffer({
        mappedAtCreation: true,
        size: byteSize,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC | GPUBufferUsage.COPY_DST
      });
      crumb("macupload: buffer created mapped; writing initial data...");
      new Uint32Array(buf.getMappedRange()).set(data);
      crumb("macupload: unmap()...");
      buf.unmap();
      crumb("macupload: unmapped (Dawn defers the staging copy to the first submit)");
      var enc = d.device.createCommandEncoder();
      d.device.queue.submit([enc.finish()]);
      crumb("macupload: submitted (empty encoder flushes the deferred initial-data copy)");
      await wait(cfg.settleMs);
      return d;
    },
    async dispatch() {
      var d = await makeDevice("dispatch");
      var parts = await buildPipelineParts("dispatch", d.device);
      var pipeline = createPipelineSync("dispatch", d.device, parts.layout, parts.module);
      var bufs = makeBuffers("dispatch", d.device);
      writeInput("dispatch", d.device, bufs.input);
      var bindGroup = makeBindGroup("dispatch", d.device, parts.bgl, bufs.input, bufs.output);
      crumb("dispatch: encode compute pass: setPipeline, setBindGroup, dispatchWorkgroups(" + WORKGROUPS + ")...");
      var enc = d.device.createCommandEncoder();
      var pass = enc.beginComputePass();
      pass.setPipeline(pipeline);
      pass.setBindGroup(0, bindGroup);
      pass.dispatchWorkgroups(WORKGROUPS);
      pass.end();
      crumb("dispatch: compute pass encoded");
      d.device.queue.submit([enc.finish()]);
      crumb("dispatch: SUBMITTED (dispatch only - no readback copy, no map)");
      await wait(cfg.settleMs + 200);
      return d;
    },
    async copy() {
      var d = await makeDevice("copy");
      var parts = await buildPipelineParts("copy", d.device);
      var pipeline = createPipelineSync("copy", d.device, parts.layout, parts.module);
      var bufs = makeBuffers("copy", d.device);
      writeInput("copy", d.device, bufs.input);
      var bindGroup = makeBindGroup("copy", d.device, parts.bgl, bufs.input, bufs.output);
      crumb("copy: encode compute pass + copyBufferToBuffer in the SAME encoder...");
      var enc = d.device.createCommandEncoder();
      var pass = enc.beginComputePass();
      pass.setPipeline(pipeline);
      pass.setBindGroup(0, bindGroup);
      pass.dispatchWorkgroups(WORKGROUPS);
      pass.end();
      crumb("copy: compute pass encoded");
      enc.copyBufferToBuffer(bufs.output, 0, bufs.readback, 0, bufs.byteSize);
      crumb("copy: copyBufferToBuffer recorded (same command buffer)");
      d.device.queue.submit([enc.finish()]);
      crumb("copy: SUBMITTED (dispatch + readback copy - no map)");
      await wait(cfg.settleMs + 200);
      return d;
    },
    async signal() {
      var d = await makeDevice("signal");
      var parts = await buildPipelineParts("signal", d.device);
      var pipeline = createPipelineSync("signal", d.device, parts.layout, parts.module);
      var bufs = makeBuffers("signal", d.device);
      writeInput("signal", d.device, bufs.input);
      var bindGroup = makeBindGroup("signal", d.device, parts.bgl, bufs.input, bufs.output);
      var enc = d.device.createCommandEncoder();
      var pass = enc.beginComputePass();
      pass.setPipeline(pipeline);
      pass.setBindGroup(0, bindGroup);
      pass.dispatchWorkgroups(WORKGROUPS);
      pass.end();
      enc.copyBufferToBuffer(bufs.output, 0, bufs.readback, 0, bufs.byteSize);
      d.device.queue.submit([enc.finish()]);
      crumb("signal: submitted; now queue.onSubmittedWorkDone()...");
      await d.device.queue.onSubmittedWorkDone();
      crumb("signal: onSubmittedWorkDone resolved (queue fence survived) - no map");
      await wait(cfg.settleMs);
      return d;
    },
    async full() {
      var d = await makeDevice("full");
      var parts = await buildPipelineParts("full", d.device);
      var pipeline = createPipelineSync("full", d.device, parts.layout, parts.module);
      var bufs = makeBuffers("full", d.device);
      writeInput("full", d.device, bufs.input);
      var bindGroup = makeBindGroup("full", d.device, parts.bgl, bufs.input, bufs.output);
      var enc = d.device.createCommandEncoder();
      var pass = enc.beginComputePass();
      pass.setPipeline(pipeline);
      pass.setBindGroup(0, bindGroup);
      pass.dispatchWorkgroups(WORKGROUPS);
      pass.end();
      enc.copyBufferToBuffer(bufs.output, 0, bufs.readback, 0, bufs.byteSize);
      d.device.queue.submit([enc.finish()]);
      crumb("full: submitted; now readback.mapAsync(READ)...");
      await bufs.readback.mapAsync(GPUMapMode.READ);
      crumb("full: readback mapped");
      var got = new Uint32Array(bufs.readback.getMappedRange());
      var ok = (got.length === ELEMENTS), firstBad = -1;
      for (var i = 0; i < got.length && ok; i++) {
        if (got[i] !== (2 * i + 1)) { ok = false; firstBad = i; }
      }
      crumb(ok
        ? "full: CHECK_PASSED (output[i] == 2i+1 for all " + ELEMENTS + " elements)"
        : "full: CHECK_FAILED firstBad=" + firstBad +
          " got=" + (firstBad >= 0 ? got[firstBad] : "?") +
          " expected=" + (firstBad >= 0 ? (2 * firstBad + 1) : "?"));
      bufs.readback.unmap();
      await wait(600);
      return d;
    },
    async webgpucheck() {
      /* EXACT replica of webgpucheck.com's compute test (f32, wg(1),
         mappedAtCreation upload, copy, mapAsync, x2 verification) */
      var d = await makeDevice("webgpucheck");
      crumb("webgpucheck: createShaderModule (exact site WGSL, f32 read_write, workgroup_size(1))");
      var module = d.device.createShaderModule({ code: WGC_WGSL });
      if (module.getCompilationInfo) {
        try {
          var ci = await module.getCompilationInfo();
          var hasErr = false;
          for (var m = 0; m < ci.messages.length; m++) {
            if (ci.messages[m].type === "error") { hasErr = true; break; }
          }
          crumb("webgpucheck: getCompilationInfo errors=" + hasErr);
          if (hasErr) throw new Error("WGSL compiled with diagnostic errors");
        } catch (e) { crumb("webgpucheck: getCompilationInfo failed: " + (e && e.message)); }
      }
      var e = new Float32Array(WGC_INPUT);
      crumb("webgpucheck: createBuffer storage mappedAtCreation:true " + e.byteLength + "B (STORAGE|COPY_SRC|COPY_DST)");
      var r = d.device.createBuffer({
        mappedAtCreation: true,
        size: e.byteLength,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC | GPUBufferUsage.COPY_DST
      });
      new Float32Array(r.getMappedRange()).set(e);
      r.unmap();
      crumb("webgpucheck: storage buffer written at creation + unmapped");
      var i2 = d.device.createBuffer({ size: e.byteLength, usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ });
      var a = d.device.createBindGroupLayout({ entries: [
        { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: "storage" } }
      ]});
      var o = d.device.createBindGroup({ layout: a, entries: [{ binding: 0, resource: { buffer: r } }] });
      var s = d.device.createPipelineLayout({ bindGroupLayouts: [a] });
      crumb("webgpucheck: createComputePipeline...");
      var c = d.device.createComputePipeline({ layout: s, compute: { module: module, entryPoint: "main" } });
      crumb("webgpucheck: PSO created; encoding pass...");
      var l = d.device.createCommandEncoder();
      var u = l.beginComputePass();
      u.setPipeline(c);
      u.setBindGroup(0, o);
      u.dispatchWorkgroups(e.length);
      u.end();
      l.copyBufferToBuffer(r, 0, i2, 0, e.byteLength);
      d.device.queue.submit([l.finish()]);
      crumb("webgpucheck: submitted; now mapAsync(READ) (the site's failing call)...");
      await i2.mapAsync(GPUMapMode.READ);
      crumb("webgpucheck: readback mapped");
      var f = new Float32Array(i2.getMappedRange());
      var p = true;
      for (var t = 0; t < e.length; t++) {
        if (Math.abs(f[t] - e[t] * 2) > 0.001) { p = false; break; }
      }
      crumb(p
        ? "webgpucheck: CHECK_PASSED ([" + Array.prototype.join.call(f, ", ") + "])"
        : "webgpucheck: CHECK_FAILED got=[" + Array.prototype.join.call(f, ", ") + "]");
      i2.unmap();
      await wait(600);
      return d;
    }
  };

  /* ---------- stage runner: settle, lost-detection, cleanup --------------- */
  async function runStage(name) {
    crumb("==== STAGE " + name + " START ====");
    var t0 = performance.now();
    var result = null, error = null;
    try { result = await STAGES[name](); }
    catch (e) { error = e; }
    var dur = Math.round(performance.now() - t0);
    if (error) {
      crumb("==== STAGE " + name + " FAILED (exception after " + dur + "ms): " +
        (error && error.message ? error.message : String(error)) + " ====");
      return { ok: false, why: "exception" };
    }
    var state = result && result.state;
    if (state && state.lost) {
      crumb("==== STAGE " + name + " FAILED (device lost after " + dur + "ms: " + state.reason + ") ====");
      return { ok: false, why: "device-lost" };
    }
    if (state && state.errorSeen) {
      crumb("==== STAGE " + name + " FAILED (uncaptured GPU error after " + dur + "ms) ====");
      return { ok: false, why: "uncaptured-error" };
    }
    try {
      result.device.destroy();
      crumb(name + ": device.destroy ok");
    } catch (e) { crumb(name + ": device.destroy threw: " + (e && e.message)); }
    crumb("==== STAGE " + name + " SURVIVED (" + dur + "ms) ====");
    lastSurvived = name;
    return { ok: true, why: "" };
  }

  /* ---------- wait for the app's own startup probe ------------------------ */
  function readAppProbe() {
    try { return window.__onitorWgpuProbe || ""; } catch (e) { return ""; }
  }
  async function waitForStartupProbe(maxMs) {
    var waited = 0, step = 500;
    while (waited < maxMs) {
      var v = readAppProbe();
      if (v && v !== "pending" && v !== "undefined") return v;
      await wait(step); waited += step;
    }
    return readAppProbe();
  }

  /* ---------- main -------------------------------------------------------- */
  async function main() {
    mount();
    crumb("probe loaded (build 1.6.2.0) ua=" + navigator.userAgent.slice(0, 120));

    /* remote steering config (cache-busted; fall back to defaults) */
    try {
      var res = await fetch(CONFIG_URL + "?t=" + Date.now(), { cache: "no-store" });
      if (res.ok) {
        var remote = await res.json();
        if (remote && typeof remote === "object") {
          cfg = Object.assign({}, DEFAULT_CONFIG, remote);
          crumb("remote config loaded v" + (remote.version || "?") + " note='" + (remote.note || "") + "'");
        }
      } else {
        crumb("remote config HTTP " + res.status + " - using defaults");
      }
    } catch (e) {
      crumb("remote config unreachable (" + (e && e.message) + ") - using defaults");
    }

    if (cfg.enabled === false) {
      crumb("config.enabled=false - probe disabled remotely. Nothing runs.");
      setBanner("probe disabled (remote config)", "#78909c");
      panel.style.display = "none"; // stay invisible for the user
      return;
    }
    try { sessionStorage.setItem("computeProbeDone", "1"); } catch (e) {}

    beacon("session START note='" + (cfg.note || "") + "' stages=" + cfg.stages.join(","));

    /* gate on the app's startup probe verdict */
    setBanner("waiting for startup probe\u2026", "#fff");
    var verdict = await waitForStartupProbe(30000);
    crumb("startup probe verdict: '" + verdict + "'");
    if (verdict && verdict.indexOf("gpu:") === 0) {
      crumb("hardware adapter confirmed by the app probe - starting compute ladder in 2s");
      await wait(2000);
    } else if (verdict && (verdict.indexOf("fallback:") === 0 || verdict === "null" || verdict === "none" || verdict === "error")) {
      if (cfg.runOnFallback === true && verdict.indexOf("fallback:") === 0) {
        /* gen6 (v1.6.4.0): the app's compute-safe default IS the swiftshader
           rung, so software sessions are now first-class verification
           targets. Run the ladder and let the stages prove compute PASS. */
        crumb("software session but cfg.runOnFallback=true - starting compute ladder on the swiftshader adapter (gen6 verification)");
        setBanner("swiftshader verification run\u2026", "#80d8ff");
        await wait(2000);
      } else {
        var why = "startup probe verdict '" + verdict + "' - not a hardware session, compute probe ABORTED (ladder would restart the app)";
        crumb(why);
        setBanner("aborted: not a hardware session", "#ffb300");
        beacon("ABORT " + why);
        return;
      }
    } else {
      /* no/late verdict: the app poller's window is over either way (30s > its
         2.5s + 18x600ms). Proceeding cannot corrupt the ladder. */
      crumb("no startup verdict after 30s (app poller window closed either way) - proceeding");
      beacon("NOTE no startup verdict after 30s - proceeding anyway");
    }

    var failed = null;
    for (var s = 0; s < cfg.stages.length; s++) {
      var name = cfg.stages[s];
      if (!STAGES[name]) { crumb("unknown stage '" + name + "' skipped"); continue; }
      var r = await runStage(name);
      if (!r.ok) {
        failed = { name: name, why: r.why };
        beacon("STAGE_FAILED " + name + " (" + r.why + ") last_crumb=" + lastCrumb);
        if (cfg.haltOnFault !== false) break;
      } else {
        beacon("STAGE_OK " + name);
      }
      await wait(400);
    }

    if (failed) {
      crumb("SEQUENCE_RESULT: died_at=" + failed.name + " why=" + failed.why +
        " last_survived=" + lastSurvived + " last_breadcrumb=" + lastCrumb);
      setBanner("DIED AT STAGE: " + failed.name + " (" + failed.why + ")", "#ff8a80");
      beacon("VERDICT DIED_AT=" + failed.name + " why=" + failed.why + " last_survived=" + lastSurvived + " last_breadcrumb=" + lastCrumb);
    } else {
      crumb("SEQUENCE_RESULT: ALL_STAGES_SURVIVED (" + cfg.stages.join(",") + ")");
      setBanner("ALL STAGES SURVIVED", "#b9f6ca");
      beacon("VERDICT ALL_STAGES_SURVIVED (" + cfg.stages.join(",") + ")");
    }
  }

  main().catch(function (e) {
    crumb("FATAL probe error: " + (e && e.message ? e.message : String(e)));
    setBanner("fatal probe error - see trail", "#ff8a80");
    beacon("FATAL " + (e && e.message ? e.message : String(e)));
  });
})();
