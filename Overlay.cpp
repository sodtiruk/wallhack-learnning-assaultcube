#include "Overlay.h"

LRESULT CALLBACK Overlay::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(
        hwnd,
        msg,
        wParam,
        lParam);
}

bool Overlay::Create(HWND targetWindow)
{
    target = targetWindow;

    if (!target)
        return false;

    hInstance = GetModuleHandle(nullptr);

    const char *className = "SimpleOverlay";

    WNDCLASSA wc{};

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;

    RegisterClassA(&wc);

    RECT rect;

    if (!GetWindowRect(target, &rect))
        return false;

    int width =
        rect.right - rect.left;

    int height =
        rect.bottom - rect.top;

    hwnd = CreateWindowExA(
        WS_EX_LAYERED |
            WS_EX_TRANSPARENT |
            WS_EX_TOPMOST |
            WS_EX_NOACTIVATE,

        className,
        "Overlay",

        WS_POPUP,

        rect.left,
        rect.top,

        width,
        height,

        nullptr,
        nullptr,

        hInstance,
        nullptr);

    if (!hwnd)
        return false;

    // สีดำจะกลายเป็น transparent
    SetLayeredWindowAttributes(
        hwnd,
        RGB(0, 0, 0),
        0,
        LWA_COLORKEY);

    ShowWindow(hwnd, SW_SHOW);

    UpdateWindow(hwnd);

    return true;
}

bool Overlay::Update()
{
    MSG msg;

    while (PeekMessage(
        &msg,
        nullptr,
        0,
        0,
        PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // ทำให้ Overlay ตาม Game Window
    RECT rect;

    if (GetWindowRect(target, &rect))
    {
        SetWindowPos(
            hwnd,
            HWND_TOPMOST,

            rect.left,
            rect.top,

            rect.right - rect.left,
            rect.bottom - rect.top,

            SWP_NOACTIVATE);
    }

    return true;
}

void Overlay::DrawBox(int x1, int y1, int x2, int y2)
{
    HDC hdc = GetDC(hwnd);

    if (!hdc)
        return;

    HPEN pen = CreatePen(
        PS_SOLID,
        3,
        RGB(255, 0, 0));

    HPEN oldPen =
        (HPEN)SelectObject(hdc, pen);

    HBRUSH oldBrush =
        (HBRUSH)SelectObject(
            hdc,
            GetStockObject(NULL_BRUSH));

    Rectangle(
        hdc,
        x1,
        y1,
        x2,
        y2);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);

    DeleteObject(pen);

    ReleaseDC(hwnd, hdc);
}

HWND Overlay::GetHandle() const
{
    return hwnd;
}