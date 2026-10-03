# WebGPU on Xbox — measured constraints and the working hardware-compute path

Everything here was measured on the console itself (Xbox Series X, SystemOS
`26100.9438`, Edge WebView2 `150.0.7871.212`, Chromium `150.0.7871.212`)
through the Device Portal, on 2026-10-03. Nothing is inferred from
documentation.

## Summary

**WebGPU cannot execute a compute dispatch on this console.** Every hardware
backend is closed off. The console GPU itself is fine and fully usable — it is
simply not reachable through `navigator.gpu`. Hardware compute *is* available
through ANGLE/D3D11 (the stack WebGL already uses), which is what
`hwcompute.js` + `HwCompute.html` in this folder provide.

## Backend-by-backend evidence

| WebGPU backend | Flags | Result |
| --- | --- | --- |
| Dawn **D3D12** (`kDefault`) | `--ignore-gpu-blocklist --enable-unsafe-webgpu` | Adapter created (reports `vendor=microsoft`), compute PSO created, buffers created, **command buffer submitted successfully** — then the GPU process dies when the driver executes the work. |
| Dawn **D3D11** | `--use-webgpu-adapter=d3d11 --use-angle=d3d11` | **No device at all.** Re-tested against a working probe: verdict `fallback:swiftshader google`. |
| Dawn **OpenGLES** | `--use-webgpu-adapter=opengles` | Compiled out of the win-UWP WebView2 runtime. |
| **SwiftShader** | `--use-webgpu-adapter=swiftshader` | Fully working — every compute stage passes. CPU only. |

## The exact failure sequence (this is the important part)

Breadcrumbs from `chromium.log`, D3D12 rung, one fresh device:

```
13:49:22.664  dispatch: PSO created (sync)                        OK
13:49:22.664  dispatch: createBuffer input   (STORAGE|COPY_DST)   OK
13:49:22.665  dispatch: createBuffer output  (STORAGE|COPY_SRC)   OK
13:49:22.665  dispatch: createBuffer readback(MAP_READ|COPY_DST)  OK
13:49:22.666  dispatch: queue.writeBuffer(input)                  OK
13:49:22.667  dispatch: createBindGroup                           OK
13:49:22.667  dispatch: encode compute pass, dispatchWorkgroups(4) OK
13:49:22.668  dispatch: SUBMITTED                                 OK
13:49:22.690  ERROR Renderer11::testDeviceLost: D3D11 device removed, HRESULT 0x887A0005
13:49:22.690  ID3D12Device::CreateHeap failed with DXGI_ERROR_DEVICE_REMOVED
              Device removed reason: DXGI_ERROR_DRIVER_INTERNAL_ERROR (0x887A0020)
13:49:22.712  GPU process exited unexpectedly: exit_code=34
```

Two conclusions that are easy to get wrong:

1. **The `CreateHeap` / `CreateBuffer` message is a red herring.** It is not the
   failing operation — it is merely where Dawn *next touched* an already-dead
   device. Everything, including `queue.submit()`, returned successfully. The
   device dies ~22 ms after submit, when the driver actually runs the compute
   work. So this is not a validation error, not a heap-allocation bug and not a
   probe artefact: executing compute on this driver is fatal.
2. **It is not just WebGPU that dies.** In the same millisecond ANGLE reports
   `Renderer11::testDeviceLost` with the same HRESULT and Chromium logs
   `SharedContextState context lost via EXT_robustness`. The removal is
   *adapter-wide*, so the page's WebGL context is destroyed too
   (`WebGL: CONTEXT_LOST_WEBGL`). Exposing a compute-capable-looking WebGPU on
   the D3D12 rung actively destabilised the browser.

An earlier crash in the same session — 1 ms after `device.destroy()` on a device
that had only ever had a compute PSO created — shows the same signature, so the
trigger is the compute-PSO lifecycle, not the dispatch alone.

## The 1.6.4.0 (gen6) bug this exposed

The ladder correctly defaulted to SwiftShader, but the software-detection name
test in both the badge and the startup probe was:

```js
/swiftshader|software|llvmpipe|basic render|warp/i
```

The Xbox D3D12 adapter reports `vendor = "microsoft"` with an **empty**
description and architecture (Chromium masks those fields), so it matched
nothing and was classified as *hardware*. The badge therefore painted a green

> **WebGPU: hardware ✓**
> adapter: GPU · WebGL: hardware

over an adapter that cannot survive one compute dispatch. That is the
"it's green and says hardware, but compute fails" state.

**1.6.5.0 (gen7)** matches `microsoft`/`basicrender` in both places. The badge
now reads `WebGPU: software · Microsoft (no compute)`, the adapter is a
*fallback* verdict so the adaptive ladder steers away from it, and
`ComputeProbe` aborts instead of running the crashing ladder.

## Hardware compute that actually works

`hwcompute.js` implements GPU compute as fragment-shader passes on a hardware
**WebGL2** context — i.e. ANGLE → D3D11 → D3D11On12 → the console GPU. That
stack demonstrably works on Xbox (`IDCompositionTexture is not supported on
11on12 devices` confirms ANGLE is on the console's D3D11On12 layer, and the
WebGL badge reports hardware).

Verified (locally, on the same ANGLE/D3D11 stack):

```
COMPUTE PASSED ON HARDWARE GPU
renderer: ANGLE (Intel, Intel(R) HD Graphics 520 ... Direct3D11 vs_5_0 ps_5_0, D3D11)
Kernel out[i]=a[i]*2+1 : all 1048576 values correct
```

Data is held 4 floats per `RGBA32F` texel (the spec only guarantees
`readPixels` for RGBA32F). Kernels are written as:

```glsl
void kernel(inout vec4 io, int i) {
  io.x = elemA(i) * 2.0 + 1.0;      // elemA / elemB read the two inputs
}
```

Limitations, stated plainly: element-wise and gather kernels only. There is no
workgroup shared memory and no `workgroupBarrier()`, because a fragment-shader
pass has no equivalent. Kernels needing intra-workgroup communication cannot be
expressed.

## Open question

The gen6/gen7 probe builds a **brand-new adapter+device for every stage**, and
in the long ladder runs the crash always landed on the 5th device. That raised
the possibility that the failure is resource exhaustion rather than compute. It
is not: the sequence above is a *single fresh device* whose first compute
dispatch killed the GPU process. The single-device run (`stages:["full"]`) is
still queued to confirm on-console.

## Controls (Device Portal → `LocalAppData/<package>/LocalState`)

| File | Values |
| --- | --- |
| `webgpu-mode.txt` | `auto` (or delete) · `auto-cpu` · `auto-d3d12` · `d3d12` · `d3d11` · `gles` |
| `browser-flags.txt` | Verbatim browser arguments; overrides every built-in flag |
| `webgpu-status.txt` | Every probe decision, appended each launch |
| `chromium.log` | Chromium + `PROBE` breadcrumbs |

Pinning `d3d12` gives hardware WebGPU *rendering* and a browser whose WebGL dies
on the first compute site. It is a diagnostics setting, not a usable mode.

## Microsoft's position

WebGPU is not a supported WebView2 feature on Xbox — see
[WebView2Feedback discussion #4138](https://github.com/MicrosoftEdge/WebView2Feedback/discussions/4138),
where the reported symptom is `navigator.gpu` existing but yielding no device.
