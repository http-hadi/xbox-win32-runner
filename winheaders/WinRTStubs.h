// winheaders/WinRTStubs.h
// Stub C++/WinRT projection headers for Linux syntax-checking.
//
// Real Windows / Xbox builds use the Microsoft.Windows.CppWinRT NuGet package
// to generate these projections from .winmd metadata. On Linux we don't have
// that, so we hand-stub the minimal types and methods our App.cpp uses.
//
// The stubs are functionally complete enough that the REAL App.cpp code path
// (not the #ifdef XWR_SYNTAX_CHECK stub) compiles AND links under g++, so we
// can syntax-check the real implementation, not a fake one.

#pragma once
#include <Windows.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

// IID is already typedef'd as GUID in Windows.h.
// STDMETHODCALLTYPE is __stdcall (no-op on Linux).

namespace winrt {
namespace Windows {
namespace Foundation {

// IAsyncAction — stub. App.cpp doesn't actually await; it just fire-and-forgets.
struct IAsyncAction {
    void get() {}
};

// Forward-declare IAsyncAction for CoreDispatcher (which returns it).
// (Already defined above; this is just a note.)

// TypedEventHandler<TSender, TArgs> — a callable wrapper.
// In real C++/WinRT this is a delegate; we just use std::function.
template <typename TSender, typename TArgs>
using TypedEventHandler = std::function<void(const TSender&, const TArgs&)>;

// Deferral — stub.
struct Deferral {};

// CharacterReceivedEventArgs stub.
struct CharacterReceivedEventArgs {
    UINT CodePoint = 0;
};

}  // namespace Foundation

namespace UI {
namespace Core {

// Forward-declare CoreWindow so KeyEventArgs can reference it.
struct CoreWindow;

// KeyEventArgs stub — provides VirtualKey, scan code, etc.
struct KeyEventArgs {
    UINT VirtualKey = 0;
    UINT ScanCode   = 0;
    BOOL WasKeyDown = FALSE;
};

// CoreDispatchPriority — used by CoreDispatcher::RunAsync.
enum class CoreDispatchPriority : int {
    Normal = 0,
    High   = 1,
    Low    = 2,
};

// CoreDispatcher stub. Defined after Foundation::IAsyncAction.
struct CoreDispatcher {
    template <typename F>
    Foundation::IAsyncAction RunAsync(CoreDispatchPriority, F&& fn) {
        fn();  // synchronous for the stub
        return Foundation::IAsyncAction{};
    }
};

// ICoreWindowInterop — the undocumented-but-stable COM interface for getting
// an HWND from a CoreWindow. Real builds #include <corewindow.h> or use
// the IID directly. We provide a minimal abstract base (NOT deriving from
// the stub IUnknown, which is a DWORD typedef in our stub Windows.h).
struct ICoreWindowInterop {
    virtual HRESULT STDMETHODCALLTYPE get_WindowHandle(HWND* hwnd) = 0;
    virtual ~ICoreWindowInterop() = default;
};

// CoreWindow stub. The real type has many more methods; we stub the ones
// App.cpp actually calls.
struct CoreWindow {
    // Static factory.
    static CoreWindow GetForCurrentThread() {
        return CoreWindow{};
    }

    void Activate() {}
    CoreDispatcher Dispatcher() { return CoreDispatcher{}; }

    // Event subscription — real C++/WinRT uses +=; we store the handler.
    Foundation::TypedEventHandler<CoreWindow, KeyEventArgs> m_keyDown;
    Foundation::TypedEventHandler<CoreWindow, KeyEventArgs> m_keyUp;
    Foundation::TypedEventHandler<CoreWindow, Foundation::CharacterReceivedEventArgs> m_charReceived;

    void KeyDown(Foundation::TypedEventHandler<CoreWindow, KeyEventArgs> handler) {
        m_keyDown = handler;
    }
    void KeyUp(Foundation::TypedEventHandler<CoreWindow, KeyEventArgs> handler) {
        m_keyUp = handler;
    }
    void CharacterReceived(Foundation::TypedEventHandler<CoreWindow, Foundation::CharacterReceivedEventArgs> handler) {
        m_charReceived = handler;
    }

    // COM interop — `as<T>()` in real C++/WinRT queries for an interface.
    // For the stub, we return a fake ICoreWindowInterop whose
    // get_WindowHandle returns a sentinel HWND so D3D11Bridge::Initialize
    // receives a non-null window.
    struct StubInterop : public ICoreWindowInterop {
        HRESULT STDMETHODCALLTYPE get_WindowHandle(HWND* hwnd) override {
            if (hwnd) *hwnd = static_cast<HWND>(0xDEAD);
            return S_OK;
        }
    };

    // Returns a heap-allocated StubInterop. Real C++/WinRT returns a smart
    // com_ptr; we use a shared_ptr since the stub doesn't track refcounts.
    // The template parameter T should be ICoreWindowInterop; we always return
    // a StubInterop (which is-a ICoreWindowInterop).
    template <typename T>
    std::shared_ptr<T> as() const {
        // std::make_shared<T> would fail if T is abstract (ICoreWindowInterop
        // has a pure virtual). Return a StubInterop cast to T*.
        return std::shared_ptr<T>(reinterpret_cast<T*>(new StubInterop()));
    }
};

}  // namespace Core
}  // namespace UI

namespace Storage {

struct IStorageFolder {};

struct ApplicationData {
    static ApplicationData Current() { return ApplicationData{}; }
    IStorageFolder LocalFolder() { return IStorageFolder{}; }
};

}  // namespace Storage

namespace Gaming {
namespace Input {

// GamepadReading — matches the real WinRT struct.
struct GamepadReading {
    uint64_t Timestamp;
    uint16_t Buttons;
    double LeftThumbstickX;
    double LeftThumbstickY;
    double RightThumbstickX;
    double RightThumbstickY;
    double LeftTrigger;
    double RightTrigger;
};

// GamepadButtons bitmask — matches real WinRT enum.
enum class GamepadButtons : uint32_t {
    None           = 0,
    Menu           = 1,
    View           = 2,
    A              = 4,
    B              = 8,
    X              = 16,
    Y              = 32,
    DPadUp         = 64,
    DPadDown       = 128,
    DPadLeft       = 256,
    DPadRight      = 512,
    LeftShoulder   = 1024,
    RightShoulder  = 2048,
    LeftThumbstick = 4096,
    RightThumbstick= 8192,
};

struct GamepadVibration {
    double LeftMotor;
    double RightMotor;
    double LeftTrigger;
    double RightTrigger;
};

// Gamepad stub.
struct Gamepad {
    static std::vector<Gamepad> Gamepads() { return {}; }
    GamepadReading GetCurrentReading() const { return GamepadReading{}; }
    void SetVibration(GamepadVibration) {}
};

}  // namespace Input
}  // namespace Gaming

}  // namespace Windows
}  // namespace winrt

// Convenience: real C++/WinRT uses `winrt::com_ptr<T>` for smart COM pointers.
// We provide a minimal stub.
namespace winrt {
template <typename T>
struct com_ptr {
    T* p = nullptr;
    T* operator->() const { return p; }
    T** put() { return &p; }
    void* get() const { return p; }
    explicit operator bool() const { return p != nullptr; }
};
}  // namespace winrt
