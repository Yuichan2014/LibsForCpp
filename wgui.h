#pragma once
// ============================================================
//  wgui.h  --  Header-only OOP wrapper for Win32 windows.
//  Usage:  #include "wgui.h"
//          Derive from Window and override hooks, or use MapMessage().
//
//  Note on naming:
//    The member that attaches to an existing HWND is called "Attach"
//    instead of "SubclassWindow", because <commctrl.h> defines
//    SubclassWindow as a macro and it would clash at parse time.
// ============================================================

#include <windows.h>
#include <windowsx.h>
#include <functional>
#include <unordered_map>
#include <string>
#include <utility>

class Window
{
public:
    Window() = default;

    virtual ~Window()
    {
        if (m_hwnd && m_ownsWindow && ::IsWindow(m_hwnd))
        {
            // Unbind "this" first so that WM_NCDESTROY cannot reach a
            // partially-destroyed object.
            ::SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0);
            ::DestroyWindow(m_hwnd);
        }
    }

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    // ---------------- Create / Show ----------------
    bool Create(const std::wstring& title,
                DWORD dwStyle = WS_OVERLAPPEDWINDOW,
                int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
                int w = 800, int h = 600,
                HWND parent = nullptr,
                HINSTANCE hInstance = nullptr)
    {
        if (!hInstance) hInstance = ::GetModuleHandleW(nullptr);
        if (!EnsureClassRegistered(hInstance)) return false;

        // "this" is forwarded through lpParam and picked up in WM_NCCREATE.
        m_hwnd = ::CreateWindowExW(
            0, kClassName, title.c_str(), dwStyle,
            x, y, w, h,
            parent, nullptr, hInstance, this);

        if (!m_hwnd) return false;
        m_ownsWindow = (parent == nullptr);
        return true;
    }

    // ---------------- Attach existing window ----------------
    // Attach to an existing HWND (subclassing), e.g. a system control.
    // Named "Attach" to avoid the SubclassWindow macro from <commctrl.h>.
    bool Attach(HWND hwnd)
    {
        if (!hwnd || !::IsWindow(hwnd)) return false;
        if (m_hwnd)                     return false;  // Already attached.

        m_hwnd       = hwnd;
        m_ownsWindow = false;
        m_prevProc   = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(&Window::StaticWndProc)));
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(this));
        return true;
    }

    void Show(int nCmdShow = SW_SHOW)
    {
        if (m_hwnd) { ::ShowWindow(m_hwnd, nCmdShow); ::UpdateWindow(m_hwnd); }
    }

    void Update() const { if (m_hwnd) ::UpdateWindow(m_hwnd); }

    HWND Handle() const { return m_hwnd; }

    // ---------------- Static message loop ----------------
    static int RunMessageLoop()
    {
        MSG msg{};
        while (::GetMessageW(&msg, nullptr, 0, 0) > 0)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        return static_cast<int>(msg.wParam);
    }

protected:
    // ---------------- Overridable event hooks ----------------
    virtual void    OnCreate()              {}
    virtual void    OnDestroy()             {}
    virtual void    OnNcDestroy()           {}
    virtual void    OnPaint(HDC)            {}
    virtual void    OnSize(int, int)        {}
    virtual void    OnLButtonDown(int, int) {}
    virtual void    OnKeyDown(WPARAM)       {}

    virtual LRESULT OnClose()
    {
        if (m_hwnd) ::DestroyWindow(m_hwnd);
        return 0;
    }

    virtual LRESULT OnDefault(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // If attached via Attach(), forward to the original window procedure.
        // Otherwise, use DefWindowProcW.
        if (m_prevProc)
            return ::CallWindowProcW(m_prevProc, m_hwnd, msg, wParam, lParam);
        return ::DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    // ---------------- Message-map API ----------------
    using Handler = std::function<LRESULT(WPARAM, LPARAM)>;
    void MapMessage(UINT msg, Handler handler)
    {
        m_messageMap[msg] = std::move(handler);
    }

    HWND m_hwnd       = nullptr;
    bool m_ownsWindow = false;

private:
    // ---------------- Window class registration ----------------
    static bool EnsureClassRegistered(HINSTANCE hInstance)
    {
        static bool s_registered = false;
        if (s_registered) return true;

        WNDCLASSEXW wc{ sizeof(WNDCLASSEXW) };
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = &Window::StaticWndProc;
        wc.hInstance     = hInstance;
        wc.hCursor       = ::LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = kClassName;
        wc.hIcon         = ::LoadIconW(nullptr, IDI_APPLICATION);

        s_registered = (::RegisterClassExW(&wc) != 0);
        return s_registered;
    }

    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        Window* self = nullptr;

        if (msg == WM_NCCREATE)
        {
            // WM_NCCREATE is the only place where lpCreateParams is available.
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<Window*>(cs->lpCreateParams);
            self->m_hwnd = hwnd;
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<Window*>(
                ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
            return self->Dispatch(msg, wParam, lParam);

        return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT Dispatch(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // 1) User-registered message handlers take priority.
        if (auto it = m_messageMap.find(msg); it != m_messageMap.end())
            return it->second(wParam, lParam);

        // 2) Virtual hooks.
        switch (msg)
        {
        case WM_CREATE:      OnCreate();                                     return 0;
        case WM_DESTROY:     OnDestroy();                                    return 0;
        case WM_CLOSE:       return OnClose();
        case WM_NCDESTROY:
        {
            // Unbind "this" so the pointer is not left dangling.
            ::SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0);
            OnNcDestroy();
            return OnDefault(msg, wParam, lParam);
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(m_hwnd, &ps);
            OnPaint(hdc);
            ::EndPaint(m_hwnd, &ps);
            return 0;
        }
        case WM_SIZE:        OnSize(LOWORD(lParam), HIWORD(lParam));         return 0;
        case WM_LBUTTONDOWN: OnLButtonDown(GET_X_LPARAM(lParam),
                                          GET_Y_LPARAM(lParam));           return 0;
        case WM_KEYDOWN:     OnKeyDown(wParam);                              return 0;
        }

        // 3) Fall back to default handling.
        return OnDefault(msg, wParam, lParam);
    }

    // Unique window class name shared by all Window-derived instances.
    static constexpr const wchar_t* kClassName = L"WindowClass_OOP_HeaderOnly";

    std::unordered_map<UINT, Handler> m_messageMap;
    WNDPROC m_prevProc = nullptr;   // Original proc when attached via Attach()
};
