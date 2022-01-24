
#include "WindowsPrerequisites.h"
#include <cassert>
#include <sstream>


constexpr const TCHAR* WINDOW_CLASS_NAME = TEXT("RUNTIME_WINDOW");

[[nodiscard]] static HICON LoadApplicationIcon(const TCHAR* iconName, HINSTANCE hInstance, int iconSize)
{
    HICON result = reinterpret_cast<HICON>(
        ::LoadImage(
            hInstance,
            iconName,
            IMAGE_ICON,
            iconSize,
            iconSize,
            LR_DEFAULTSIZE | LR_SHARED));
    return result;
}

[[nodiscard]] static bool GetWindowClassInfo(WNDCLASSEX& windowClassEX, HINSTANCE hInstance)
{
    bool result = ::GetClassInfoEx(hInstance, WINDOW_CLASS_NAME, &windowClassEX) != 0;
    return result;
}

[[nodiscard]] static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SIZE:
    {
        RECT rc = {};
        ::GetClientRect(hWnd, &rc);
        std::stringstream ss;
        ss << "WM_SIZE(" << rc.right - rc.left << ", " << rc.bottom - rc.top << ")\n";
        ::OutputDebugStringA(ss.str().c_str());
    }
        break;
    }

    return ::DefWindowProc(
        hWnd,
        uMsg,
        wParam,
        lParam
    );
}

[[nodiscard]] static bool RegisterWindowClass(HINSTANCE hInstance)
{
    constexpr int largeIconSize = 64;
    constexpr int smallIconSize = 16;

    // 拡張ウィンドウクラスの設定
    WNDCLASSEX windowClassEX = {};

    if (!GetWindowClassInfo(windowClassEX, hInstance))
    {
        windowClassEX.cbSize = sizeof(WNDCLASSEX);
        windowClassEX.style = CS_OWNDC;
        windowClassEX.lpfnWndProc = &WindowProcedure;
        windowClassEX.cbClsExtra = 0;
        windowClassEX.cbWndExtra = 0;
        windowClassEX.hInstance = hInstance;
        windowClassEX.hIcon = LoadApplicationIcon(TEXT(""), hInstance, largeIconSize);
        windowClassEX.hIconSm = LoadApplicationIcon(TEXT(""), hInstance, smallIconSize);
        windowClassEX.hCursor = NULL;
        windowClassEX.hbrBackground = (HBRUSH)::GetStockObject(GRAY_BRUSH);
        windowClassEX.lpszMenuName = NULL;
        windowClassEX.lpszClassName = WINDOW_CLASS_NAME;
    }

    if (!::RegisterClassEx(&windowClassEX))
    {
        return false;
    }

    return true;
}

[[nodiscard]] static bool AdjustApplicationWindowRect(DWORD style, HWND hParentWindow, RECT& rect)
{
    // 指定されたクライアント領域を確保するために必要なウィンドウ座標を計算

    BOOL hasMenu = FALSE;
    UINT dpi = 0;

    if (hParentWindow != NULL)
    {
        if (style != WS_CHILD) { return false; }

        dpi = ::GetDpiForWindow(hParentWindow);

        if (::GetClientRect(hParentWindow, &rect) == 0)
        {
            return false;
        }
    }

    if (dpi == 0)
    {
        if (!::AdjustWindowRect(&rect, style, hasMenu))
        {
            return false;
        }
    }
    else
    {
        if (!::AdjustWindowRectExForDpi(&rect, style, hasMenu, 0, dpi))
        {
            return false;
        }
    }

    return true;
}

[[nodiscard]] static HWND CreateRuntimeWindow(
    HINSTANCE hInstance,
    HWND hParentWindow,
    DWORD style,
    int width,
    int height)
{
    // ウィンドウを生成
    HWND hWindow = ::CreateWindow(
        WINDOW_CLASS_NAME,
        TEXT("Win32"),
        style,
        0,
        0,
        width,
        height,
        hParentWindow,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWindow) { return NULL; }

    ::SetLastError(0);
    ::ShowWindow(hWindow, SW_SHOWNORMAL);
    if (::GetLastError()) { ::DestroyWindow(hWindow); return NULL; }

    ::SetLastError(0);
    ::UpdateWindow(hWindow);
    if (::GetLastError()) { ::DestroyWindow(hWindow); return NULL; }

    ::SetLastError(0);
    ::SetFocus(hWindow);
    if (::GetLastError()) { ::DestroyWindow(hWindow); return NULL; }

    return hWindow;
}

[[nodiscard]] static bool DoSystemEvents(HWND hWindow)
{
    if (::IsWindow(hWindow) != TRUE) { return false; }

    MSG msg = { 0 };
    while (::PeekMessage(&msg, hWindow, 0, 0, PM_REMOVE) != 0)
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return msg.message != WM_QUIT;
}

static void Update()
{

}

[[nodiscard]] static BOOL CALLBACK EnumChildWindowProcedure(HWND hWindow, LPARAM lParam)
{
    constexpr int MAX_LENGTH = 256;

    TCHAR windowText[MAX_LENGTH] = TEXT("");
    ::GetWindowText(hWindow, windowText, MAX_LENGTH);

    if (_tcscmp(windowText, TEXT("SUUDOG-Runtime")) == 0)
    {
        HWND* hParentWindow = reinterpret_cast<HWND*>(lParam);

        assert(hParentWindow);

        (*hParentWindow) = hWindow;

        return FALSE;
    }
    return TRUE;
}

[[nodiscard]] static BOOL CALLBACK EnumWindowProcedure(HWND hWindow, LPARAM lParam)
{
    constexpr int MAX_LENGTH = 256;

    TCHAR windowText[MAX_LENGTH] = TEXT("");
    ::GetWindowText(hWindow, windowText, MAX_LENGTH);

    if (_tcscmp(windowText, TEXT("SUUDOG-Editor")) == 0)
    {
        if (!::EnumChildWindows(hWindow, EnumChildWindowProcedure, lParam))
        {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

[[nodiscard]] static bool InitializeDpiAwareness()
{
    return ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE) == TRUE;
}

[[nodiscard]] static HWND SearchRuntimeHostWindow()
{
    HWND hRuntimeHostWindow = NULL;

    if (::EnumWindows(EnumWindowProcedure, reinterpret_cast<LPARAM>(&hRuntimeHostWindow)) != 0)
    {
        return hRuntimeHostWindow;
    }

    return NULL;
}

constexpr DWORD STANDALONE_WINDOW_STYLE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;

int main(int, const char* [])
{
    if (!InitializeDpiAwareness()) { return -1; }

    const HINSTANCE hInstance = ::GetModuleHandle(NULL);
    RECT rect = { 0, 0, 640, 480 };

    HWND hRuntimeHostWindow = SearchRuntimeHostWindow();

    const DWORD style = hRuntimeHostWindow != NULL ? WS_CHILD : STANDALONE_WINDOW_STYLE;

    if (!RegisterWindowClass(hInstance)) { return -1; }

    if (!AdjustApplicationWindowRect(style, hRuntimeHostWindow, rect)) { return -1; }

    HWND hWindow = CreateRuntimeWindow(
        hInstance,
        hRuntimeHostWindow,
        style,
        static_cast<int>(rect.right - rect.left),
        static_cast<int>(rect.bottom - rect.top));

    while (DoSystemEvents(hWindow))
    {
        Update();
    }

    return 0;
}
