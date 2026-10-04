// d3d11probe.cpp
// ---------------------------------------------------------------------------
// Native D3D11 COMPUTE probe for Xbox UWP.
//
// Answers one question that nothing else can: can a native D3D11 compute
// shader (cs_5_0) actually execute on this console's GPU, in this app
// container?
//
// Why it matters: WebGPU's Dawn D3D12 backend dies on this console when the
// driver executes a compute dispatch (DXGI_ERROR_DRIVER_INTERNAL_ERROR, GPU
// process exit_code=34). Meanwhile ANGLE's D3D11 path -- vertex/pixel shading
// only -- works fine. Compute shaders are a distinct pipeline stage, so
// "D3D11 works" does NOT prove "D3D11 compute works". This probe settles it.
//
// It reports, in detail:
//   * every DXGI adapter it can see, with a hardware/software classification
//   * which adapter it used, and the D3D11 feature level it got
//   * whether the device is hardware or the WARP software rasteriser
//   * the HLSL compile result
//   * the dispatch + readback result and whether the numbers are correct
//
// Exported to managed code as:
//   extern "C" int __stdcall RunD3D11ComputeProbe(wchar_t* out, int outChars)
// Returns 0 on success, non-zero on failure. `out` receives a report.
// ---------------------------------------------------------------------------

#include <windows.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include <cstdio>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace {

    const UINT kElements = 256;
    const UINT kThreadsPerGroup = 64;
    const UINT kGroups = kElements / kThreadsPerGroup;   // 4

    // Same shape as the WebGPU probe's shader, so the numbers are comparable.
    const char* kComputeShader =
        "RWStructuredBuffer<uint> outBuf : register(u0);\n"
        "[numthreads(64,1,1)]\n"
        "void main(uint3 tid : SV_DispatchThreadID)\n"
        "{\n"
        "    outBuf[tid.x] = tid.x * 2u + 1u;\n"
        "}\n";

    std::wstring g_log;

    void Put(const wchar_t* fmt, ...)
    {
        wchar_t buf[1024];
        va_list args;
        va_start(args, fmt);
        _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args);
        va_end(args);
        g_log += buf;
        g_log += L"\r\n";
    }

    void HR(const wchar_t* what, HRESULT hr)
    {
        Put(L"  %-38s hr=0x%08X %s", what, (unsigned)hr,
            SUCCEEDED(hr) ? L"OK" : L"FAIL");
    }

    const wchar_t* FeatureLevelName(D3D_FEATURE_LEVEL fl)
    {
        switch (fl) {
        case D3D_FEATURE_LEVEL_11_1: return L"11_1";
        case D3D_FEATURE_LEVEL_11_0: return L"11_0";
        case D3D_FEATURE_LEVEL_10_1: return L"10_1";
        case D3D_FEATURE_LEVEL_10_0: return L"10_0";
        case D3D_FEATURE_LEVEL_9_3:  return L"9_3";
        default:                     return L"(other)";
        }
    }

} // namespace

// Dependency-free probe: proves the DLL loads and is callable from the UWP app
// container WITHOUT touching D3D11. Call this before RunD3D11ComputeProbe so a
// failure can be attributed: if this returns but the compute probe kills the
// process, the problem is D3D11-in-container, not loading.
extern "C" __declspec(dllexport) int __stdcall ProbePing(wchar_t* out, int outChars)
{
    const wchar_t* msg = L"ProbePing OK - dll loaded, exported fn callable";
    if (out && outChars > 0) wcsncpy_s(out, (size_t)outChars, msg, _TRUNCATE);
    return 0;
}

extern "C" __declspec(dllexport) int __stdcall RunD3D11ComputeProbe(wchar_t* out, int outChars)
{
    g_log.clear();
    Put(L"=== native D3D11 COMPUTE probe ===");

    // ---- 1) enumerate adapters -------------------------------------------
    IDXGIFactory1* factory = nullptr;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
    HR(L"CreateDXGIFactory1", hr);
    if (FAILED(hr)) { goto done; }

    {
        IDXGIAdapter1* ad = nullptr;
        for (UINT i = 0; factory->EnumAdapters1(i, &ad) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 d = {};
            ad->GetDesc1(&d);
            bool software = (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
            Put(L"  adapter[%u] \"%s\" vendor=0x%04X device=0x%04X software=%s vram=%lluMB",
                i, d.Description, d.VendorId, d.DeviceId,
                software ? L"YES" : L"no",
                (unsigned long long)(d.DedicatedVideoMemory / (1024 * 1024)));
            ad->Release();
            ad = nullptr;
        }
    }

    // ---- 2) pick the first HARDWARE adapter ------------------------------
    IDXGIAdapter1* chosen = nullptr;
    {
        IDXGIAdapter1* ad = nullptr;
        for (UINT i = 0; factory->EnumAdapters1(i, &ad) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 d = {};
            ad->GetDesc1(&d);
            if ((d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) { chosen = ad; break; }
            ad->Release();
            ad = nullptr;
        }
    }
    if (!chosen) { Put(L"  NO HARDWARE ADAPTER FOUND"); }

    // ---- 3) create the D3D11 device --------------------------------------
    D3D_FEATURE_LEVEL want[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL got = D3D_FEATURE_LEVEL_9_3;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;

    hr = D3D11CreateDevice(
        chosen, chosen ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, want, _countof(want), D3D11_SDK_VERSION,
        &dev, &got, &ctx);
    HR(L"D3D11CreateDevice (hardware)", hr);
    if (FAILED(hr))
    {
        Put(L"  retrying with D3D_DRIVER_TYPE_WARP ...");
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
            want, _countof(want), D3D11_SDK_VERSION, &dev, &got, &ctx);
        HR(L"D3D11CreateDevice (WARP)", hr);
    }
    if (FAILED(hr)) { Put(L"  cannot create a D3D11 device at all"); goto done; }

    Put(L"  feature level: %s", FeatureLevelName(got));

    // Confirm which adapter the device actually bound to.
    {
        IDXGIDevice* dxgiDev = nullptr;
        if (SUCCEEDED(dev->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDev)))
        {
            IDXGIAdapter* a = nullptr;
            if (SUCCEEDED(dxgiDev->GetAdapter(&a)))
            {
                // GetDesc1/DXGI_ADAPTER_DESC1 is the one carrying Flags.
                IDXGIAdapter1* a1 = nullptr;
                DXGI_ADAPTER_DESC1 d1 = {};
                if (SUCCEEDED(a->QueryInterface(__uuidof(IDXGIAdapter1), (void**)&a1)) &&
                    SUCCEEDED(a1->GetDesc1(&d1)))
                {
                    Put(L"  device is bound to: \"%s\"  software=%s",
                        d1.Description,
                        (d1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) ? L"YES (WARP)" : L"no (hardware)");
                }
                if (a1) a1->Release();
                a->Release();
            }
            dxgiDev->Release();
        }
    }

    // Compute shader support is the whole point.
    Put(L"  CS_5_0 supported: %s",
        dev->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 ? L"yes (needs 11_0)" :
        (dev->GetFeatureLevel() >= D3D_FEATURE_LEVEL_10_0 ? L"yes (needs 10_0)" : L"no"));

    // ---- 4) compile the compute shader -----------------------------------
    ID3DBlob* code = nullptr;
    ID3DBlob* errs = nullptr;
    hr = D3DCompile(kComputeShader, strlen(kComputeShader), "probe",
        nullptr, nullptr, "main", "cs_5_0", 0, 0, &code, &errs);
    HR(L"D3DCompile(cs_5_0)", hr);
    if (FAILED(hr))
    {
        if (errs) Put(L"  compiler said: %S", (char*)errs->GetBufferPointer());
        goto done;
    }

    {
        ID3D11ComputeShader* cs = nullptr;
        hr = dev->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &cs);
        HR(L"CreateComputeShader", hr);
        if (FAILED(hr)) goto done;

        // ---- 5) buffers --------------------------------------------------
        // out (UAV) + staging (CPU readable)
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = kElements * sizeof(UINT);
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bd.StructureByteStride = sizeof(UINT);
        ID3D11Buffer* outBuf = nullptr;
        hr = dev->CreateBuffer(&bd, nullptr, &outBuf);
        HR(L"CreateBuffer(out, structured UAV)", hr);
        if (FAILED(hr)) goto done;

        D3D11_BUFFER_DESC sd = bd;
        sd.Usage = D3D11_USAGE_STAGING;
        sd.BindFlags = 0;
        sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ID3D11Buffer* staging = nullptr;
        hr = dev->CreateBuffer(&sd, nullptr, &staging);
        HR(L"CreateBuffer(staging)", hr);
        if (FAILED(hr)) goto done;

        D3D11_UNORDERED_ACCESS_VIEW_DESC ud = {};
        ud.Format = DXGI_FORMAT_UNKNOWN;
        ud.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        ud.Buffer.FirstElement = 0;
        ud.Buffer.NumElements = kElements;
        ID3D11UnorderedAccessView* uav = nullptr;
        hr = dev->CreateUnorderedAccessView(outBuf, &ud, &uav);
        HR(L"CreateUnorderedAccessView", hr);
        if (FAILED(hr)) goto done;

        // ---- 6) DISPATCH -------------------------------------------------
        ctx->CSSetShader(cs, nullptr, 0);
        ctx->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
        ctx->Dispatch(kGroups, 1, 1);
        Put(L"  Dispatch(%u,1,1) issued", kGroups);

        ctx->CopyResource(staging, outBuf);

        // ---- 7) read back -------------------------------------------------
        D3D11_MAPPED_SUBRESOURCE ms = {};
        hr = ctx->Map(staging, 0, D3D11_MAP_READ, 0, &ms);
        HR(L"Map(staging, READ)", hr);
        if (SUCCEEDED(hr))
        {
            UINT* p = (UINT*)ms.pData;
            bool ok = true;
            int firstBad = -1;
            for (UINT i = 0; i < kElements; ++i)
            {
                if (p[i] != (i * 2u + 1u)) { ok = false; firstBad = (int)i; break; }
            }
            Put(L"  first 6 = [%u,%u,%u,%u,%u,%u]  expected [1,3,5,7,9,11]",
                p[0], p[1], p[2], p[3], p[4], p[5]);
            Put(L"  COMPUTE %s%s", ok ? L"PASSED" : L"FAILED",
                ok ? L"" : L" (mismatch)");
            if (!ok) Put(L"  first mismatch at index %d", firstBad);
            ctx->Unmap(staging, 0);
            if (ok) Put(L"  RESULT: native D3D11 compute WORKS on this console");
        }

        if (uav) uav->Release();
        if (staging) staging->Release();
        if (outBuf) outBuf->Release();
        cs->Release();
    }

done:
    if (code) code->Release();
    if (errs) errs->Release();
    if (ctx) ctx->Release();
    if (dev) dev->Release();
    if (chosen) chosen->Release();
    if (factory) factory->Release();

    // copy out
    if (out && outChars > 0)
    {
        wcsncpy_s(out, (size_t)outChars, g_log.c_str(), _TRUNCATE);
    }
    return g_log.find(L"COMPUTE PASSED") != std::wstring::npos ? 0 : 1;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
