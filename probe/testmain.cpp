// testmain.cpp - local desktop harness for d3d11probe.cpp
#include <windows.h>
#include <cstdio>

extern "C" __declspec(dllimport) int __stdcall RunD3D11ComputeProbe(wchar_t* out, int outChars);

int wmain()
{
    static wchar_t buf[16384];
    buf[0] = 0;
    int rc = RunD3D11ComputeProbe(buf, _countof(buf));

    // UTF-8 report so it is easy to read from PowerShell
    FILE* f = nullptr;
    if (_wfopen_s(&f, L"probe_report.txt", L"wb, ccs=UTF-8") == 0 && f)
    {
        fwprintf(f, L"%s\r\nrc=%d\r\n", buf, rc);
        fclose(f);
    }
    wprintf(L"%s\nrc=%d\n", buf, rc);
    return rc;
}
