"use strict";
/* ============================================================================
   wgpu-shim.js — navigator.gpu compute shim backed by the hardware WebGL2 engine
   ============================================================================
   WHY THIS EXISTS
   ---------------
   On the Xbox (SystemOS + Edge WebView2) a real WebGPU compute dispatch kills
   the GPU process:
       ID3D12Device::CreateHeap failed with DXGI_ERROR_DEVICE_REMOVED
       Device removed reason: DXGI_ERROR_DRIVER_INTERNAL_ERROR (0x887A0020)
       GPU process exited unexpectedly: exit_code=34
   The adapter, device, buffers and compute PSO are all created successfully -
   it is the actual dispatch that faults. Pages that only ever call
   navigator.gpu therefore fail on the console.

   hwcompute.js already runs general-purpose compute on the *same* console GPU
   through ANGLE/D3D11 (WebGL2 fragment-shader passes), and that stack works.
   This file installs a WebGPU-compute-compatible facade on navigator.gpu that
   executes dispatches through HWCompute instead of Dawn.

   HOW IT WORKS
   ------------
     navigator.gpu  ->  WgpuShim GPU object  ->  WgpuShimDevice
                                                    |-- translated pipeline:
                                                    |     WGSL -> restricted GLSL
                                                    |     -> HWCompute.runEx()
                                                    |     -> WebGL2 on the real GPU
                                                    '-- untranslatable shader:
                                                          delegate to the real
                                                          WebGPU device, or
                                                          throw a loud Error.
                                                          NEVER guess.

   Storage buffers are mirrored CPU-side (an ArrayBuffer per GPUBuffer), so
   writeBuffer / mappedAtCreation / getMappedRange / mapAsync all work without
   the GPU being involved, and each dispatch uploads only what it needs. f32,
   u32 and i32 buffers all round-trip bit-exactly through RGBA32F textures
   (values are reinterpreted with floatBitsToUint / uintBitsToFloat, never
   numerically converted). WgpuShim.stats.caps.u32Exact records whether the
   host GPU really preserves every bit pattern, measured at install time.

   INSTALL (nothing happens unless you ask)
   ----------------------------------------
       <script src="hwcompute.js"></script>
       <script src="wgpu-shim.js"></script>
       <script>
         var r = WgpuShim.install({ force: true });   // force is REQUIRED when
         console.log(r);                             // navigator.gpu exists
       </script>

   install() with no options refuses to replace an existing navigator.gpu (the
   safe default - it will not silently hijack a browser whose WebGPU works).
   The native object is captured first, so `delegateUnsupported: true` can hand
   untranslatable shaders back to real WebGPU. NOTE: on the Xbox that real path
   is exactly what faults the GPU process, so delegation defaults to OFF and is
   strictly opt-in.

   WHAT IS SUPPORTED / WHAT IS REFUSED
   -----------------------------------
   SUPPORTED WGSL (the element-wise compute subset)
     @group(N) @binding(M) var<storage, read|read_write> name : array<f32|u32|i32>;
     @compute @workgroup_size(x[,y,z]) fn main(@builtin(global_invocation_id)
         gid : vec3<u32>) { ... }            (gid.x, gid.y and gid.z, so 1D, 2D
                                              and 3D dispatches all work)
     let / var / const locals (typed or inferred), arithmetic, comparison
     (< > <= >= == !=), && || !, bitwise & | ^ ~ << >>, if/else, for, while,
     break, continue, return, blocks, assignment (= += -= *= /= %=), indexing
     buf[i], arrayLength(&buf), select(a,b,c), bitcast<T>(x), constructors
     f32/u32/i32/bool and vec2/3/4<T>(...), module-scope `const`, and swizzles.
     Builtins: abs min max clamp floor ceil round sqrt pow exp exp2 log log2
     sin cos tan asin acos atan atan2 sinh cosh tanh asinh acosh atanh
     mix step smoothstep fract sign trunc length dot distance cross normalize
     reflect refract faceForward any all inverseSqrt degrees radians.

   REFUSED (loudly - never silently mis-computed)
     workgroupBarrier / storageBarrier / textureBarrier, var<workgroup>,
     var<uniform>, var<private>, atomics (atomic<...>, atomicAdd, ...),
     textures and samplers, every @builtin except global_invocation_id, struct
     types, array<struct>, array<vec4<f32>> and other non-scalar elements,
     matrices, user-defined functions, an entry point that returns a value,
     switch, loop, discard, ++/--, ?:, local arrays, pointers and address-of
     (except arrayLength(&buf)), unrecognised builtins, unrecognised syntax,
     and reads of a storage buffer that lexically follow a write to the same
     buffer in one dispatch.

   STORE INDICES are checked symbolically. Every write to a storage buffer must
   reduce to a linear form a*gid.x + b*gid.y + c*gid.z + k, and at dispatch time
   the shim proves that form equals the invocation's own element for that exact
   dispatch (nx/ny/nz). So `out[gid.x]`, `out[i]` where `let i = gid.x`, and
   `out[gid.y * WIDTH + gid.x]` in a 2D dispatch all work; a genuine scatter
   (out[gid.x * 2u], out[gid.x + 1u], a fixed index, or a gid.x store inside a
   2D dispatch, where several invocations would target one element) is refused
   at pipeline creation or at submit, and recorded on WgpuShim.stats.

   ============================================================================ */

(function (global) {
  var VERSION = "1.0.0";

  /* ==========================================================================
   * 0. tiny helpers
   * ======================================================================== */

  function refuse(reason) {
    var e = new Error(reason);
    e.wgslRefuse = true;
    throw e;
  }

  function isIdStart(c) {
    return (c >= "a" && c <= "z") || (c >= "A" && c <= "Z") || c === "_";
  }
  function isIdChar(c) {
    return isIdStart(c) || (c >= "0" && c <= "9");
  }
  function isDigit(c) { return c >= "0" && c <= "9"; }
  function has(o, k) { return Object.prototype.hasOwnProperty.call(o, k); }

  /* ==========================================================================
   * 1. WGSL tokenizer
   * ======================================================================== */

  // Longest first, so ">>=" wins over ">>" over ">".
  var PUNCT = [
    "<<=", ">>=", "==", "!=", "<=", ">=", "&&", "||", "<<", ">>", "->",
    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "++", "--", "::",
    "(", ")", "{", "}", "[", "]", ",", ".", ";", ":", "@", "=", "<", ">",
    "+", "-", "*", "/", "%", "&", "|", "^", "~", "!", "?"
  ];

  function tokenize(src) {
    var toks = [];
    var i = 0, n = src.length, line = 1;
    while (i < n) {
      var c = src.charAt(i);
      if (c === "\n") { line++; i++; continue; }
      if (c === " " || c === "\t" || c === "\r" || c === "\f" || c === "\v") { i++; continue; }
      // comments (WGSL block comments nest)
      if (c === "/" && src.charAt(i + 1) === "/") {
        while (i < n && src.charAt(i) !== "\n") i++;
        continue;
      }
      if (c === "/" && src.charAt(i + 1) === "*") {
        var depth = 1;
        i += 2;
        while (i < n && depth > 0) {
          if (src.charAt(i) === "/" && src.charAt(i + 1) === "*") { depth++; i += 2; }
          else if (src.charAt(i) === "*" && src.charAt(i + 1) === "/") { depth--; i += 2; }
          else { if (src.charAt(i) === "\n") line++; i++; }
        }
        continue;
      }
      if (isIdStart(c)) {
        var j = i;
        while (j < n && isIdChar(src.charAt(j))) j++;
        toks.push({ k: "id", v: src.slice(i, j), line: line });
        i = j;
        continue;
      }
      if (isDigit(c) || (c === "." && isDigit(src.charAt(i + 1) || ""))) {
        var st = i, isFloat = false, text = "";
        if (c === "0" && (src.charAt(i + 1) === "x" || src.charAt(i + 1) === "X")) {
          i += 2;
          var hs = i;
          while (i < n && /[0-9a-fA-F]/.test(src.charAt(i))) i++;
          if (i === hs) refuse("line " + line + ": malformed hexadecimal literal");
          text = src.slice(st, i);
        } else {
          while (i < n && isDigit(src.charAt(i))) i++;
          if (src.charAt(i) === "." && isDigit(src.charAt(i + 1) || "")) {
            isFloat = true;
            i++;
            while (i < n && isDigit(src.charAt(i))) i++;
          }
          if (src.charAt(i) === "e" || src.charAt(i) === "E") {
            var save = i;
            i++;
            if (src.charAt(i) === "+" || src.charAt(i) === "-") i++;
            if (isDigit(src.charAt(i) || "")) {
              isFloat = true;
              while (i < n && isDigit(src.charAt(i))) i++;
            } else { i = save; }
          }
          text = src.slice(st, i);
        }
        var suffix = "";
        var sc = src.charAt(i);
        if (sc === "f" || sc === "F" || sc === "u" || sc === "U" || sc === "i" || sc === "I" || sc === "h" || sc === "H") {
          suffix = sc.toLowerCase();
          i++;
        }
        if (suffix === "h") refuse("line " + line + ": f16 literals are not supported");
        if (suffix === "f") isFloat = true;
        var value = suffix === "u" || suffix === "i"
          ? parseInt(text, text.indexOf("0x") === 0 ? 16 : 10)
          : parseFloat(text);
        toks.push({
          k: "num", v: text + suffix, text: text, suffix: suffix,
          isFloat: isFloat, value: value, line: line
        });
        continue;
      }
      if (c === '"' || c === "'") refuse("line " + line + ": string literals are not supported");
      var matched = null;
      for (var p = 0; p < PUNCT.length; p++) {
        if (src.substr(i, PUNCT[p].length) === PUNCT[p]) { matched = PUNCT[p]; break; }
      }
      if (!matched) refuse("line " + line + ": unexpected character '" + c + "'");
      toks.push({ k: "punct", v: matched, line: line });
      i += matched.length;
    }
    return toks;
  }

  /* ==========================================================================
   * 2. WGSL parser  (module + entry point body)
   * ======================================================================== */

  var SCALAR = { f32: 1, u32: 1, i32: 1, bool: 1 };
  var VEC_ALIAS = {
    vec2f: "vec2<f32>", vec2u: "vec2<u32>", vec2i: "vec2<i32>",
    vec3f: "vec3<f32>", vec3u: "vec3<u32>", vec3i: "vec3<i32>",
    vec4f: "vec4<f32>", vec4u: "vec4<u32>", vec4i: "vec4<i32>"
  };

  function Parser(toks) { this.t = toks; this.p = 0; }
  Parser.prototype.peek = function (k) { return this.t[this.p + (k || 0)]; };
  Parser.prototype.next = function () { return this.t[this.p++]; };
  Parser.prototype.at = function (v, k) { var t = this.peek(k); return !!t && t.v === v; };
  Parser.prototype.atId = function (v, k) { var t = this.peek(k); return !!t && t.k === "id" && t.v === v; };
  Parser.prototype.eat = function (v) { if (this.at(v)) { this.p++; return true; } return false; };
  Parser.prototype.expect = function (v) {
    if (!this.at(v)) {
      var t = this.peek();
      refuse("expected '" + v + "' but found '" + (t ? t.v : "end of source") + "'" +
        (t ? " (line " + t.line + ")" : ""));
    }
    this.p++;
  };
  Parser.prototype.expectId = function () {
    var t = this.next();
    if (!t || t.k !== "id") refuse("expected an identifier but found '" + (t ? t.v : "end of source") + "'");
    return t.v;
  };

  function parseAttributes(p) {
    var out = [];
    while (p.at("@")) {
      p.p++;
      var name = p.expectId();
      var args = [];
      if (p.eat("(")) {
        var depth = 1;
        while (depth > 0) {
          if (p.p >= p.t.length) refuse("unterminated attribute @" + name);
          var tk = p.next();
          if (tk.v === "(") depth++;
          else if (tk.v === ")") { depth--; if (depth === 0) break; }
          args.push(tk);
        }
      }
      out.push({ name: name, args: args });
    }
    return out;
  }

  function attrOf(attrs, name) {
    for (var i = 0; i < attrs.length; i++) if (attrs[i].name === name) return attrs[i];
    return null;
  }

  function parseType(p) {
    var t = p.peek();
    if (!t || t.k !== "id") refuse("expected a type name but found '" + (t ? t.v : "end of source") + "'");
    var name = t.v;
    p.p++;
    if (name === "vec2" || name === "vec3" || name === "vec4") {
      var el = "f32";
      if (p.eat("<")) { el = p.expectId(); p.expect(">"); }
      if (!SCALAR[el]) refuse("unsupported vector element type '" + el + "'");
      return name + "<" + el + ">";
    }
    if (has(VEC_ALIAS, name)) return VEC_ALIAS[name];
    if (SCALAR[name]) return name;
    if (name === "array") refuse("nested arrays and array-typed values are not supported");
    if (name === "ptr") refuse("pointer types are not supported");
    if (name === "atomic") refuse("atomic types are not supported");
    if (name === "mat2x2" || name === "mat3x3" || name === "mat4x4" || name.indexOf("mat") === 0) {
      refuse("matrix types are not supported");
    }
    refuse("unsupported type '" + name + "' (only f32, u32, i32, bool and vec2/3/4 of those)");
  }

  // ---- module scope -------------------------------------------------------

  function parseModule(src) {
    var toks = tokenize(src);
    var p = new Parser(toks);
    var mod = { bindings: [], consts: [], entry: null, gidName: null, workgroupSize: [1, 1, 1] };
    while (p.p < p.t.length) {
      if (p.eat(";")) continue;
      var t = p.peek();
      var attrs = parseAttributes(p);
      t = p.peek();
      if (!t) break;
      if (t.k !== "id") refuse("line " + t.line + ": unexpected token '" + t.v + "' at module scope");
      if (t.v === "enable" || t.v === "requires" || t.v === "diagnostic") {
        while (p.p < p.t.length && !p.at(";")) p.p++;
        p.eat(";");
        continue;
      }
      if (t.v === "struct") refuse("struct types are not supported (storage buffers must be array<f32|u32|i32>)");
      if (t.v === "alias") refuse("alias declarations are not supported");
      if (t.v === "override") refuse("override declarations are not supported");
      if (t.v === "const") { parseConst(p, mod); continue; }
      if (t.v === "var") { parseGlobalVar(p, attrs, mod); continue; }
      if (t.v === "fn") { parseFn(p, attrs, mod); continue; }
      refuse("line " + t.line + ": unsupported module-scope declaration '" + t.v + "'");
    }
    return mod;
  }

  function parseConst(p, mod) {
    p.expect("const");
    var name = p.expectId();
    var type = null;
    if (p.eat(":")) type = parseType(p);
    p.expect("=");
    var expr = parseExpr(p, 0);
    p.expect(";");
    mod.consts.push({ name: name, type: type, expr: expr });
  }

  function parseGlobalVar(p, attrs, mod) {
    p.expect("var");
    var addrSpace = "function";
    var access = "read";
    if (p.eat("<")) {
      addrSpace = p.expectId();
      if (p.eat(",")) access = p.expectId();
      p.expect(">");
    }
    var spaceProblem = null;
    if (addrSpace === "workgroup") spaceProblem = "var<workgroup> (workgroup shared memory) is not supported";
    else if (addrSpace === "uniform") spaceProblem = "var<uniform> (uniform buffers) is not supported";
    else if (addrSpace === "private") spaceProblem = "var<private> (module-scope mutable state) is not supported";
    else if (addrSpace !== "storage") spaceProblem = "unsupported address space '" + addrSpace + "'";
    else if (access !== "read" && access !== "read_write") {
      spaceProblem = "storage access mode '" + access + "' is not supported (write-only storage)";
    }
    var name = p.expectId();
    p.expect(":");
    // A handle type is the most useful diagnosis, so check it first.
    var typeTok = p.peek();
    if (typeTok && typeTok.k === "id" &&
        (typeTok.v.indexOf("texture") === 0 || typeTok.v === "sampler" || typeTok.v === "sampler_comparison")) {
      refuse("textures and samplers are not supported ('" + name + "' is a " + typeTok.v + ")");
    }
    if (spaceProblem) refuse(spaceProblem);
    // array<T> or array<T, N>
    if (!p.atId("array")) {
      var bad = p.peek();
      refuse("storage buffer '" + name + "' must be declared as array<f32|u32|i32>, found '" +
        (bad ? bad.v : "?") + "'");
    }
    p.expect("array");
    p.expect("<");
    var elem = p.expectId();
    if (!SCALAR[elem] || elem === "bool") {
      refuse("storage buffer '" + name + "' element type '" + elem +
        "' is not supported (only f32, u32, i32)");
    }
    if (p.eat(",")) {
      var lenTok = p.next();
      if (!lenTok || lenTok.k !== "num" || lenTok.isFloat) refuse("array length must be an integer literal");
      if (lenTok.suffix === "u" || lenTok.suffix === "i") refuse("array length must be an abstract integer literal");
    }
    p.expect(">");
    p.expect(";");

    var g = attrOf(attrs, "group");
    var b = attrOf(attrs, "binding");
    if (!g || !b) refuse("storage buffer '" + name + "' needs both @group(N) and @binding(M)");
    if (g.args.length !== 1 || b.args.length !== 1 ||
        g.args[0].k !== "num" || b.args[0].k !== "num") {
      refuse("@" + (g.args.length !== 1 ? "group" : "binding") + " needs a single integer literal");
    }
    for (var a = 0; a < attrs.length; a++) {
      if (attrs[a].name !== "group" && attrs[a].name !== "binding") {
        refuse("@" + attrs[a].name + " is not supported on a storage buffer");
      }
    }
    mod.bindings.push({
      name: name, group: g.args[0].value, binding: b.args[0].value,
      access: access, elem: elem
    });
  }

  function parseFn(p, attrs, mod) {
    p.expect("fn");
    var name = p.expectId();
    p.expect("(");
    var params = [];
    if (!p.at(")")) {
      do {
        var pattrs = parseAttributes(p);
        var pname = p.expectId();
        p.expect(":");
        var ptype = parseType(p);
        params.push({ attrs: pattrs, name: pname, type: ptype });
      } while (p.eat(","));
    }
    p.expect(")");
    var ret = null;
    if (p.eat("->")) ret = parseType(p);
    var body = parseBlock(p);

    if (!attrOf(attrs, "compute")) {
      refuse("user-defined function '" + name + "' is not supported (only the @compute entry point)");
    }
    if (mod.entry) refuse("more than one @compute entry point in one module is not supported");
    var wg = attrOf(attrs, "workgroup_size");
    if (!wg) refuse("@compute entry point '" + name + "' needs @workgroup_size(...)");
    var wgArgs = [];
    for (var wa = 0; wa < wg.args.length; wa++) {
      if (wg.args[wa].v === ",") continue;
      wgArgs.push(wg.args[wa]);
    }
    if (wgArgs.length < 1 || wgArgs.length > 3) refuse("@workgroup_size takes 1 to 3 arguments");
    var sizes = [1, 1, 1];
    for (var w = 0; w < wgArgs.length; w++) {
      var at = wgArgs[w];
      if (at.k !== "num" || at.isFloat) refuse("@workgroup_size arguments must be integer literals");
      sizes[w] = at.value | 0;
      if (sizes[w] < 1) refuse("@workgroup_size arguments must be >= 1");
    }
    if (ret) refuse("a @compute entry point must not return a value");
    if (params.length !== 1) {
      refuse("the @compute entry point must take exactly one parameter " +
        "(@builtin(global_invocation_id) gid : vec3<u32>), found " + params.length);
    }
    var pr = params[0];
    var bAttr = attrOf(pr.attrs, "builtin");
    if (!bAttr) refuse("entry point parameter '" + pr.name + "' is not a builtin");
    var bName = bAttr.args.length === 1 ? bAttr.args[0].v : "?";
    if (bName !== "global_invocation_id") {
      refuse("@builtin(" + bName + ") is not supported - only @builtin(global_invocation_id)");
    }
    if (pr.type !== "vec3<u32>") {
      refuse("@builtin(global_invocation_id) must be declared as vec3<u32>, found " + pr.type);
    }
    mod.entry = { name: name, gidName: pr.name, body: body };
    mod.workgroupSize = sizes;
  }

  // ---- statements ---------------------------------------------------------

  function parseBlock(p) {
    p.expect("{");
    var stmts = [];
    while (!p.at("}")) {
      if (p.p >= p.t.length) refuse("unexpected end of source inside a block");
      stmts.push(parseStmt(p));
    }
    p.expect("}");
    return { k: "block", stmts: stmts };
  }

  var ASSIGN_OPS = { "=": 1, "+=": 1, "-=": 1, "*=": 1, "/=": 1, "%=": 1, "&=": 1, "|=": 1, "^=": 1, "<<=": 1, ">>=": 1 };

  function parseStmt(p) {
    var t = p.peek();
    if (!t) refuse("unexpected end of source");
    if (t.v === "{") return parseBlock(p);
    if (t.k === "id") {
      switch (t.v) {
        case "if": return parseIf(p);
        case "for": return parseFor(p);
        case "while": return parseWhile(p);
        case "loop": refuse("`loop` is not supported (use `for` or `while`)");
        case "switch": refuse("`switch` is not supported");
        case "discard": refuse("`discard` is not supported in a compute shader");
        case "break":
          p.p++;
          if (p.atId("if")) refuse("`break if` is not supported");
          p.expect(";");
          return { k: "break" };
        case "continue": p.p++; p.expect(";"); return { k: "continue" };
        case "return": {
          p.p++;
          var e = null;
          if (!p.at(";")) e = parseExpr(p, 0);
          p.expect(";");
          return { k: "return", e: e };
        }
        case "let":
        case "var":
        case "const": return parseLocalDecl(p);
        default: break;
      }
    }
    // expression statement, possibly an assignment
    var lhs = parseExpr(p, 0);
    var op = p.peek();
    if (op && op.v === "_" ) refuse("'=' as a phony assignment (WGSL '=' _) is not supported");
    if (op && ASSIGN_OPS[op.v]) {
      p.p++;
      var rhs = parseExpr(p, 0);
      p.expect(";");
      return { k: "assign", op: op.v, target: lhs, value: rhs, line: op.line };
    }
    p.expect(";");
    return { k: "exprStmt", e: lhs };
  }

  function parseLocalDecl(p) {
    var kind = p.next().v;                    // let | var | const
    var name = p.expectId();
    var type = null;
    if (p.eat(":")) type = parseType(p);
    var init = null;
    if (p.eat("=")) init = parseExpr(p, 0);
    p.expect(";");
    if (kind !== "var" && !init) refuse("'" + kind + "' requires an initialiser");
    if (kind !== "var" && kind !== "let" && kind !== "const") refuse("unsupported declaration '" + kind + "'");
    return { k: "decl", kind: kind, name: name, type: type, init: init, line: p.peek() ? p.peek().line : 0 };
  }

  function parseIf(p) {
    p.expect("if");
    var paren = p.eat("(");
    var cond = parseExpr(p, 0);
    if (paren) p.expect(")");
    var then = parseStmt(p);
    var els = null;
    if (p.atId("else")) { p.p++; els = parseStmt(p); }
    return { k: "if", cond: cond, then: then, els: els };
  }

  function parseWhile(p) {
    p.expect("while");
    var paren = p.eat("(");
    var cond = parseExpr(p, 0);
    if (paren) p.expect(")");
    return { k: "while", cond: cond, body: parseStmt(p) };
  }

  function parseFor(p) {
    p.expect("for");
    p.expect("(");
    var init = null;
    if (!p.at(";")) {
      if (p.atId("var") || p.atId("let") || p.atId("const")) init = parseLocalDecl(p);
      else {
        var lhs = parseExpr(p, 0);
        var op = p.next();
        if (!op || !ASSIGN_OPS[op.v]) refuse("a for-loop initialiser must be a variable declaration or an assignment");
        var v = parseExpr(p, 0);
        p.expect(";");
        init = { k: "assign", op: op.v, target: lhs, value: v, line: op.line };
      }
    } else { p.expect(";"); }
    var cond = null;
    if (!p.at(";")) cond = parseExpr(p, 0);
    p.expect(";");
    var cont = null;
    if (!p.at(")")) {
      var lhs2 = parseExpr(p, 0);
      var op2 = p.next();
      if (!op2 || !ASSIGN_OPS[op2.v]) refuse("a for-loop continue expression must be an assignment");
      var v2 = parseExpr(p, 0);
      cont = { k: "assign", op: op2.v, target: lhs2, value: v2, line: op2.line };
    }
    p.expect(")");
    return { k: "for", init: init, cond: cond, cont: cont, body: parseStmt(p) };
  }

  // ---- expressions --------------------------------------------------------

  var BIN_PREC = {
    "||": 1, "&&": 2, "|": 3, "^": 4, "&": 5,
    "==": 6, "!=": 6,
    "<": 7, ">": 7, "<=": 7, ">=": 7,
    "<<": 8, ">>": 8,
    "+": 9, "-": 9,
    "*": 10, "/": 10, "%": 10
  };

  function isTypeCtorName(v) {
    return v === "f32" || v === "u32" || v === "i32" || v === "bool" ||
      v === "vec2" || v === "vec3" || v === "vec4" || has(VEC_ALIAS, v);
  }

  function parseExpr(p, minPrec) {
    var left = parseUnary(p);
    for (;;) {
      var t = p.peek();
      if (!t || !has(BIN_PREC, t.v)) return left;
      var prec = BIN_PREC[t.v];
      if (prec < minPrec) return left;
      p.p++;
      var right = parseExpr(p, prec + 1);
      left = { k: "bin", op: t.v, l: left, r: right, line: t.line };
    }
  }

  function parseUnary(p) {
    var t = p.peek();
    if (!t) refuse("unexpected end of expression");
    if (t.v === "-" || t.v === "!" || t.v === "~") {
      p.p++;
      return { k: "unary", op: t.v, e: parseUnary(p), line: t.line };
    }
    if (t.v === "&") {
      p.p++;
      return { k: "addr", e: parseUnary(p), line: t.line };
    }
    if (t.v === "*") refuse("pointer dereference is not supported");
    if (t.v === "++" || t.v === "--") refuse("'" + t.v + "' is not valid in WGSL");
    return parsePostfix(p);
  }

  function parsePostfix(p) {
    var e = parsePrimary(p);
    for (;;) {
      if (p.at(".")) {
        p.p++;
        var m = p.expectId();
        e = { k: "member", base: e, name: m };
        continue;
      }
      if (p.at("[")) {
        p.p++;
        var idx = parseExpr(p, 0);
        p.expect("]");
        e = { k: "index", base: e, idx: idx };
        continue;
      }
      if (p.at("++") || p.at("--")) {
        refuse("'" + p.peek().v + "' is not valid WGSL (use 'x += 1' / 'x -= 1')");
      }
      if (p.at("(")) {
        refuse("calling a value (function pointer / closure) is not supported");
      }
      return e;
    }
  }

  function parsePrimary(p) {
    var t = p.peek();
    if (!t) refuse("unexpected end of expression");
    if (t.v === "(") {
      p.p++;
      var e = parseExpr(p, 0);
      p.expect(")");
      return { k: "paren", e: e };
    }
    if (t.k === "num") { p.p++; return { k: "lit", tok: t, line: t.line }; }
    if (t.k !== "id") refuse("line " + t.line + ": unexpected token '" + t.v + "' in expression");

    // arrayLength(&buf)
    if (t.v === "arrayLength") {
      p.p++;
      p.expect("(");
      p.expect("&");
      var nm = p.expectId();
      p.expect(")");
      return { k: "arrayLength", name: nm, line: t.line };
    }
    // bitcast<T>(expr)
    if (t.v === "bitcast") {
      p.p++;
      p.expect("<");
      var bt = parseType(p);
      p.expect(">");
      p.expect("(");
      var be = parseExpr(p, 0);
      p.expect(")");
      return { k: "bitcast", type: bt, e: be, line: t.line };
    }
    // matrices: named explicitly so the refusal says why
    if (/^mat[234]x[234]$/.test(t.v) || t.v === "mat2x2" || t.v.indexOf("mat") === 0) {
      refuse("matrix types are not supported");
    }
    // type constructors
    if (isTypeCtorName(t.v)) {
      var ty = null;
      var name = t.v;
      p.p++;
      if (name === "vec2" || name === "vec3" || name === "vec4") {
        if (p.eat("<")) {
          var el = p.expectId();
          p.expect(">");
          if (!SCALAR[el]) refuse("unsupported vector element type '" + el + "'");
          ty = name + "<" + el + ">";
        } else {
          ty = name + "<f32>";
        }
      } else if (has(VEC_ALIAS, name)) {
        ty = VEC_ALIAS[name];
      } else {
        ty = name;
      }
      p.expect("(");
      var args = [];
      if (!p.at(")")) {
        do { args.push(parseExpr(p, 0)); } while (p.eat(","));
      }
      p.expect(")");
      return { k: "ctor", type: ty, args: args, line: t.line };
    }
    // plain call?
    if (p.at("(", 1)) {
      p.p++;
      p.expect("(");
      var cargs = [];
      if (!p.at(")")) {
        do { cargs.push(parseExpr(p, 0)); } while (p.eat(","));
      }
      p.expect(")");
      return { k: "call", name: t.v, args: cargs, line: t.line };
    }
    p.p++;
    return { k: "ident", name: t.v, line: t.line };
  }

  /* ==========================================================================
   * 3. Types
   * ======================================================================== */

  function T(s, n) { return { s: s, n: n }; }
  var ABSTRACT_INT = T("abstract-int", 0);
  var ABSTRACT_FLOAT = T("abstract-float", 0);

  function typeFromString(str) {
    if (str === "f32") return T("f32", 1);
    if (str === "u32") return T("u32", 1);
    if (str === "i32") return T("i32", 1);
    if (str === "bool") return T("bool", 1);
    var m = /^vec([234])<(\w+)>$/.exec(str);
    if (m) return T(m[2], parseInt(m[1], 10));
    refuse("internal: unknown type string " + str);
  }

  function isAbstract(t) { return t && t.n === 0; }
  function isScalarType(t) { return t && t.n === 1; }
  function isFloatType(t) { return t && (t.s === "f32" || t.s === "abstract-float"); }
  function isIntType(t) { return t && (t.s === "i32" || t.s === "u32" || t.s === "abstract-int"); }
  function isBoolType(t) { return t && t.s === "bool"; }

  function glslType(t) {
    if (isAbstract(t)) t = T(t.s === "abstract-int" ? "i32" : "f32", 1);
    var base = t.s === "f32" ? "float" : t.s === "u32" ? "uint" : t.s === "i32" ? "int" : "bool";
    if (t.n === 1) return base;
    var pre = t.s === "f32" ? "vec" : t.s === "u32" ? "uvec" : t.s === "i32" ? "ivec" : "bvec";
    return pre + t.n;
  }

  function typeName(t) {
    if (isAbstract(t)) return t.s;
    if (t.n === 1) return t.s;
    return "vec" + t.n + "<" + t.s + ">";
  }

  function zeroLiteral(t) {
    if (isAbstract(t)) t = T("f32", 1);
    if (t.s === "bool") return t.n === 1 ? "false" : glslType(t) + "(false)";
    var z = t.s === "f32" ? "0.0" : t.s === "u32" ? "0u" : "0";
    return t.n === 1 ? z : glslType(t) + "(" + z + ")";
  }

  // Unify two operand types. Returns the result type, or null when the two
  // operands are not compatible (the caller refuses rather than guessing).
  function unify(a, b) {
    if (!a || !b) return null;
    if (isAbstract(a) && isAbstract(b)) {
      if (a.s === b.s) return a;
      return ABSTRACT_FLOAT;
    }
    if (isAbstract(a)) return concreteTo(b, a);
    if (isAbstract(b)) return concreteTo(a, b);
    if (a.s === b.s && a.n === b.n) return a;
    // vector with a scalar of the same element type
    if (a.n > 1 && b.n === 1 && a.s === b.s) return a;
    if (b.n > 1 && a.n === 1 && a.s === b.s) return b;
    // bool scalar with bvec
    if (a.s === "bool" && b.s === "bool") return a.n >= b.n ? a : b;
    return null;
  }

  // Make an abstract literal take the shape of its concrete sibling.
  function concreteTo(concrete, abs) {
    if (concrete.n === 1) return T(concrete.s, 1);
    return T(concrete.s, concrete.n);
  }

  /* ==========================================================================
   * 4. WGSL -> GLSL emitter
   * ======================================================================== */

  // Builtins that map straight onto a GLSL function of the same shape.
  var DIRECT_BUILTINS = {
    floor: { g: "floor", n: 1 }, ceil: { g: "ceil", n: 1 },
    trunc: { g: "trunc", n: 1 }, fract: { g: "fract", n: 1 },
    sqrt: { g: "sqrt", n: 1 }, inverseSqrt: { g: "inversesqrt", n: 1 },
    exp: { g: "exp", n: 1 }, exp2: { g: "exp2", n: 1 },
    log: { g: "log", n: 1 }, log2: { g: "log2", n: 1 },
    sin: { g: "sin", n: 1 }, cos: { g: "cos", n: 1 }, tan: { g: "tan", n: 1 },
    asin: { g: "asin", n: 1 }, acos: { g: "acos", n: 1 },
    sinh: { g: "sinh", n: 1 }, cosh: { g: "cosh", n: 1 }, tanh: { g: "tanh", n: 1 },
    asinh: { g: "asinh", n: 1 }, acosh: { g: "acosh", n: 1 }, atanh: { g: "atanh", n: 1 },
    degrees: { g: "degrees", n: 1 }, radians: { g: "radians", n: 1 },
    normalize: { g: "normalize", n: 1 },
    pow: { g: "pow", n: 2 }, atan2: { g: "atan", n: 2 },
    step: { g: "step", n: 2 }, smoothstep: { g: "smoothstep", n: 3 },
    reflect: { g: "reflect", n: 2 }, refract: { g: "refract", n: 3 },
    faceForward: { g: "faceForward", n: 3 }
  };

  var BAD_BUILTINS = {
    workgroupBarrier: "workgroupBarrier (intra-workgroup sync)",
    storageBarrier: "storageBarrier (intra-workgroup sync)",
    textureBarrier: "textureBarrier",
    atomicLoad: "atomics", atomicStore: "atomics", atomicAdd: "atomics",
    atomicSub: "atomics", atomicMax: "atomics", atomicMin: "atomics",
    atomicAnd: "atomics", atomicOr: "atomics", atomicXor: "atomics",
    atomicExchange: "atomics", atomicCompareExchangeWeak: "atomics",
    textureSample: "textures", textureSampleLevel: "textures",
    textureLoad: "textures", textureStore: "textures",
    textureDimensions: "textures", textureNumLevels: "textures",
    textureGather: "textures",
    pack4x8snorm: "pack/unpack builtins", pack4x8unorm: "pack/unpack builtins",
    pack2x16snorm: "pack/unpack builtins", pack2x16unorm: "pack/unpack builtins",
    pack2x16float: "pack/unpack builtins",
    unpack4x8snorm: "pack/unpack builtins", unpack4x8unorm: "pack/unpack builtins",
    unpack2x16snorm: "pack/unpack builtins", unpack2x16unorm: "pack/unpack builtins",
    unpack2x16float: "pack/unpack builtins",
    countOneBits: "bit-count builtins (no GLSL ES 3.0 equivalent)",
    countLeadingZeros: "bit-count builtins (no GLSL ES 3.0 equivalent)",
    countTrailingZeros: "bit-count builtins (no GLSL ES 3.0 equivalent)",
    firstLeadingBit: "bit-count builtins (no GLSL ES 3.0 equivalent)",
    firstTrailingBit: "bit-count builtins (no GLSL ES 3.0 equivalent)",
    reverseBits: "reverseBits (no GLSL ES 3.0 equivalent)",
    fma: "fma (no GLSL ES 3.0 equivalent)",
    modf: "modf", frexp: "frexp", ldexp: "ldexp",
    determinant: "matrices", transpose: "matrices",
    subgroupBallot: "subgroups", subgroupAdd: "subgroups",
    dpdx: "derivatives", dpdy: "derivatives", fwidth: "derivatives"
  };

  function Emitter(mod, outBinding) {
    this.mod = mod;
    this.out = outBinding;             // binding object this pass writes, or null
    this.scopes = [{}];
    this.lines = [];
    this.depth = 0;
    this.uid = 0;
    this.wroteOut = false;
    this.uses = { select: false, round: false };
    this.writeForms = {};      // buffer name -> [linear index forms of its stores]
    this.bindings = {};
    for (var i = 0; i < mod.bindings.length; i++) {
      this.bindings[mod.bindings[i].name] = mod.bindings[i];
    }
    this.constTypes = {};
  }

  Emitter.prototype.fail = function (node, msg) {
    refuse(msg + (node && node.line ? " (line " + node.line + ")" : ""));
  };

  Emitter.prototype.declare = function (name, info) {
    var gname = "_v" + (this.uid++) + "_" + name.replace(/[^A-Za-z0-9_]/g, "_");
    this.scopes[this.scopes.length - 1][name] = {
      g: gname, type: info.type, mutable: info.mutable !== false,
      form: info.form || null, const: !!info.const
    };
    return gname;
  };

  Emitter.prototype.lookup = function (name) {
    for (var i = this.scopes.length - 1; i >= 0; i--) {
      if (has(this.scopes[i], name)) return this.scopes[i][name];
    }
    return null;
  };

  Emitter.prototype.push = function () { this.scopes.push({}); };
  Emitter.prototype.pop = function () { this.scopes.pop(); };

  Emitter.prototype.emit = function (s) {
    var pad = "";
    for (var i = 0; i < this.depth; i++) pad += "  ";
    this.lines.push(pad + s);
  };

  // ---- type helpers -------------------------------------------------------

  Emitter.prototype.expectType = function (node, t, what) {
    if (!t) this.fail(node, "cannot determine the type of " + what);
    return t;
  };

  /* Syntactic/linear analysis of a store index: can we prove that the element
     this expression targets is exactly the invocation's own element?

     Everything is reduced to a linear form  a*gid.x + b*gid.y + c*gid.z + k.
     The WebGL2 engine runs invocation i for element i, so a write is only
     stored when the form provably equals i for the dispatch actually being
     run - that check happens in _dispatchTranslated, where gid's extents
     (workgroup size * dispatch size) are known. Anything that does not reduce
     to a linear form is refused here rather than guessed at. */
  Emitter.prototype.indexForm = function (node) {
    var f = this.indexFormRaw(node);
    if (!f) return null;
    // NaN/Inf safety: only finite integer coefficients are meaningful.
    if (!isFinite(f.x) || !isFinite(f.y) || !isFinite(f.z) || !isFinite(f.k)) return null;
    return f;
  };

  Emitter.prototype.indexFormRaw = function (node) {
    if (!node) return null;
    switch (node.k) {
      case "paren": return this.indexFormRaw(node.e);
      case "lit":
        if (node.tok.isFloat) return null;
        return { x: 0, y: 0, z: 0, k: node.tok.value };
      case "member": {
        var b = node.base;
        if (b.k === "ident" && b.name === this.mod.entry.gidName) {
          if (node.name === "x") return { x: 1, y: 0, z: 0, k: 0 };
          if (node.name === "y") return { x: 0, y: 1, z: 0, k: 0 };
          if (node.name === "z") return { x: 0, y: 0, z: 1, k: 0 };
        }
        return null;
      }
      case "ident": {
        var sym = this.lookup(node.name);
        if (sym && sym.form) return { x: sym.form.x, y: sym.form.y, z: sym.form.z, k: sym.form.k };
        if (has(this.constValues, node.name)) return { x: 0, y: 0, z: 0, k: this.constValues[node.name] };
        return null;
      }
      case "ctor":
        if (node.args.length !== 1) return null;
        if (node.type !== "u32" && node.type !== "i32") return null;
        return this.indexFormRaw(node.args[0]);
      case "unary": {
        if (node.op !== "-") return null;
        var e = this.indexFormRaw(node.e);
        if (!e) return null;
        return { x: -e.x, y: -e.y, z: -e.z, k: -e.k };
      }
      case "bin": {
        if (node.op === "+" || node.op === "-") {
          var l = this.indexFormRaw(node.l), r = this.indexFormRaw(node.r);
          if (!l || !r) return null;
          var s = node.op === "+" ? 1 : -1;
          return { x: l.x + s * r.x, y: l.y + s * r.y, z: l.z + s * r.z, k: l.k + s * r.k };
        }
        if (node.op === "*") {
          var l2 = this.indexFormRaw(node.l), r2 = this.indexFormRaw(node.r);
          if (!l2 || !r2) return null;
          var lc = (l2.x === 0 && l2.y === 0 && l2.z === 0);
          var rc = (r2.x === 0 && r2.y === 0 && r2.z === 0);
          if (lc) return { x: r2.x * l2.k, y: r2.y * l2.k, z: r2.z * l2.k, k: r2.k * l2.k };
          if (rc) return { x: l2.x * r2.k, y: l2.y * r2.k, z: l2.z * r2.k, k: l2.k * r2.k };
          return null;
        }
        return null;
      }
      default: return null;
    }
  };

  /* Does this linear form equal the invocation's own linear index for a
     dispatch whose global_invocation_id extents are nx/ny/nz? */
  function formIsIdentity(form, nx, ny, nz) {
    if (!form) return false;
    if (form.k !== 0) return false;
    if (form.x !== 1) return false;
    if (ny > 1 && form.y !== nx) return false;
    if (nz > 1 && form.z !== nx * ny) return false;
    return true;
  }

  // ---- expressions --------------------------------------------------------

  Emitter.prototype.expr = function (node, hint) {
    var r = this.exprRaw(node, hint);
    this.expectType(node, r.type, "expression '" + node.k + "'");
    return r;
  };

  Emitter.prototype.exprRaw = function (node, hint) {
    switch (node.k) {
      case "paren": return this.exprRaw(node.e, hint);
      case "lit": return this.literal(node, hint);
      case "ident": return this.ident(node);
      case "member": return this.member(node, hint);
      case "index": return this.index(node);
      case "call": return this.call(node, hint);
      case "ctor": return this.ctor(node);
      case "bitcast": return this.bitcast(node);
      case "arrayLength": return this.arrayLength(node);
      case "unary": return this.unary(node, hint);
      case "bin": return this.binary(node, hint);
      case "addr": this.fail(node, "address-of expressions are only supported inside arrayLength(&buffer)");
        return null;
      default: this.fail(node, "unsupported expression");
        return null;
    }
  };

  Emitter.prototype.literal = function (node, hint) {
    var t = node.tok;
    // A suffixed literal (2u, 3i, 4f) has a concrete type of its own; an
    // unsuffixed one is an abstract literal that takes the type of whatever it
    // is combined with (WGSL abstract-int / abstract-float).
    var nat;
    if (t.suffix === "u") nat = T("u32", 1);
    else if (t.suffix === "i") nat = T("i32", 1);
    else if (t.suffix === "f" || t.isFloat) nat = ABSTRACT_FLOAT;
    else nat = ABSTRACT_INT;

    var target = hint && hint.n !== 0 ? hint : null;
    var emitType = nat;
    if (target) {
      if (isAbstract(nat)) {
        if (target.s === "bool") this.fail(node, "a numeric literal cannot be used as bool");
        if (nat.s === "abstract-float" && (target.s === "u32" || target.s === "i32")) {
          this.fail(node, "a float literal cannot be used where " + typeName(target) + " is required");
        }
        emitType = T(target.s, 1);
      }
    }
    if (isAbstract(emitType)) emitType = T(emitType.s === "abstract-int" ? "i32" : "f32", 1);

    var code;
    if (t.isFloat) {
      code = t.text;
      if (code.indexOf(".") < 0 && code.indexOf("e") < 0 && code.indexOf("E") < 0) code += ".0";
      if (emitType.s === "i32" || emitType.s === "u32") {
        this.fail(node, "a float literal cannot be used where " + typeName(emitType) + " is required");
      }
    } else {
      code = t.text;
      if (emitType.s === "u32") code += "u";
      else if (emitType.s === "f32") code = (t.text.indexOf("0x") === 0 ? "float(" + t.text + ")" : t.text + ".0");
    }
    return this.shape(node, code, emitType, target);
  };

  // Splat a scalar into a vector when the expected shape is a vector.
  Emitter.prototype.shape = function (node, code, type, target) {
    if (target && target.n > 1 && type.n === 1) {
      return { code: glslType(target) + "(" + code + ")", type: target };
    }
    return { code: code, type: type };
  };

  Emitter.prototype.ident = function (node) {
    if (node.name === this.mod.entry.gidName) {
      this.fail(node, "the global_invocation_id parameter can only be used as " +
        this.mod.entry.gidName + ".x / .y / .z");
    }
    var sym = this.lookup(node.name);
    if (sym) return { code: sym.g, type: sym.type };
    if (has(this.bindings, node.name)) {
      this.fail(node, "storage buffer '" + node.name + "' must be indexed, e.g. " + node.name + "[i]");
    }
    for (var i = 0; i < this.mod.consts.length; i++) {
      if (this.mod.consts[i].name === node.name) {
        return { code: "_c_" + node.name, type: this.constTypes[node.name] };
      }
    }
    this.fail(node, "unknown identifier '" + node.name +
      "' (user functions, module-scope var and non-const globals are not supported)");
    return null;
  };

  Emitter.prototype.member = function (node, hint) {
    var base = node.base;
    if (base.k === "ident" && base.name === this.mod.entry.gidName) {
      if (node.name === "x") return { code: "_wg_gid_x(i)", type: T("u32", 1) };
      if (node.name === "y") return { code: "_wg_gid_y(i)", type: T("u32", 1) };
      if (node.name === "z") return { code: "_wg_gid_z(i)", type: T("u32", 1) };
      this.fail(node, "only .x, .y and .z of the global_invocation_id are supported");
    }
    var b = this.expr(base, null);
    if (b.type.n < 2) this.fail(node, "member access on a non-vector value");
    var sw = node.name;
    var map = { x: 0, y: 1, z: 2, w: 3, r: 0, g: 1, b: 2, a: 3 };
    var comps = [];
    for (var i = 0; i < sw.length; i++) {
      var c = sw.charAt(i);
      if (!has(map, c)) this.fail(node, "unsupported vector member '." + sw + "'");
      var idx = map[c];
      if (b.type.n >= 2 && /[rgba]/.test(sw) && /[xyzw]/.test(sw)) {
        this.fail(node, "mixed swizzle sets are not valid");
      }
      if (idx >= b.type.n) this.fail(node, "swizzle '." + sw + "' reads past the end of " + typeName(b.type));
      comps.push(idx);
    }
    var sel = "";
    for (var k = 0; k < comps.length; k++) sel += "xyzw".charAt(comps[k]);
    var rt = comps.length === 1 ? T(b.type.s, 1) : T(b.type.s, comps.length);
    return { code: b.code + "." + sel, type: rt };
  };

  Emitter.prototype.index = function (node) {
    var base = node.base;
    if (base.k === "ident" && has(this.bindings, base.name)) {
      var b = this.bindings[base.name];
      var idx = this.expr(node.idx, T("u32", 1));
      if (idx.type.s !== "u32" && idx.type.s !== "i32") {
        this.fail(node, "a storage buffer index must be an integer, found " + typeName(idx.type));
      }
      if (b === this.out && this.wroteOut) {
        this.fail(node, "storage buffer '" + b.name + "' is read after being written in the same " +
          "dispatch - the WebGL2 engine cannot order those accesses, so this shader is refused");
      }
      // Gather reads may use any integer index: uint() of a negative i32 wraps
      // to a huge value, which the accessor's bounds check turns into 0.
      return { code: this.accessor(b, "uint(" + idx.code + ")"), type: T(b.elem, 1) };
    }
    var bb = this.expr(base, null);
    if (bb.type.n < 2) this.fail(node, "indexing a non-vector value");
    var ix = this.expr(node.idx, T("i32", 1));
    if (ix.type.s !== "i32" && ix.type.s !== "u32") this.fail(node, "a vector index must be an integer");
    return { code: bb.code + "[" + ix.code + "]", type: T(bb.type.s, 1) };
  };

  Emitter.prototype.accessor = function (b, idxCode) {
    return "_rd_" + b.elem + "_" + b.group + "_" + b.binding + "(" + idxCode + ")";
  };

  Emitter.prototype.arrayLength = function (node) {
    var b = this.bindings[node.name];
    if (!b) this.fail(node, "arrayLength(&" + node.name + ") does not refer to a storage buffer");
    return { code: "uint(_n_" + b.group + "_" + b.binding + ")", type: T("u32", 1) };
  };

  Emitter.prototype.bitcast = function (node) {
    var from = this.expr(node.e, null);
    var to = typeFromString(node.type);
    if (from.type.n !== to.n) this.fail(node, "bitcast must keep the same shape");
    if (from.type.s === "bool" || to.s === "bool") this.fail(node, "bitcast to/from bool is not supported");
    var fn = null;
    if (to.s === "u32" && from.type.s === "f32") fn = "floatBitsToUint";
    else if (to.s === "f32" && from.type.s === "u32") fn = "uintBitsToFloat";
    else if (to.s === "i32" && from.type.s === "f32") fn = "floatBitsToInt";
    else if (to.s === "f32" && from.type.s === "i32") fn = "intBitsToFloat";
    else if (to.s === "u32" && from.type.s === "i32") fn = "uint";
    else if (to.s === "i32" && from.type.s === "u32") fn = "int";
    else this.fail(node, "bitcast from " + typeName(from.type) + " to " + typeName(to) + " is not supported");
    return { code: fn + "(" + from.code + ")", type: to };
  };

  Emitter.prototype.unary = function (node, hint) {
    var e = this.expr(node.e, hint);
    if (node.op === "!") {
      if (e.type.s !== "bool") this.fail(node, "'!' needs a boolean operand");
      return { code: "!" + this.paren(e.code), type: e.type };
    }
    if (node.op === "~") {
      if (!isIntType(e.type)) this.fail(node, "'~' needs an integer operand");
      return { code: "~" + this.paren(e.code), type: e.type };
    }
    if (e.type.s === "u32") this.fail(node, "unary '-' is not defined for u32 in WGSL");
    if (e.type.s === "bool") this.fail(node, "unary '-' needs a numeric operand");
    return { code: "-" + this.paren(e.code), type: e.type };
  };

  Emitter.prototype.paren = function (code) {
    return "((" + code + "))";
  };

  // Vector comparisons are component-wise in WGSL but produce a single bool in
  // GLSL, so they must become lessThan()/equal()/... (which return bvecN).
  var VEC_CMP = {
    "<": "lessThan", ">": "greaterThan", "<=": "lessThanEqual",
    ">=": "greaterThanEqual", "==": "equal", "!=": "notEqual"
  };

  Emitter.prototype.binary = function (node, hint) {
    var op = node.op;
    var L = this.exprRaw(node.l, null);
    var R = this.exprRaw(node.r, null);
    var t = unify(L.type, R.type);
    if (!t) {
      this.fail(node, "operator '" + op + "' on incompatible types " +
        typeName(L.type) + " and " + typeName(R.type));
    }
    // Re-emit abstract literals with the concrete shape they must take.
    if (isAbstract(L.type) && !isAbstract(t)) L = this.exprRaw(node.l, t);
    if (isAbstract(R.type) && !isAbstract(t)) R = this.exprRaw(node.r, t);

    var cmp = has(VEC_CMP, op);
    if (cmp) {
      var resType = t.n > 1 ? T("bool", t.n) : T("bool", 1);
      if (t.n > 1) {
        if (t.s === "bool") this.fail(node, "comparison of boolean vectors is not supported");
        return { code: VEC_CMP[op] + "(" + L.code + ", " + R.code + ")", type: resType };
      }
      if (t.s === "bool" && op !== "==" && op !== "!=") {
        this.fail(node, "ordering comparison of booleans");
      }
      return { code: "(" + L.code + " " + op + " " + R.code + ")", type: resType };
    }

    if (op === "&&" || op === "||") {
      if (t.s !== "bool" || t.n !== 1) this.fail(node, "'" + op + "' needs scalar boolean operands");
      return { code: "(" + L.code + " " + op + " " + R.code + ")", type: T("bool", 1) };
    }
    if (op === "&" || op === "|" || op === "^" || op === "<<" || op === ">>") {
      if (!isIntType(t)) this.fail(node, "'" + op + "' needs integer operands");
      return { code: "(" + L.code + " " + op + " " + R.code + ")", type: t };
    }
    if (op === "%") {
      if (isFloatType(t)) return { code: "mod(" + L.code + ", " + R.code + ")", type: t };
      if (!isIntType(t)) this.fail(node, "'%' needs numeric operands");
      return { code: "(" + L.code + " % " + R.code + ")", type: t };
    }
    if (isBoolType(t)) this.fail(node, "arithmetic on booleans");
    return { code: "(" + L.code + " " + op + " " + R.code + ")", type: t };
  };

  Emitter.prototype.ctor = function (node) {
    var to = typeFromString(node.type);
    var args = [], i;
    var compType = to.n > 1 ? T(to.s, 1) : to;
    for (i = 0; i < node.args.length; i++) {
      args.push(this.expr(node.args[i], to.n > 1 ? compType : to));
    }
    if (node.args.length === 0) this.fail(node, "zero-argument constructors are not supported");
    if (to.n === 1) {
      if (node.args.length !== 1) this.fail(node, typeName(to) + "() takes exactly one argument");
      if (args[0].type.n > 1) this.fail(node, "cannot convert " + typeName(args[0].type) + " to " + typeName(to));
      if (args[0].type.s === "bool" && to.s !== "bool") {
        return { code: glslType(to) + "(" + args[0].code + ")", type: to };
      }
      return { code: glslType(to) + "(" + args[0].code + ")", type: to };
    }
    // vector construction: GLSL accepts scalars, same-size vectors and mixes
    var total = 0;
    for (i = 0; i < args.length; i++) total += args[i].type.n;
    if (args.length > 1 && total !== to.n) {
      this.fail(node, typeName(to) + "() has " + total + " components from " + args.length + " arguments");
    }
    var parts = [];
    for (i = 0; i < args.length; i++) parts.push(args[i].code);
    return { code: glslType(to) + "(" + parts.join(", ") + ")", type: to };
  };

  Emitter.prototype.call = function (node, hint) {
    var name = node.name;

    if (has(BAD_BUILTINS, name)) {
      this.fail(node, "unsupported WGSL construct: " + name + " (" + BAD_BUILTINS[name] + " is not supported)");
    }
    if (name === "select") {
      if (node.args.length !== 3) this.fail(node, "select() takes 3 arguments");
      var f = this.expr(node.args[0], hint);
      var t2 = this.expr(node.args[1], hint);
      var c = this.expr(node.args[2], null);
      var rt = unify(f.type, t2.type);
      if (!rt) this.fail(node, "select() arguments must have the same type");
      if (c.type.s !== "bool") this.fail(node, "select() condition must be boolean");
      if (c.type.n !== rt.n) {
        this.fail(node, "select() condition must be " + (rt.n === 1 ? "a scalar bool" : "a vec" + rt.n + "<bool>"));
      }
      this.uses.select = true;
      return { code: "_wg_sel(" + f.code + ", " + t2.code + ", " + c.code + ")", type: rt };
    }
    if (name === "abs") {
      var ax = this.expr(node.args[0], hint);
      if (!isIntType(ax.type) && !isFloatType(ax.type)) this.fail(node, "abs() needs a numeric argument");
      if (ax.type.s === "u32" || ax.type.s === "abstract-int") return { code: ax.code, type: ax.type };
      return { code: "abs(" + ax.code + ")", type: ax.type };
    }
    if (name === "min" || name === "max" || name === "clamp") {
      var want = name === "clamp" ? 3 : 2;
      if (node.args.length !== want) this.fail(node, name + "() takes " + want + " arguments");
      var a0 = this.expr(node.args[0], hint);
      var parts = [a0.code], tt = a0.type;
      for (var k = 1; k < node.args.length; k++) {
        var ak = this.expr(node.args[k], tt);
        tt = unify(tt, ak.type);
        if (!tt) this.fail(node, name + "() arguments must have matching types");
        parts.push(ak.code);
      }
      if (!isIntType(tt) && !isFloatType(tt)) this.fail(node, name + "() needs numeric arguments");
      return { code: name + "(" + parts.join(", ") + ")", type: tt };
    }
    if (name === "sign") {
      var sg = this.expr(node.args[0], hint);
      if (sg.type.s === "u32" || sg.type.s === "abstract-int") {
        // sign(u) is exactly min(u, 1) for unsigned values
        return { code: this.paren("min(" + sg.code + ", " + (sg.type.n === 1 ? "1u" : glslType(sg.type) + "(1u)") + ")"), type: sg.type };
      }
      if (!isFloatType(sg.type) && sg.type.s !== "i32") this.fail(node, "sign() needs a numeric argument");
      return { code: "sign(" + sg.code + ")", type: sg.type };
    }
    if (name === "mix") {
      if (node.args.length !== 3) this.fail(node, "mix() takes 3 arguments");
      var m0 = this.expr(node.args[0], hint);
      var m1 = this.expr(node.args[1], m0.type);
      var m2 = this.expr(node.args[2], m0.type);
      if (!isFloatType(m2.type) && m2.type.s !== "f32") this.fail(node, "mix() interpolation must be floating point");
      var mt = unify(m0.type, m1.type);
      if (!mt || !isFloatType(mt)) this.fail(node, "mix() needs floating point arguments");
      return { code: "mix(" + m0.code + ", " + m1.code + ", " + m2.code + ")", type: mt };
    }
    if (name === "round") {
      var rr = this.expr(node.args[0], hint);
      if (!isFloatType(rr.type)) this.fail(node, "round() needs a floating point argument");
      this.uses.round = true;
      return { code: "_wg_round(" + rr.code + ")", type: rr.type };
    }
    if (name === "length" || name === "distance" || name === "dot" || name === "cross") {
      var c0 = this.expr(node.args[0], hint);
      if (name === "length") {
        if (c0.type.n < 2) this.fail(node, "length() needs a vector");
        return { code: "length(" + c0.code + ")", type: T("f32", 1) };
      }
      var c1 = this.expr(node.args[1], c0.type);
      if (c0.type.n < 2) this.fail(node, name + "() needs vectors");
      if (name === "cross") {
        if (c0.type.n !== 3) this.fail(node, "cross() needs vec3");
        return { code: "cross(" + c0.code + ", " + c1.code + ")", type: c0.type };
      }
      if (name === "dot") return { code: "dot(" + c0.code + ", " + c1.code + ")", type: T("f32", 1) };
      return { code: "distance(" + c0.code + ", " + c1.code + ")", type: T("f32", 1) };
    }
    if (name === "any" || name === "all") {
      var b0 = this.expr(node.args[0], hint);
      if (b0.type.s !== "bool") this.fail(node, name + "() needs a boolean argument");
      if (b0.type.n === 1) return { code: b0.code, type: T("bool", 1) };
      return { code: name + "(" + b0.code + ")", type: T("bool", 1) };
    }
    if (name === "atan") {
      if (node.args.length === 1) {
        var a1 = this.expr(node.args[0], hint);
        if (!isFloatType(a1.type)) this.fail(node, "atan() needs floating point");
        return { code: "atan(" + a1.code + ")", type: a1.type };
      }
      if (node.args.length === 2) {
        var a2 = this.expr(node.args[0], hint);
        var a3 = this.expr(node.args[1], a2.type);
        return { code: "atan(" + a2.code + ", " + a3.code + ")", type: a2.type };
      }
      this.fail(node, "atan() takes 1 or 2 arguments");
    }
    if (has(DIRECT_BUILTINS, name)) {
      var d = DIRECT_BUILTINS[name];
      if (node.args.length !== d.n) this.fail(node, name + "() takes " + d.n + " arguments");
      var first = this.expr(node.args[0], hint);
      if (!isFloatType(first.type)) this.fail(node, name + "() needs floating point arguments");
      var codes = [first.code];
      for (var q = 1; q < node.args.length; q++) {
        var aq = this.expr(node.args[q], q === 2 ? first.type : first.type);
        codes.push(aq.code);
      }
      var resT = first.type;
      if (name === "faceForward") resT = first.type;
      return { code: d.g + "(" + codes.join(", ") + ")", type: resT };
    }
    this.fail(node, "unsupported WGSL builtin '" + name + "()' - refused rather than guessed");
    return null;
  };

  // ---- statements ---------------------------------------------------------

  Emitter.prototype.stmt = function (s) {
    switch (s.k) {
      case "block":
        this.emit("{");
        this.depth++;
        this.push();
        for (var i = 0; i < s.stmts.length; i++) this.stmt(s.stmts[i]);
        this.pop();
        this.depth--;
        this.emit("}");
        return;
      case "decl": return this.decl(s);
      case "assign": return this.assign(s);
      case "exprStmt": {
        var e = this.expr(s.e, null);
        this.emit(e.code + ";");
        return;
      }
      case "if": {
        var c = this.expr(s.cond, null);
        if (c.type.s !== "bool") this.fail(s.cond, "an if condition must be boolean");
        this.emit("if (" + c.code + ") {");
        this.depth++;
        this.push();
        this.subStatement(s.then);
        this.pop();
        this.depth--;
        if (s.els) {
          this.emit("} else {");
          this.depth++;
          this.push();
          this.subStatement(s.els);
          this.pop();
          this.depth--;
        }
        this.emit("}");
        return;
      }
      case "for": {
        this.push();
        var init = "";
        if (s.init) init = this.headerStmt(s.init);
        var cond = "";
        if (s.cond) {
          var cc = this.expr(s.cond, null);
          if (cc.type.s !== "bool" || cc.type.n !== 1) this.fail(s.cond, "a for condition must be a scalar bool");
          cond = cc.code;
        }
        var cont = "";
        if (s.cont) cont = this.headerStmt(s.cont);
        this.emit("for (" + init + "; " + cond + "; " + cont + ") {");
        this.depth++;
        this.subStatement(s.body);
        this.depth--;
        this.emit("}");
        this.pop();
        return;
      }
      case "while": {
        var wc = this.expr(s.cond, null);
        if (wc.type.s !== "bool" || wc.type.n !== 1) this.fail(s.cond, "a while condition must be a scalar bool");
        this.emit("while (" + wc.code + ") {");
        this.depth++;
        this.push();
        this.subStatement(s.body);
        this.pop();
        this.depth--;
        this.emit("}");
        return;
      }
      case "break": this.emit("break;"); return;
      case "continue": this.emit("continue;"); return;
      case "return":
        if (s.e) this.fail(s, "a @compute entry point cannot return a value");
        this.emit("return;");
        return;
      default: this.fail(s, "unsupported statement '" + s.k + "'");
    }
  };

  // A statement used as a for/if body: emit it inside the braces we already
  // opened, so a nested block does not double up.
  Emitter.prototype.subStatement = function (s) {
    if (s.k === "block") {
      for (var i = 0; i < s.stmts.length; i++) this.stmt(s.stmts[i]);
      return;
    }
    this.stmt(s);
  };

  Emitter.prototype.headerStmt = function (s) {
    // Declarations and assignments only; they become GLSL without a trailing ;
    var save = this.lines;
    this.lines = [];
    this.stmt(s);
    var out = this.lines.join(" ").replace(/;\s*$/, "");
    this.lines = save;
    return out;
  };

  Emitter.prototype.decl = function (s) {
    var initType = null;
    if (s.init) {
      var declared = s.type ? typeFromString(s.type) : null;
      var r = this.expr(s.init, declared);
      initType = r.type;
      if (declared) {
        var u = unify(declared, r.type);
        if (!u) this.fail(s, "cannot initialise " + s.type + " from " + typeName(r.type));
        if (typeName(u) !== typeName(declared) && !(isAbstract(r.type))) {
          this.fail(s, "cannot initialise " + s.type + " from " + typeName(r.type));
        }
      }
      var vt = declared || (isAbstract(r.type) ? T(r.type.s === "abstract-int" ? "i32" : "f32", 1) : r.type);
      // Remember the linear index form so `let idx = gid.y*W + gid.x;
      // out[idx] = ...` stays provably element-local.
      var form = this.indexForm(s.init);
      var g = this.declare(s.name, { type: vt, form: form, mutable: s.kind === "var" });
      this.emit(glslType(vt) + " " + g + " = " + r.code + ";");
      return;
    }
    if (!s.type) this.fail(s, "'" + s.kind + " " + s.name + "' needs a type or an initialiser");
    var t = typeFromString(s.type);
    var gg = this.declare(s.name, { type: t, invIndex: false, mutable: s.kind === "var" });
    this.emit(glslType(t) + " " + gg + " = " + zeroLiteral(t) + ";");
  };

  Emitter.prototype.assign = function (s) {
    var target = s.target;
    var op = s.op;

    // local variable / swizzle / vector index
    if (target.k === "ident" || target.k === "member" || target.k === "index") {
      var isBuf = target.k === "index" && target.base.k === "ident" && has(this.bindings, target.base.name);
      if (!isBuf) {
        var info = this.lvalue(target);
        var rhs2 = this.expr(s.value, info.type);
        if (info.sym) info.sym.form = null;      // a mutated var is no longer index-like
        if (op === "=") {
          this.emit(info.code + " = " + rhs2.code + ";");
          return;
        }
        var bin = op.charAt(0);
        if (bin === "%" && isFloatType(info.type)) {
          this.emit(info.code + " = mod(" + info.code + ", " + rhs2.code + ");");
        } else {
          this.emit(info.code + " = (" + info.code + " " + bin + " " + rhs2.code + ");");
        }
        return;
      }

      // storage buffer element
      var b = this.bindings[target.base.name];
      var form = this.indexForm(target.idx);
      if (!form) {
        this.fail(s, "cannot prove that the index used to write '" + b.name + "' is the current " +
          "invocation's own element (the WebGL2 engine stores element i back into element i, so store " +
          "indices must be a linear combination of the global_invocation_id components); " +
          "this shader is refused rather than mis-computed");
      }
      if (form.x === 0 && form.y === 0 && form.z === 0) {
        this.fail(s, "a write to '" + b.name + "' at a fixed index (no global_invocation_id term) " +
          "cannot be expressed on the WebGL2 engine; this shader is refused");
      }
      if (!has(this.writeForms, b.name)) this.writeForms[b.name] = [];
      this.writeForms[b.name].push(form);
      if (b.access !== "read_write") {
        this.fail(s, "assignment to '" + b.name + "', which is declared var<storage, read>");
      }
      if (b !== this.out) {
        // A different writable buffer is handled in its own pass. The value is
        // discarded here on purpose; nothing is mis-computed because every
        // pass re-runs the kernel with the same pre-dispatch inputs.
        var v = this.expr(s.value, T(b.elem, 1));
        this.emit("/* write to " + b.name + " is performed in its own pass */");
        this.emit(v.code + ";");
        return;
      }
      var et = T(b.elem, 1);
      var rhs;
      if (op === "=") {
        rhs = this.expr(s.value, et);
      } else {
        var cur = { code: this.accessor(b, "uint(i)"), type: et };
        var rv = this.expr(s.value, et);
        var binop = op.charAt(0);
        if (binop === "%" && b.elem === "f32") {
          rhs = { code: "mod(" + cur.code + ", " + rv.code + ")", type: et };
        } else {
          rhs = { code: "(" + cur.code + " " + binop + " " + rv.code + ")", type: et };
        }
      }
      var conv = this.store(b, rhs);
      this.emit("io.x = " + conv + ";");
      this.wroteOut = true;
      return;
    }
    this.fail(s, "unsupported assignment target");
  };

  Emitter.prototype.store = function (b, val) {
    if (b.elem === "f32") return val.code;
    if (b.elem === "u32") return "uintBitsToFloat(" + val.code + ")";
    return "intBitsToFloat(" + val.code + ")";
  };

  Emitter.prototype.lvalue = function (node) {
    if (node.k === "ident") {
      var sym = this.lookup(node.name);
      if (!sym) this.fail(node, "assignment to unknown identifier '" + node.name + "'");
      if (!sym.mutable) this.fail(node, "'" + node.name + "' is declared with let/const and cannot be reassigned");
      return { code: sym.g, type: sym.type, sym: sym };
    }
    if (node.k === "member") {
      var b = this.lvalue(node.base);
      if (b.type.n < 2) this.fail(node, "member assignment on a non-vector");
      var map = { x: 0, y: 1, z: 2, w: 3, r: 0, g: 1, b: 2, a: 3 };
      var sel = "", n = 0;
      for (var i = 0; i < node.name.length; i++) {
        var c = node.name.charAt(i);
        if (!has(map, c)) this.fail(node, "unsupported swizzle '." + node.name + "'");
        sel += "xyzw".charAt(map[c]);
        n++;
      }
      return { code: b.code + "." + sel, type: n === 1 ? T(b.type.s, 1) : T(b.type.s, n) };
    }
    if (node.k === "index") {
      var bb = this.lvalue(node.base);
      if (bb.type.n < 2) this.fail(node, "index assignment on a non-vector");
      var ix = this.expr(node.idx, T("i32", 1));
      return { code: bb.code + "[" + ix.code + "]", type: T(bb.type.s, 1) };
    }
    this.fail(node, "unsupported assignment target");
    return null;
  };

  /* ==========================================================================
   * 5. GLSL assembly
   * ======================================================================== */

  function bindingSuffix(b) { return b.group + "_" + b.binding; }

  function selectHelpers() {
    var out = [];
    var types = [
      { k: "float", c: "bool", b: "float" },
      { k: "vec2", c: "bool", b: "float" },
      { k: "vec3", c: "bool", b: "float" },
      { k: "vec4", c: "bool", b: "float" },
      { k: "vec2", c: "bvec2", b: "float" },
      { k: "vec3", c: "bvec3", b: "float" },
      { k: "vec4", c: "bvec4", b: "float" },
      { k: "int", c: "bool", b: "int" },
      { k: "ivec2", c: "bool", b: "int" },
      { k: "ivec3", c: "bool", b: "int" },
      { k: "ivec4", c: "bool", b: "int" },
      { k: "ivec2", c: "bvec2", b: "int" },
      { k: "ivec3", c: "bvec3", b: "int" },
      { k: "ivec4", c: "bvec4", b: "int" },
      { k: "uint", c: "bool", b: "uint" },
      { k: "uvec2", c: "bool", b: "uint" },
      { k: "uvec3", c: "bool", b: "uint" },
      { k: "uvec4", c: "bool", b: "uint" },
      { k: "uvec2", c: "bvec2", b: "uint" },
      { k: "uvec3", c: "bvec3", b: "uint" },
      { k: "uvec4", c: "bvec4", b: "uint" },
      { k: "bool", c: "bool", b: "bool" },
      { k: "bvec2", c: "bvec2", b: "bool" },
      { k: "bvec3", c: "bvec3", b: "bool" },
      { k: "bvec4", c: "bvec4", b: "bool" }
    ];
    out.push("// select(a, b, cond) -> componentwise; WGSL semantics exactly");
    for (var i = 0; i < types.length; i++) {
      var t = types[i];
      var isVec = t.k !== "float" && t.k !== "int" && t.k !== "uint" && t.k !== "bool";
      if (!isVec) {
        out.push(t.k + " _wg_sel(" + t.k + " a, " + t.k + " b, " + t.c + " c) { return c ? b : a; }");
      } else {
        var n = parseInt(t.k.charAt(t.k.length - 1), 10);
        var comps = "", cparts = "";
        var lanes = "xyzw";
        for (var j = 0; j < n; j++) {
          comps += (j ? ", " : "") + t.b + "(" + t.c.charAt(0) + (t.c.indexOf("vec") >= 0 ? "." + lanes.charAt(j) : "") + ")";
          cparts += (j ? " : " : " : ");
        }
        // build componentwise with explicit condition per lane
        var args = [];
        for (var k2 = 0; k2 < n; k2++) {
          var cond = t.c === "bool" ? "c" : ("c." + lanes.charAt(k2));
          args.push("(" + cond + " ? b." + lanes.charAt(k2) + " : a." + lanes.charAt(k2) + ")");
        }
        out.push(t.k + " _wg_sel(" + t.k + " a, " + t.k + " b, " + t.c + " c) { return " +
          t.k + "(" + args.join(", ") + "); }");
      }
    }
    return out.join("\n");
  }

  function roundHelpers() {
    return [
      "// WGSL round(): ties away from zero (GLSL round() is implementation defined)",
      "float _wg_round(float x) { return sign(x) * floor(abs(x) + 0.5); }",
      "vec2 _wg_round(vec2 x) { return sign(x) * floor(abs(x) + vec2(0.5)); }",
      "vec3 _wg_round(vec3 x) { return sign(x) * floor(abs(x) + vec3(0.5)); }",
      "vec4 _wg_round(vec4 x) { return sign(x) * floor(abs(x) + vec4(0.5)); }"
    ].join("\n");
  }

  function buildCommonGlsl(mod) {
    var out = [];
    out.push("// --- wgpu-shim generated declarations -------------------------------");
    out.push("uniform int u_nx;   // global_invocation_id extents = workgroup_size * dispatch");
    out.push("uniform int u_ny;");
    out.push("uniform int u_nz;");
    out.push("uint _wg_gid_x(int i) { return uint(i) % uint(u_nx); }");
    out.push("uint _wg_gid_y(int i) { return (uint(i) / uint(u_nx)) % uint(u_ny); }");
    out.push("uint _wg_gid_z(int i) { return uint(i) / (uint(u_nx) * uint(u_ny)); }");
    for (var i = 0; i < mod.bindings.length; i++) {
      var b = mod.bindings[i];
      var sfx = bindingSuffix(b);
      var read = "  uint t = j >> 2u;";
      var loc = "  ivec2 c = ivec2(int(t % uint(u_width)), int(t / uint(u_width)));";
      var fetch = "  vec4 v = texelFetch(_s_" + sfx + ", c, 0);";
      var lane = "  return v[int(j & 3u)];";
      out.push("uniform sampler2D _s_" + sfx + ";");
      out.push("uniform int _n_" + sfx + ";   // element count of " + b.name);
      if (b.elem === "f32") {
        out.push("float _rd_f32_" + sfx + "(uint j) {");
        out.push("  if (j >= uint(_n_" + sfx + ")) return 0.0;");
        out.push(read); out.push(loc); out.push(fetch); out.push(lane);
        out.push("}");
      } else if (b.elem === "u32") {
        out.push("uint _rd_u32_" + sfx + "(uint j) {");
        out.push("  if (j >= uint(_n_" + sfx + ")) return 0u;");
        out.push(read); out.push(loc); out.push(fetch);
        out.push("  return floatBitsToUint(v[int(j & 3u)]);");
        out.push("}");
      } else {
        out.push("int _rd_i32_" + sfx + "(uint j) {");
        out.push("  if (j >= uint(_n_" + sfx + ")) return 0;");
        out.push(read); out.push(loc); out.push(fetch);
        out.push("  return floatBitsToInt(v[int(j & 3u)]);");
        out.push("}");
      }
    }
    // module-scope consts (integer ones are also folded into index forms)
    var types = {}, values = {};
    if (mod.consts.length) {
      var tmp = new Emitter(mod, null);
      for (var ci = 0; ci < mod.consts.length; ci++) {
        var c = mod.consts[ci];
        var declared = c.type ? typeFromString(c.type) : null;
        var r = tmp.expr(c.expr, declared);
        var t = declared || (isAbstract(r.type) ? T(r.type.s === "abstract-int" ? "i32" : "f32", 1) : r.type);
        types[c.name] = t;
        var cv = constIntValue(c.expr);
        if (cv !== null) values[c.name] = cv;
        out.push("const " + glslType(t) + " _c_" + c.name + " = " + r.code + ";");
      }
    }
    return { glsl: out.join("\n"), constTypes: types, constValues: values };
  }

  // Fold a constant expression down to an integer, when it is a plain literal
  // expression. Used so `const W = 640u;` can appear in a store index.
  function constIntValue(node) {
    if (!node) return null;
    if (node.k === "paren") return constIntValue(node.e);
    if (node.k === "lit") return node.tok.isFloat ? null : node.tok.value;
    if (node.k === "ctor" && (node.type === "u32" || node.type === "i32") && node.args.length === 1) {
      return constIntValue(node.args[0]);
    }
    if (node.k === "unary" && node.op === "-") {
      var v = constIntValue(node.e);
      return v === null ? null : -v;
    }
    if (node.k === "bin" && (node.op === "+" || node.op === "-" || node.op === "*")) {
      var l = constIntValue(node.l), r = constIntValue(node.r);
      if (l === null || r === null) return null;
      return node.op === "+" ? l + r : node.op === "-" ? l - r : l * r;
    }
    return null;
  }

  /**
   * Translate a WGSL module for one entry point.
   * Returns { ok, reason, workgroupSize, bindings, writes, common, helpers,
   *           bodies: {bindingName: "void kernel(...) {...}"} }
   */
  function translate(code, entryPoint) {
    var mod;
    try {
      mod = parseModule(String(code || ""));
    } catch (e) {
      if (e && e.wgslRefuse) return { ok: false, reason: e.message };
      return { ok: false, reason: "WGSL parse error: " + ((e && e.message) || e) };
    }
    if (!mod.entry) return { ok: false, reason: "the module has no @compute entry point" };
    if (entryPoint && entryPoint !== mod.entry.name) {
      return { ok: false, reason: "entry point '" + entryPoint + "' not found (module defines '" + mod.entry.name + "')" };
    }

    var common;
    try {
      common = buildCommonGlsl(mod);
    } catch (e) {
      if (e && e.wgslRefuse) return { ok: false, reason: e.message };
      return { ok: false, reason: "WGSL translation error: " + ((e && e.message) || e) };
    }

    var writes = [];
    for (var w = 0; w < mod.bindings.length; w++) {
      if (mod.bindings[w].access === "read_write") writes.push(mod.bindings[w]);
    }

    var bodies = {}, uses = { select: false, round: false }, writeForms = {};
    for (var i = 0; i < writes.length; i++) {
      var em = new Emitter(mod, writes[i]);
      em.constTypes = common.constTypes;
      em.constValues = common.constValues;
      var b = writes[i];
      em.emit("void kernel(inout vec4 io, int i) {");
      em.depth++;
      // Elements this dispatch does not write must keep their previous value,
      // so the output starts as a read of the pre-dispatch contents.
      em.emit("io.x = " + (b.elem === "f32" ? em.accessor(b, "uint(i)")
        : (b.elem === "u32" ? "uintBitsToFloat(" : "intBitsToFloat(") + em.accessor(b, "uint(i)") + ")") + ";");
      try {
        for (var s = 0; s < mod.entry.body.stmts.length; s++) em.stmt(mod.entry.body.stmts[s]);
      } catch (e) {
        if (e && e.wgslRefuse) return { ok: false, reason: e.message };
        return { ok: false, reason: "WGSL translation error: " + ((e && e.message) || e) };
      }
      em.depth--;
      em.emit("}");
      bodies[b.name] = em.lines.join("\n");
      if (em.uses.select) uses.select = true;
      if (em.uses.round) uses.round = true;
      for (var bn in em.writeForms) {
        if (has(em.writeForms, bn)) writeForms[bn] = em.writeForms[bn];
      }
    }

    if (writes.length === 0) {
      // Nothing is stored: a dispatch would have no observable effect. Accept
      // it (real WebGPU does) but there is no kernel to run.
      bodies = {};
    }

    return {
      ok: true, reason: null, mod: mod, workgroupSize: mod.workgroupSize,
      bindings: mod.bindings, writes: writes, common: common.glsl,
      uses: uses, bodies: bodies, writeForms: writeForms, entryPoint: mod.entry.name
    };
  }

  /* ==========================================================================
   * 6. WebGPU constant objects
   * ======================================================================== */

  var GPUBufferUsage_ = {
    MAP_READ: 0x0001, MAP_WRITE: 0x0002, COPY_SRC: 0x0004, COPY_DST: 0x0008,
    INDEX: 0x0010, VERTEX: 0x0020, UNIFORM: 0x0040, STORAGE: 0x0080,
    INDIRECT: 0x0100, QUERY_RESOLVE: 0x0200
  };
  var GPUShaderStage_ = { VERTEX: 0x1, FRAGMENT: 0x2, COMPUTE: 0x4 };
  var GPUMapMode_ = { READ: 0x0001, WRITE: 0x0002 };

  function defineGlobal(name, value) {
    if (typeof global[name] === "undefined") global[name] = value;
  }

  /* ==========================================================================
   * 7. statistics
   * ======================================================================== */

  var MAX_REASONS = 64;
  var stats = {
    translated: 0,
    delegated: 0,
    refused: 0,
    reasons: [],
    droppedReasons: 0,
    dispatches: 0,
    passes: 0,
    copies: 0,
    installs: 0,
    uninstalls: 0,
    translatedMs: 0,
    caps: {},
    note: function (kind, entryPoint, reason, code) {
      stats[kind === "refused" ? "refused" : kind === "delegated" ? "delegated" : "translated"]++;
      if (kind === "translated") return;
      if (stats.reasons.length >= MAX_REASONS) { stats.droppedReasons++; return; }
      stats.reasons.push({
        kind: kind,
        entryPoint: entryPoint || "main",
        reason: reason,
        when: new Date().toISOString(),
        code: String(code || "").slice(0, 400)
      });
    },
    reset: function () {
      stats.translated = 0; stats.delegated = 0; stats.refused = 0;
      stats.reasons = []; stats.droppedReasons = 0;
      stats.dispatches = 0; stats.passes = 0; stats.copies = 0; stats.translatedMs = 0;
    }
  };

  /* ==========================================================================
   * 8. WebGPU-ish object model
   * ======================================================================== */

  var nextId = 1;

  function ShimBuffer(device, desc) {
    desc = desc || {};
    var size = desc.size | 0;
    if (!(size > 0)) refuse("createBuffer: size must be a positive number");
    this.id = nextId++;
    this.device = device;
    this.size = Math.ceil(size / 4) * 4;      // WebGPU rounds up to 4 bytes
    this.usage = desc.usage | 0;
    this.label = desc.label || "";
    this.mapState = "unmapped";
    this.destroyed = false;
    this._ab = new ArrayBuffer(this.size);
    this._real = null;         // mirrored real GPUBuffer
    this._realPending = null;  // promise from a delegated dispatch touching this buffer
    this._mapMode = 0;
    if (desc.mappedAtCreation) {
      this.mapState = "mapped";
      this._mapMode = GPUMapMode_.WRITE;
    }
  }

  ShimBuffer.prototype.getMappedRange = function (offset, size) {
    if (this.destroyed) refuse("getMappedRange: the buffer is destroyed");
    if (offset !== undefined && offset !== 0) {
      refuse("getMappedRange(offset > 0) is not supported by the WebGPU shim: JavaScript cannot " +
        "return a sub-range ArrayBuffer that shares memory. Use getMappedRange() and add the offset " +
        "in the typed-array view instead.");
    }
    return this._ab;
  };

  ShimBuffer.prototype.unmap = function () {
    this.mapState = "unmapped";
    this._mapMode = 0;
  };

  ShimBuffer.prototype.mapAsync = function (mode, offset, size) {
    var self = this;
    void mode; void offset; void size;
    if (this.destroyed) return Promise.reject(new Error("mapAsync: the buffer is destroyed"));
    // Any delegated GPU work touching this buffer must land first.
    var p = this._realPending || Promise.resolve();
    return p.then(function () {
      self.mapState = "mapped";
      self._mapMode = mode;
    });
  };

  ShimBuffer.prototype.destroy = function () {
    this.destroyed = true;
    this._ab = new ArrayBuffer(0);
    if (this._real && this._real.destroy) { try { this._real.destroy(); } catch (e) { /* ignore */ } }
    this._real = null;
  };

  // A view of this buffer's CPU bytes as floats, used as engine input.
  ShimBuffer.prototype._floatView = function (byteOffset, elementCount) {
    var off = byteOffset | 0;
    if (off % 4 !== 0) refuse("buffer bindings must be 4-byte aligned");
    var maxElems = Math.floor((this.size - off) / 4);
    var n = Math.min(elementCount, maxElems);
    if (n <= 0) return new Float32Array(0);
    return new Float32Array(this._ab, off, n);
  };

  function ShimShaderModule(device, desc) {
    this.id = nextId++;
    this.device = device;
    this.code = String((desc && desc.code) || "");
    this.label = (desc && desc.label) || "";
    this._messages = [];
    // Parse eagerly so getCompilationInfo() can report real diagnostics.
    try {
      parseModule(this.code);
    } catch (e) {
      this._messages.push({
        type: "error", lineNum: 0, linePos: 0, offset: 0,
        message: (e && e.message) || String(e)
      });
    }
  }

  ShimShaderModule.prototype.getCompilationInfo = function () {
    var msgs = this._messages.slice();
    return Promise.resolve({
      messages: msgs.map(function (m) {
        return { type: m.type, message: m.message, lineNum: m.lineNum, linePos: m.linePos, offset: m.offset };
      })
    });
  };

  function ShimBindGroupLayout(device, desc) {
    this.id = nextId++;
    this.device = device;
    this.entries = (desc && desc.entries) || [];
    this.label = (desc && desc.label) || "";
  }

  function ShimPipelineLayout(device, desc) {
    this.id = nextId++;
    this.device = device;
    this.bindGroupLayouts = (desc && desc.bindGroupLayouts) || [];
    this.label = (desc && desc.label) || "";
    if (this.bindGroupLayouts === "auto") this.bindGroupLayouts = [];
  }

  function ShimBindGroup(device, desc) {
    this.id = nextId++;
    this.device = device;
    this.layout = desc && desc.layout;
    this.entries = (desc && desc.entries) || [];
    this.label = (desc && desc.label) || "";
    this._real = {};
  }

  ShimBindGroup.prototype._bufferFor = function (group, binding) {
    for (var i = 0; i < this.entries.length; i++) {
      var e = this.entries[i];
      if (e.binding !== binding) continue;
      var r = e.resource;
      if (!r) return null;
      if (r.buffer) {
        return {
          buffer: r.buffer,
          offset: r.offset | 0,
          size: (r.size === undefined || r.size === null) ? (r.buffer.size - (r.offset | 0)) : (r.size | 0)
        };
      }
      refuse("bind group " + group + " binding " + binding +
        ": only buffer resources are supported by the WebGPU compute shim");
    }
    void group;
    return null;
  };

  function ShimComputePipeline(device, desc) {
    this.id = nextId++;
    this.device = device;
    this.label = (desc && desc.label) || "";
    var comp = (desc && desc.compute) || {};
    this.module = comp.module;
    this.entryPoint = comp.entryPoint || "main";
    this.layout = desc && desc.layout;
    this.mode = null;        // "translated" | "real"
    this.plan = null;
    this.realPipeline = null;
    this._realBGLs = null;

    if (!this.module || !this.module.code) refuse("createComputePipeline: compute.module is required");
    var t0 = now();
    var r = translate(this.module.code, this.entryPoint);
    var ms = now() - t0;
    stats.translatedMs += ms;

    if (r.ok) {
      this.mode = "translated";
      this.plan = r;
      stats.note("translated", r.entryPoint, null, null);
      return;
    }

    // ---- untranslatable -------------------------------------------------
    var dev = device;
    var canDelegate = !!(dev._delegate && dev._real);
    stats.note(canDelegate ? "delegated" : "refused", this.entryPoint, r.reason, this.module.code);
    if (!canDelegate) {
      var msg = "WgpuShim: refusing to run this WGSL compute shader - " + r.reason +
        ". The WebGL2 compute engine cannot express it, and it will NOT be guessed at. " +
        "Reasons are recorded on WgpuShim.stats.reasons.";
      var err = new Error(msg);
      err.name = "WgpuShimUnsupportedError";
      err.wgslReason = r.reason;
      try { console.error(msg); } catch (e2) { /* ignore */ }
      throw err;
    }
    // Delegate the whole pipeline to the real WebGPU device (opt-in: on the
    // Xbox the real compute path is what faults the GPU process).
    this.mode = "real";
    this._realReason = r.reason;
    this._realModule = dev._real.createShaderModule({ code: this.module.code, label: "wgpu-shim:" + this.label });
    this.realPipeline = null;      // built at the first dispatch, once the
    this._realBGLs = null;         // bind groups (and their layouts) are known
  }

  function ShimComputePass(encoder) {
    this.encoder = encoder;
    this._pipeline = null;
    this._bindGroups = {};
    this._ended = false;
  }
  ShimComputePass.prototype.setPipeline = function (p) { this._pipeline = p; };
  ShimComputePass.prototype.setBindGroup = function (index, group) {
    this._bindGroups[index | 0] = group;
    this._dynamicOffsets = this._dynamicOffsets || {};
  };
  ShimComputePass.prototype.dispatchWorkgroups = function (x, y, z) {
    if (!this._pipeline) refuse("dispatchWorkgroups: no pipeline is set");
    this.encoder.cmds.push({
      t: "dispatch", pipeline: this._pipeline, bindGroups: this._bindGroups,
      x: Math.max(1, x | 0), y: Math.max(1, (y === undefined ? 1 : y) | 0),
      z: Math.max(1, (z === undefined ? 1 : z) | 0)
    });
  };
  ShimComputePass.prototype.dispatchWorkgroupsIndirect = function () {
    refuse("dispatchWorkgroupsIndirect is not supported by the WebGPU compute shim");
  };
  ShimComputePass.prototype.end = function () { this._ended = true; };

  function ShimCommandEncoder(device) {
    this.device = device;
    this.cmds = [];
  }
  ShimCommandEncoder.prototype.beginComputePass = function () { return new ShimComputePass(this); };
  ShimCommandEncoder.prototype.copyBufferToBuffer = function (src, srcOffset, dst, dstOffset, size) {
    this.cmds.push({
      t: "copy", src: src, srcOffset: srcOffset | 0, dst: dst, dstOffset: dstOffset | 0, size: size | 0
    });
  };
  ShimCommandEncoder.prototype.copyBufferToTexture = function () {
    refuse("copyBufferToTexture is not supported by the WebGPU compute shim");
  };
  ShimCommandEncoder.prototype.copyTextureToBuffer = function () {
    refuse("copyTextureToBuffer is not supported by the WebGPU compute shim");
  };
  ShimCommandEncoder.prototype.finish = function () {
    return { t: "cmd", device: this.device, cmds: this.cmds.slice() };
  };

  function ShimQueue(device) {
    this.device = device;
    this._pending = [];
  }
  ShimQueue.prototype.submit = function (buffers) {
    var self = this;
    for (var i = 0; i < (buffers || []).length; i++) {
      var cb = buffers[i];
      if (!cb || cb.t !== "cmd") refuse("queue.submit: expected a command buffer from encoder.finish()");
      for (var c = 0; c < cb.cmds.length; c++) {
        var cmd = cb.cmds[c];
        if (cmd.t === "copy") {
          stats.copies++;
          copyBytes(cmd.src, cmd.srcOffset, cmd.dst, cmd.dstOffset, cmd.size);
        } else if (cmd.t === "dispatch") {
          stats.dispatches++;
          self.device._dispatch(cmd);
        }
      }
    }
  };
  ShimQueue.prototype.writeBuffer = function (buffer, bufferOffset, data, dataOffset, size) {
    if (!buffer) refuse("queue.writeBuffer: a buffer is required");
    var off = bufferOffset | 0;
    var src, srcOff = dataOffset | 0, bytes = size;
    if (data instanceof ArrayBuffer) {
      src = new Uint8Array(data);
    } else if (data && data.buffer) {
      src = new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
    } else {
      refuse("queue.writeBuffer: data must be an ArrayBuffer or a typed array");
    }
    if (bytes === undefined) bytes = src.byteLength - srcOff;
    var doWrite = function () {
      var dst = new Uint8Array(buffer._ab, off, bytes);
      dst.set(src.subarray(srcOff, srcOff + bytes));
    };
    // If delegated GPU work is still in flight for this buffer, the CPU write
    // must land after the read-back, otherwise the read-back would overwrite it.
    if (buffer._realPending) {
      buffer._realPending = buffer._realPending.then(doWrite);
    } else {
      doWrite();
    }
  };
  ShimQueue.prototype.onSubmittedWorkDone = function () {
    var all = [];
    for (var i = 0; i < this._pending.length; i++) all.push(this._pending[i]);
    this._pending = [];
    return Promise.all(all).then(function () { });
  };

  function copyBytes(src, srcOffset, dst, dstOffset, size) {
    if (!src || !dst) refuse("copyBufferToBuffer: both buffers are required");
    var s = new Uint8Array(src._ab, srcOffset, size);
    var d = new Uint8Array(dst._ab, dstOffset, size);
    d.set(s);
    // Keep the async ordering: if the source is waiting on a delegated
    // dispatch, the destination is now waiting on it too.
    if (src._realPending) {
      var p = src._realPending;
      dst._realPending = dst._realPending ? Promise.all([dst._realPending, p]) : p;
    }
  }

  function ShimFeatures() { this._set = {}; }
  ShimFeatures.prototype.has = function (f) { return !!this._set[f]; };
  ShimFeatures.prototype.forEach = function (fn, thisArg) {
    for (var k in this._set) if (has(this._set, k)) fn.call(thisArg, k, k, this);
  };

  function ShimAdapter(device, info) {
    this.id = nextId++;
    this.name = info.name || "wgpu-shim-webgl2";
    this.info = info;
    this.features = device.features;
    this.limits = device.limits;
    this.isFallbackAdapter = false;
  }
  ShimAdapter.prototype.requestDevice = function () {
    return Promise.resolve(this._device);
  };
  ShimAdapter.prototype.requestAdapterInfo = function () { return Promise.resolve(this.info); };

  function ShimDevice(adapter, options) {
    options = options || {};
    this.id = nextId++;
    this.adapter = adapter;
    this.label = options.label || "";
    this.features = new ShimFeatures();
    this.limits = {
      maxTextureDimension1D: 8192, maxTextureDimension2D: 8192, maxTextureDimension3D: 2048,
      maxTextureArrayLayers: 256, maxBindGroups: 4, maxBindingsPerBindGroup: 640,
      maxDynamicUniformBuffersPerPipelineLayout: 8,
      maxDynamicStorageBuffersPerPipelineLayout: 4,
      maxSampledTexturesPerShaderStage: 16, maxSamplersPerShaderStage: 16,
      maxStorageBuffersPerShaderStage: 8, maxStorageTexturesPerShaderStage: 4,
      maxUniformBuffersPerShaderStage: 12, maxUniformBufferBindingSize: 65536,
      maxStorageBufferBindingSize: 134217728, minUniformBufferOffsetAlignment: 256,
      minStorageBufferOffsetAlignment: 256, maxVertexBuffers: 8,
      maxBufferSize: 268435456, maxVertexAttributes: 16, maxVertexBufferArrayStride: 2048,
      maxInterStageShaderComponents: 60, maxInterStageShaderVariables: 16,
      maxColorAttachments: 8, maxColorAttachmentBytesPerSample: 32,
      maxComputeWorkgroupStorageSize: 16384, maxComputeInvocationsPerWorkgroup: 256,
      maxComputeWorkgroupSizeX: 256, maxComputeWorkgroupSizeY: 256, maxComputeWorkgroupSizeZ: 64,
      maxComputeWorkgroupsPerDimension: 65535
    };
    // Report what the WebGL2 engine can really take, not what Dawn would.
    if (options.limits) {
      for (var k in options.limits) if (has(options.limits, k)) this.limits[k] = options.limits[k];
    }
    this.queue = new ShimQueue(this);
    this.onuncapturederror = null;
    this._delegate = !!options.delegate;
    this._real = options.realDevice || null;
    this._hw = options.hw;
    this._progSeq = 0;
    this._lost = new Promise(function () { /* never settles: the shim device is never lost */ });
    this.lost = this._lost;
    this._onRealLost = null;
  }

  ShimDevice.prototype._error = function (msg) {
    var e = new Error("WgpuShim: " + msg);
    if (this.onuncapturederror) {
      try { this.onuncapturederror({ error: e, type: "uncapturederror" }); } catch (e2) { /* ignore */ }
    }
    return e;
  };

  ShimDevice.prototype.createBuffer = function (desc) {
    var b = new ShimBuffer(this, desc);
    return b;
  };
  ShimDevice.prototype.createShaderModule = function (desc) {
    return new ShimShaderModule(this, desc);
  };
  ShimDevice.prototype.createBindGroupLayout = function (desc) {
    return new ShimBindGroupLayout(this, desc);
  };
  ShimDevice.prototype.createPipelineLayout = function (desc) {
    return new ShimPipelineLayout(this, desc);
  };
  ShimDevice.prototype.createBindGroup = function (desc) {
    return new ShimBindGroup(this, desc);
  };
  ShimDevice.prototype.createComputePipeline = function (desc) {
    return new ShimComputePipeline(this, desc);
  };
  ShimDevice.prototype.createComputePipelineAsync = function (desc) {
    try { return Promise.resolve(new ShimComputePipeline(this, desc)); }
    catch (e) { return Promise.reject(e); }
  };
  ShimDevice.prototype.createCommandEncoder = function () {
    return new ShimCommandEncoder(this);
  };
  ShimDevice.prototype.createTexture = function () {
    refuse("textures are not supported by the WebGPU compute shim");
  };
  ShimDevice.prototype.createSampler = function () {
    refuse("samplers are not supported by the WebGPU compute shim");
  };
  ShimDevice.prototype.pushErrorScope = function () { /* no-op: errors are thrown */ };
  ShimDevice.prototype.popErrorScope = function () { return Promise.resolve(null); };
  ShimDevice.prototype.destroy = function () {
    if (this._hw) { try { this._hw.clearProgramCache(); } catch (e) { /* ignore */ } }
  };

  /* ------------------------------------------------------------------------
   * Dispatch: translated pipelines
   * ---------------------------------------------------------------------- */

  var MAX_INPUTS = 8;

  ShimDevice.prototype._dispatch = function (cmd) {
    var pipeline = cmd.pipeline;
    if (!pipeline || pipeline.device !== this) refuse("dispatch with a pipeline from another device");
    try {
      if (pipeline.mode === "real") return this._dispatchReal(cmd);
      return this._dispatchTranslated(cmd);
    } catch (e) {
      // A dispatch can be refused where the pipeline was not (for example when
      // the store index is only element-local for certain dispatches). Record
      // it, then let it propagate: never silently proceed.
      if (e && e.wgslRefuse) {
        stats.note("refused", pipeline.entryPoint,
          "refused at dispatch: " + e.message,
          pipeline.module ? pipeline.module.code : "");
      }
      throw e;
    }
  };

  ShimDevice.prototype._dispatchTranslated = function (cmd) {
    var plan = cmd.pipeline.plan;
    var hw = this._hw;
    var i, b;

    // ---- resolve every declared binding to a buffer --------------------
    var slots = [];
    var maxLen = 0;
    for (i = 0; i < plan.bindings.length; i++) {
      b = plan.bindings[i];
      var grp = cmd.bindGroups[b.group];
      var res = grp ? grp._bufferFor(b.group, b.binding) : null;
      if (!res) {
        refuse("no bind group entry for @group(" + b.group + ") @binding(" + b.binding + ") '" +
          b.name + "' - cannot run the dispatch");
      }
      if (res.buffer.destroyed) refuse("buffer '" + b.name + "' is destroyed");
      if (res.offset % 4 !== 0) refuse("buffer binding offset must be a multiple of 4 bytes");
      var elements = Math.floor(res.size / 4);
      slots.push({ b: b, res: res, elements: elements });
      if (elements > maxLen) maxLen = elements;
    }
    if (slots.length > MAX_INPUTS) {
      refuse("the shader binds " + slots.length + " storage buffers; the WebGL2 engine supports at most " +
        MAX_INPUTS + " texture inputs");
    }

    // The whole bound data set has to fit in one texture grid.
    var gridCap = hw.maxTexture * hw.maxTexture * 4;
    if (maxLen > gridCap) {
      refuse("a bound buffer holds " + maxLen + " elements, more than the " + gridCap +
        " element texture grid this GPU can address");
    }

    var wg = plan.workgroupSize;
    var nx = wg[0] * cmd.x, ny = wg[1] * cmd.y, nz = wg[2] * cmd.z;
    var invocations = nx * ny * nz;
    var n = Math.min(invocations, maxLen);
    if (n <= 0) return;

    if (plan.writes.length === 0) {
      // No storage writes at all: nothing observable to do.
      return;
    }

    // The store indices were reduced to linear forms at translation time; they
    // are only element-local for THIS dispatch if the forms resolve to the
    // invocation's own index. Verify, and refuse loudly otherwise.
    for (i = 0; i < plan.writes.length; i++) {
      var forms = plan.writeForms[plan.writes[i].name] || [];
      for (var fi = 0; fi < forms.length; fi++) {
        if (!formIsIdentity(forms[fi], nx, ny, nz)) {
          var f = forms[fi];
          refuse("the store index of '" + plan.writes[i].name + "' is " + f.x + "*gid.x + " + f.y +
            "*gid.y + " + f.z + "*gid.z + " + f.k + ", which is not the current invocation's own " +
            "element for this dispatch (global_invocation_id extents " + nx + "x" + ny + "x" + nz +
            "). The WebGL2 engine stores element i back into element i, so this dispatch is refused " +
            "rather than mis-computed.");
        }
      }
    }

    // ---- run one pass per writable buffer ------------------------------
    for (var w = 0; w < plan.writes.length; w++) {
      var out = plan.writes[w];
      var outSlot = null;
      for (i = 0; i < slots.length; i++) if (slots[i].b === out) outSlot = slots[i];
      if (!outSlot) refuse("internal: output buffer " + out.name + " has no binding");

      var inputs = [];
      for (i = 0; i < slots.length; i++) {
        inputs.push({
          name: "_s_" + bindingSuffix(slots[i].b),
          data: slots[i].res.buffer._floatView(slots[i].res.offset, slots[i].elements)
        });
      }
      var uniforms = { u_nx: nx, u_ny: ny, u_nz: nz };
      for (i = 0; i < slots.length; i++) {
        uniforms["_n_" + bindingSuffix(slots[i].b)] = slots[i].elements;
      }
      var body = plan.bodies[out.name];
      if (!body) continue;
      var glsl = plan.common + "\n" +
        (plan.uses.select ? selectHelpers() + "\n" : "") +
        (plan.uses.round ? roundHelpers() + "\n" : "") +
        body;

      var t0 = now();
      var res = hw.runEx({
        count: n,
        gridCount: maxLen,
        inputs: inputs,
        uniforms: uniforms,
        glsl: glsl,
        cacheKey: "shim_p" + cmd.pipeline.id + "_" + out.name
      });
      stats.translatedMs += now() - t0;
      stats.passes++;

      // bit-exact write-back into the CPU mirror
      var writeCount = Math.min(n, outSlot.elements);
      var dst = new Uint32Array(outSlot.res.buffer._ab, outSlot.res.offset, writeCount);
      dst.set(new Uint32Array(res.buffer, res.byteOffset, writeCount));
    }
  };

  /* ------------------------------------------------------------------------
   * Dispatch: delegated pipelines (real WebGPU device)
   * ---------------------------------------------------------------------- */

  var REAL_USAGE = GPUBufferUsage_.STORAGE | GPUBufferUsage_.COPY_SRC |
    GPUBufferUsage_.COPY_DST | GPUBufferUsage_.MAP_READ | GPUBufferUsage_.MAP_WRITE;

  ShimDevice.prototype._realBuffer = function (buffer) {
    if (!buffer._real) {
      buffer._real = this._real.createBuffer({
        size: buffer.size,
        usage: REAL_USAGE,
        label: "wgpu-shim-mirror"
      });
    }
    return buffer._real;
  };

  /* Build the real pipeline (and its bind group layouts) the first time a
     delegated dispatch actually runs, because only then are the bind groups
     available to describe the layout. */
  ShimDevice.prototype._ensureRealPipeline = function (pipeline, indexGroups) {
    if (pipeline.realPipeline) return;
    var dev = this._real;
    var realBGLs = [];
    for (var i = 0; i < indexGroups.length; i++) {
      var bg = indexGroups[i].group;
      var entries = [];
      for (var e = 0; e < bg.entries.length; e++) {
        var entry = bg.entries[e];
        if (!entry.resource || !entry.resource.buffer) continue;
        var type = "storage";
        var lay = bg.layout;
        if (lay && lay.entries) {
          for (var l = 0; l < lay.entries.length; l++) {
            if (lay.entries[l].binding === entry.binding && lay.entries[l].buffer &&
                lay.entries[l].buffer.type) {
              type = lay.entries[l].buffer.type;
            }
          }
        }
        entries.push({ binding: entry.binding, visibility: GPUShaderStage_.COMPUTE, buffer: { type: type } });
      }
      realBGLs.push(dev.createBindGroupLayout({ entries: entries }));
    }
    pipeline._realBGLs = realBGLs;
    pipeline._realPipelineLayout = dev.createPipelineLayout({ bindGroupLayouts: realBGLs });
    pipeline.realPipeline = dev.createComputePipeline({
      layout: pipeline._realPipelineLayout,
      compute: { module: pipeline._realModule, entryPoint: pipeline.entryPoint }
    });
  };

  ShimDevice.prototype._dispatchReal = function (cmd) {
    var self = this;
    var dev = this._real;
    if (!dev) refuse("this pipeline needs real WebGPU, but no real device is available");

    var sources = [];
    var groups = [];
    for (var g in cmd.bindGroups) {
      if (has(cmd.bindGroups, g)) groups.push({ index: parseInt(g, 10), group: cmd.bindGroups[g] });
    }
    groups.sort(function (a, b) { return a.index - b.index; });
    for (var gi = 0; gi < groups.length; gi++) {
      var entries = groups[gi].group.entries;
      for (var e = 0; e < entries.length; e++) {
        var r = entries[e].resource;
        if (r && r.buffer) sources.push(r.buffer);
      }
    }

    var prereq = [];
    var mirrors = {};
    var i;
    for (i = 0; i < sources.length; i++) {
      if (sources[i]._realPending) prereq.push(sources[i]._realPending);
    }

    var p = Promise.all(prereq).then(function () {
      self._ensureRealPipeline(cmd.pipeline, groups);
      // 1. mirror every bound buffer and upload the current CPU bytes
      for (var k = 0; k < sources.length; k++) {
        var buf = sources[k];
        var mirror = mirrors[buf.id] || (mirrors[buf.id] = self._realBuffer(buf));
        dev.queue.writeBuffer(mirror, 0, new Uint8Array(buf._ab));
      }
      // 2. real bind groups
      var realGroups = [];
      for (var q = 0; q < groups.length; q++) {
        var bgs = groups[q].group;
        if (!bgs._real[cmd.pipeline.id]) {
          var realEntries = [];
          for (var m = 0; m < bgs.entries.length; m++) {
            var ent = bgs.entries[m];
            if (!ent.resource || !ent.resource.buffer) continue;
            realEntries.push({
              binding: ent.binding,
              resource: {
                buffer: self._realBuffer(ent.resource.buffer),
                offset: ent.resource.offset | 0,
                size: (ent.resource.size === undefined || ent.resource.size === null)
                  ? undefined : (ent.resource.size | 0)
              }
            });
          }
          bgs._real[cmd.pipeline.id] = dev.createBindGroup({
            layout: cmd.pipeline._realBGLs[q],
            entries: realEntries
          });
        }
        realGroups.push({ index: groups[q].index, group: bgs._real[cmd.pipeline.id] });
      }
      // 3. encode and submit
      var enc = dev.createCommandEncoder();
      var pass = enc.beginComputePass();
      pass.setPipeline(cmd.pipeline.realPipeline);
      for (var z = 0; z < realGroups.length; z++) {
        pass.setBindGroup(realGroups[z].index, realGroups[z].group);
      }
      pass.dispatchWorkgroups(cmd.x, cmd.y, cmd.z);
      pass.end();
      var staging = [];
      for (var s2 = 0; s2 < sources.length; s2++) {
        var rb = mirrors[sources[s2].id];
        var st = dev.createBuffer({ size: sources[s2].size, usage: GPUBufferUsage_.COPY_DST | GPUBufferUsage_.MAP_READ });
        enc.copyBufferToBuffer(rb, 0, st, 0, sources[s2].size);
        staging.push({ shim: sources[s2], real: st });
      }
      dev.queue.submit([enc.finish()]);
      // 4. read everything back into the CPU mirrors
      var maps = [];
      for (var t = 0; t < staging.length; t++) {
        maps.push((function (st) {
          return st.real.mapAsync(GPUMapMode_.READ).then(function () {
            var src = new Uint8Array(st.real.getMappedRange());
            new Uint8Array(st.shim._ab).set(src.subarray(0, st.shim.size));
            st.real.unmap();
            if (st.real.destroy) st.real.destroy();
          });
        })(staging[t]));
      }
      return Promise.all(maps);
    });

    // Every buffer touching this dispatch now has real work in flight.
    for (i = 0; i < sources.length; i++) {
      var prev = sources[i]._realPending;
      sources[i]._realPending = prev ? Promise.all([prev, p]) : p;
    }
    this.queue._pending.push(p);
  };

  /* ==========================================================================
   * 9. install / uninstall
   * ======================================================================== */

  var WgpuShim = {
    version: VERSION,
    installed: false,
    stats: stats,
    realGpu: null,
    realDevice: null,
    hw: null,
    device: null,
    adapter: null,
    _nativeGpu: null
  };

  function now() {
    return (global.performance && global.performance.now) ? global.performance.now() : Date.now();
  }

  /* Probe whether u32/i32 bit patterns survive a round trip through the
     RGBA32F texture grid. On every stack tested so far they do, but the shim
     reports it instead of assuming it. */
  function probeU32Exact(hw) {
    var pat = new Uint32Array([
      0, 1, 0x00800000, 0x3F800000, 0x7F800001, 0x7FC00000,
      0xFFFFFFFF, 0x807FFFFF, 0x00400000, 0x12345678, 0x80000000, 0x7F7FFFFF
    ]);
    var glsl = [
      "uniform sampler2D _s_0_0;",
      "uniform int _n_0_0;",
      "uint _rd_u32_0_0(uint j) {",
      "  if (j >= uint(_n_0_0)) return 0u;",
      "  uint t = j >> 2u;",
      "  ivec2 c = ivec2(int(t % uint(u_width)), int(t / uint(u_width)));",
      "  return floatBitsToUint(texelFetch(_s_0_0, c, 0)[int(j & 3u)]);",
      "}",
      "void kernel(inout vec4 io, int i) { io.x = uintBitsToFloat(_rd_u32_0_0(uint(i))); }"
    ].join("\n");
    try {
      var r = hw.runEx({
        count: pat.length, gridCount: pat.length,
        inputs: [{ name: "_s_0_0", data: new Float32Array(pat.buffer) }],
        uniforms: { _n_0_0: pat.length },
        glsl: glsl
      });
      var got = new Uint32Array(r.buffer, r.byteOffset, pat.length);
      for (var i = 0; i < pat.length; i++) if (got[i] !== pat[i]) return false;
      return true;
    } catch (e) {
      return false;
    }
  }

  /**
   * install(options)
   *
   *   force               : replace an existing navigator.gpu (default false)
   *   delegateUnsupported : hand untranslatable shaders to the real WebGPU
   *                         device instead of throwing (default false)
   *   realGpu             : the real GPU object to keep as the delegation
   *                         target (defaults to the current navigator.gpu)
   *   realDevice          : an already-created real GPUDevice (skips
   *                         requestAdapter/requestDevice entirely)
   *   hw                  : reuse an existing HWCompute instance
   *
   * Returns a report object; never throws for the "refused to install" cases.
   */
  WgpuShim.install = function (options) {
    options = options || {};
    var report = { installed: false, version: VERSION, reason: null };

    if (WgpuShim.installed && !options.force) {
      report.reason = "WgpuShim is already installed";
      return report;
    }
    var HW = global.HWCompute;
    if (!HW) {
      report.reason = "HWCompute is not loaded - include hwcompute.js before wgpu-shim.js";
      return report;
    }
    var nav = global.navigator;
    if (!nav) {
      report.reason = "no navigator object in this environment";
      return report;
    }

    // Capture the real object FIRST, before anything is replaced.
    var nativeGpu = has(options, "realGpu") ? options.realGpu : nav.gpu;
    if (nativeGpu) WgpuShim.realGpu = nativeGpu;
    if (options.realDevice) WgpuShim.realDevice = options.realDevice;

    if (nativeGpu && !options.force) {
      report.reason = "a native navigator.gpu is present; pass {force:true} to replace it " +
        "(the native object has been captured for delegation)";
      report.nativePresent = true;
      stats.caps.nativePresent = true;
      return report;
    }

    var hw = options.hw || WgpuShim.hw;
    if (!hw) {
      try { hw = new HW(); } catch (e) {
        report.reason = "WebGL2 compute engine unavailable: " + ((e && e.message) || e);
        return report;
      }
    }
    if (!hw.floatRenderable) {
      report.reason = "EXT_color_buffer_float is unavailable - float render targets are required";
      return report;
    }
    WgpuShim.hw = hw;

    var delegate = !!options.delegateUnsupported;
    var device = new ShimDevice(null, {
      delegate: delegate,
      realDevice: WgpuShim.realDevice,
      hw: hw
    });
    var info = {
      vendor: hw.vendor || "", architecture: "webgl2-angles",
      description: (hw.renderer || "") + " [wgpu-shim over WebGL2]",
      device: hw.renderer || "", name: "wgpu-shim-webgl2"
    };
    var adapter = new ShimAdapter(device, info);
    adapter._device = device;
    device.adapter = adapter;

    // Probe + record capabilities once.
    stats.caps.renderer = hw.renderer;
    stats.caps.vendor = hw.vendor;
    stats.caps.isHardware = hw.isHardware;
    stats.caps.maxTexture = hw.maxTexture;
    stats.caps.floatRenderable = hw.floatRenderable;
    stats.caps.maxElements = hw.maxTexture * hw.maxTexture * 4;
    stats.caps.u32Exact = probeU32Exact(hw);
    stats.caps.delegation = delegate;

    var gpuObject = {
      __wgpuShim: true,
      version: VERSION,
      requestAdapter: function (adapterOptions) {
        void adapterOptions;
        return Promise.resolve(adapter);
      },
      requestAdapterInfo: function () { return Promise.resolve(info); },
      getPreferredCanvasFormat: function () { return "bgra8unorm"; },
      wgslLanguageFeatures: { has: function () { return false; } }
    };

    var installed = false;
    try {
      Object.defineProperty(nav, "gpu", {
        configurable: true, enumerable: true, get: function () { return gpuObject; }
      });
      installed = nav.gpu === gpuObject;
    } catch (e1) {
      try { nav.gpu = gpuObject; installed = nav.gpu === gpuObject; }
      catch (e2) { installed = false; }
    }
    if (!installed) {
      report.reason = "navigator.gpu could not be replaced in this environment";
      return report;
    }

    WgpuShim._nativeGpu = nativeGpu || null;
    WgpuShim._gpuObject = gpuObject;
    WgpuShim.device = device;
    WgpuShim.adapter = adapter;
    WgpuShim.installed = true;
    stats.installs++;

    // Optionally go get a real device for delegation. This is deliberately
    // fire-and-forget: requestAdapter() can hang forever on some stacks (it
    // did in headless Chrome), and install() must never block on it.
    if (delegate && WgpuShim.realGpu && !WgpuShim.realDevice) {
      try {
        Promise.resolve(WgpuShim.realGpu.requestAdapter())
          .then(function (a) { return a ? a.requestDevice() : null; })
          .then(function (d) {
            if (!d) return;
            WgpuShim.realDevice = d;
            device._real = d;
            stats.caps.realDevice = true;
            if (d.lost && d.lost.then) {
              d.lost.then(function (info2) {
                stats.caps.realDeviceLost = (info2 && info2.reason) || "unknown";
                device._real = null;
                WgpuShim.realDevice = null;
              });
            }
          }, function (e) {
            stats.caps.realDeviceError = String((e && e.message) || e);
          });
      } catch (e) {
        stats.caps.realDeviceError = String((e && e.message) || e);
      }
    } else {
      device._real = WgpuShim.realDevice;
    }

    report.installed = true;
    report.adapter = adapter;
    report.device = device;
    report.info = info;
    report.caps = stats.caps;
    return report;
  };

  WgpuShim.uninstall = function () {
    if (!WgpuShim.installed) return false;
    var nav = global.navigator;
    try {
      if (WgpuShim._nativeGpu) {
        var ng = WgpuShim._nativeGpu;
        Object.defineProperty(nav, "gpu", {
          configurable: true, enumerable: true, get: function () { return ng; }
        });
      } else {
        delete nav.gpu;
      }
    } catch (e) {
      try { nav.gpu = WgpuShim._nativeGpu; } catch (e2) { return false; }
    }
    WgpuShim.installed = false;
    stats.uninstalls++;
    return true;
  };

  // Public translator entry point: useful for diagnostics and tests.
  WgpuShim.translateWgsl = function (code, entryPoint) {
    var r = translate(code, entryPoint);
    if (!r.ok) return { ok: false, reason: r.reason };
    return {
      ok: true, entryPoint: r.entryPoint, workgroupSize: r.workgroupSize,
      bindings: r.bindings.map(function (b) {
        return { name: b.name, group: b.group, binding: b.binding, access: b.access, elem: b.elem };
      }),
      writes: r.writes.map(function (b) { return b.name; }),
      common: r.common,
      bodies: r.bodies
    };
  };

  /* Self-test: runs the classic webgpucheck f32 shader and the ComputeProbe
     u32 shader through the shim without touching navigator.gpu. Handy on the
     console where you cannot open devtools. */
  WgpuShim.selfTest = function (options) {
    options = options || {};
    var out = { ok: false };
    try {
      var HW = global.HWCompute;
      if (!HW) throw new Error("HWCompute is not loaded");
      var hw = options.hw || WgpuShim.hw || new HW();
      WgpuShim.hw = hw;
      out.renderer = hw.renderer;
      out.isHardware = hw.isHardware;
      var device = new ShimDevice(null, { hw: hw, delegate: false, realDevice: null });

      // T1 shape
      var wgc = [
        "@group(0) @binding(0) var<storage, read_write> numbers: array<f32>;",
        "@compute @workgroup_size(1)",
        "fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {",
        "  let index = global_id.x;",
        "  numbers[index] = numbers[index] * 2.0;",
        "}"
      ].join("\n");
      var mod = device.createShaderModule({ code: wgc });
      var bgl = device.createBindGroupLayout({
        entries: [{ binding: 0, visibility: GPUShaderStage_.COMPUTE, buffer: { type: "storage" } }]
      });
      var input = new Float32Array([12.5, 45, 100.5, 0.25]);
      var buf = device.createBuffer({
        size: input.byteLength, mappedAtCreation: true,
        usage: GPUBufferUsage_.STORAGE | GPUBufferUsage_.COPY_SRC | GPUBufferUsage_.COPY_DST
      });
      new Float32Array(buf.getMappedRange()).set(input);
      buf.unmap();
      var readback = device.createBuffer({
        size: input.byteLength, usage: GPUBufferUsage_.COPY_DST | GPUBufferUsage_.MAP_READ
      });
      var bg = device.createBindGroup({ layout: bgl, entries: [{ binding: 0, resource: { buffer: buf } }] });
      var pl = device.createPipelineLayout({ bindGroupLayouts: [bgl] });
      var pipe = device.createComputePipeline({ layout: pl, compute: { module: mod, entryPoint: "main" } });
      var enc = device.createCommandEncoder();
      var pass = enc.beginComputePass();
      pass.setPipeline(pipe);
      pass.setBindGroup(0, bg);
      pass.dispatchWorkgroups(4);
      pass.end();
      enc.copyBufferToBuffer(buf, 0, readback, 0, input.byteLength);
      device.queue.submit([enc.finish()]);
      var got = new Float32Array(readback.getMappedRange());
      out.t1 = Array.prototype.join.call(got, ",");
      out.t1ok = got.length === 4;
      for (var i = 0; i < 4; i++) if (Math.abs(got[i] - input[i] * 2) > 0.001) out.t1ok = false;
      out.ok = out.t1ok;
      out.stats = {
        translated: stats.translated, refused: stats.refused, delegated: stats.delegated
      };
    } catch (e) {
      out.error = String((e && e.message) || e);
    }
    return out;
  };

  /* ==========================================================================
   * 10. exports
   * ======================================================================== */

  defineGlobal("GPUBufferUsage", GPUBufferUsage_);
  defineGlobal("GPUShaderStage", GPUShaderStage_);
  defineGlobal("GPUMapMode", GPUMapMode_);

  global.WgpuShim = WgpuShim;
})(typeof window !== "undefined" ? window : this);
