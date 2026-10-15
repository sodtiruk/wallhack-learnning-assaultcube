#pragma once

#include <Windows.h>

class Overlay
{
private:
    HWND hwnd = nullptr;
    HWND target = nullptr;

    HINSTANCE hInstance = nullptr;

    static LRESULT CALLBACK WndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    );

public:
    bool Create(HWND targetWindow);

    bool Update();

    void DrawBox(
        int x1,
        int y1,
        int x2,
        int y2
    );

    HWND GetHandle() const;
};


/*
// การใช้ overlay
    // Overlay overlay;
    // if (!overlay.Create(game))
    // {
    //     std::cout << "Failed to create overlay\n";
    //     return 1;
    // }
    // while (overlay.Update()){
    //     overlay.DrawBox(
    //         100,
    //         100,
    //         300,
    //         250
    //     );

    //     Sleep(10);
    // }
*/