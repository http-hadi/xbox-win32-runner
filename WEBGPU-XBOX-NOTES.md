# WebGPU on Xbox — verified constraints (1.6.5.0 / ladder gen7)

Everything below was measured on the console itself (Xbox Series X, SystemOS
`26100.9438`), through the Device Portal, on 2026-10-03. No claim here is
inferred from documentation alone.

## The one-line summary

**WebGPU compute cannot run on the Xbox GPU. SwiftShader is the only Xbox path
on which WebGPU — including compute and `mapAsync` — actually works.**

WebGL and compositing do run on the console GPU (ANGLE over D3D11On12). That
part was always true and is unchanged.

## Backend-by-backend evidence

| WebGPU backend | Flags | Result |
| --- | --- | --- |
| Dawn **D3D12** (`kDefault`) | `--ignore-gpu-blocklist --enable-unsafe-webgpu` | Adapter is created and reports `vendor=microsoft`, but the **first compute dispatch destroys the device**. |
| Dawn **D3D11** | `--use-webgpu-adapter=d3d11 --use-angle=d3d11` | **No device at all** — the probe verdict falls straight through to `fallback:swiftshader google`. |
| Dawn **OpenGLES** | `--use-webgpu-adapter=opengles` | Compiled out of the win-UWP WebView2 runtime. |
| **SwiftShader** | `--use-webgpu-adapter=swiftshader` | **Fully working**, every stage of the compute ladder passes. |

### The D3D12 failure, verbatim

```
STAGE dispatch FAILED (device lost after 1215ms):
  ID3D12Device::CreateHeap failed with DXGI_ERROR_DEVICE_REMOVED (0x887A0005)
   - While calling [Device].CreateBuffer([BufferDescriptor]).
   at CheckHRESULTImpl (..\..\third_party\dawn\src\dawn\native\d3d\D3DError.cpp:119)
  Backend messages:
   * Device removed reason: DXGI_ERROR_DRIVER_INTERNAL_ERROR (0x887A0020)
```

It is *reproducible*: an identical failure at the identical point (1215 ms into
the `dispatch` stage) in every run.

### Why a compute crash breaks far more than WebGPU

The same instant that the WebGPU device dies, **ANGLE's D3D11 device is removed
too**:

```
ERROR:ui\gl\angle_platform_impl.cc:47] Renderer11.cpp:2251
  (virtual rx::Renderer11::testDeviceLost): The D3D11 device was removed, HRESULT: 0x887A0005
```

So one WebGPU compute dispatch does not merely fail that call — it takes the
whole GPU stack down for the process, **WebGL included**. Exposing a
compute-capable-looking WebGPU on the D3D12 rung actively *destabilised* the
browser.

## What was actually wrong in 1.6.4.0 (gen6)

The ladder correctly defaulted to SwiftShader, but its software-detection
name test was:

```js
/swiftshader|software|llvmpipe|basic render|warp/i
```

The Xbox D3D12 adapter reports `vendor = "microsoft"` with an **empty**
description and architecture (Chromium masks those fields), so it matched
**nothing** in that expression and was classified as *hardware*. The result was
a green badge reading:

> **WebGPU: hardware ✓**
> adapter: GPU · WebGL: hardware

…over an adapter on which no compute dispatch can survive. That is the exact
"it's green and says hardware, but compute fails" state users reported.

## What 1.6.5.0 (gen7) changes

1. `microsoft` is now recognised by the software test in **both** the C# startup
   probe and the on-page badge, so the D3D12 adapter can never again be
   presented as hardware. The badge now reads
   `WebGPU: software · Microsoft (no compute)`.
2. Because that adapter is classified as a fallback, the adaptive ladder
   steers away from it automatically instead of settling on it.
3. The status/pin text now states the verified facts, including that Dawn's
   D3D11 backend was **re-tested against a working probe** and yields no device.
4. `CurrentLadderGeneration` bumped to `gen7-1.6.5.0` so existing consoles
   re-evaluate once with the corrected classification.
5. `WindowsMobile` `SDKReference` now tracks `$(TargetPlatformVersion)` instead
   of being pinned to `10.0.19041.0`. That pinned version was the single error
   (`MSB3774`) blocking every local build on a machine carrying SDK 22000/22621.

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

WebGPU is not a supported WebView2 feature on Xbox. See
[WebView2Feedback discussion #4138](https://github.com/MicrosoftEdge/WebView2Feedback/discussions/4138),
where the reported symptom is `navigator.gpu` existing but yielding no device.
This build deliberately runs WebGPU on SwiftShader rather than exposing a
device the console GPU driver cannot service.
