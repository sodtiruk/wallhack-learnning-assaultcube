

// ESP แบบง่ายสุดสำหรับ AssaultCube (external: อ่าน memory อย่างเดียว ไม่แก้ค่าในเกม)
// - ต้อง build เป็น x86 (32-bit) เพราะ ac_client.exe เป็น 32-bit
// - ใช้ศึกษาบนบอทในเครื่อง (offline) เท่านั้น
// - เกมต้องเปิดแบบ Windowed / Borderless (fullscreen แท้ overlay จะไม่ขึ้น)
// - กด END เพื่อออกจากโปรแกรม
#include <windows.h>
#include <cstdint>
#include <iostream>
#include "Memory.h"   // <-- ใส่ header ของคลาส Memory ของคุณ (และ header ที่มี LocalPlayer/EntityList/ViewMatrix)

using namespace std;

// ตั้งชื่อไม่ซ้ำกับ Vec2/Vec3 เดิมในโปรเจกต์ จะได้ไม่ชน
struct V2 { float x, y; };
struct V3 { float x, y, z; };

// offset ของ entity (AssaultCube 1.3.0.2) ถ้าเกมคนละเวอร์ชันให้หาใหม่ด้วย Cheat Engine
constexpr uintptr_t OFF_PLAYER_COUNT = 0x18AC0C; // offset จาก base module
constexpr uintptr_t OFF_HEAD   = 0x4;    // V3 ตำแหน่งหัว
constexpr uintptr_t OFF_FEET   = 0x28;   // V3 ตำแหน่งเท้า
constexpr uintptr_t OFF_HEALTH = 0xEC;   // int
constexpr uintptr_t OFF_TEAM   = 0x30C;  // int

// แปลงพิกัดโลก 3D -> พิกัดหน้าจอ 2D (matrix เป็น OpenGL column-major 16 floats)
bool WorldToScreen(const V3& pos, V2& out, const float m[16], float w, float h)
{
    float clipX = pos.x * m[0] + pos.y * m[4] + pos.z * m[8]  + m[12];
    float clipY = pos.x * m[1] + pos.y * m[5] + pos.z * m[9]  + m[13];
    float clipW = pos.x * m[3] + pos.y * m[7] + pos.z * m[11] + m[15];

    if (clipW < 0.1f) return false;           // อยู่หลังกล้อง

    float ndcX = clipX / clipW;
    float ndcY = clipY / clipW;

    out.x = (w / 2.0f) * (1.0f + ndcX);
    out.y = (h / 2.0f) * (1.0f - ndcY);

    return out.x >= 0 && out.x <= w && out.y >= 0 && out.y <= h;
}

int main()
{
    Memory mem;
    if (!mem.Attach("AssaultCube")) {
        cout << "Game not found\n";
        return 1;
    }

    uintptr_t base            = mem.GetModuleBase(L"ac_client.exe");
    uintptr_t localPlayerAddr = base + 0x0017E0A8;
    uintptr_t entityList      = base + 0x0018AC04;
    uintptr_t viewMatrix      = base + 0x0017DFD0;
    uintptr_t playerCountAddr = base + OFF_PLAYER_COUNT;

    HWND game = FindWindowA(nullptr, "AssaultCube");
    if (!game) {
        cout << "Game window not found\n";
        return 1;
    }

    // ---------- สร้างหน้าต่าง overlay โปร่งใส ทับบนเกม ----------
    HINSTANCE hInst = GetModuleHandleA(nullptr);

    WNDCLASSA wc = {};
    wc.lpfnWndProc   = DefWindowProcA;
    wc.hInstance     = hInst;
    wc.lpszClassName = "EspOverlay";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);   // สีดำ = สีที่จะถูกทำให้โปร่งใส
    RegisterClassA(&wc);

    HWND overlay = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "EspOverlay", "", WS_POPUP, 0, 0, 100, 100,
        nullptr, nullptr, hInst, nullptr);

    SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), 0, LWA_COLORKEY); // สีดำโปร่งใส
    ShowWindow(overlay, SW_SHOWNOACTIVATE);

    // ---------- เตรียม brush (สร้างครั้งเดียว ใช้ซ้ำทุกเฟรม) ----------
    HBRUSH redBrush   = CreateSolidBrush(RGB(255, 0, 0));
    HBRUSH greenBrush = CreateSolidBrush(RGB(0, 255, 0));
    HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);  // stock object ไม่ต้อง Delete

    // ---------- double buffer กันภาพกะพริบ (ขนาดตามหน้าต่างเกมตอนเริ่ม) ----------
    RECT rc;
    GetClientRect(game, &rc);
    int W = rc.right;
    int H = rc.bottom;

    HDC     winDC = GetDC(overlay);
    HDC     memDC = CreateCompatibleDC(winDC);
    HBITMAP bmp   = CreateCompatibleBitmap(winDC, W, H);
    SelectObject(memDC, bmp);

    // ---------- ลูปหลัก ----------
    while (!(GetAsyncKeyState(VK_END) & 0x8000) && IsWindow(game))
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // ให้ overlay ตามตำแหน่งหน้าต่างเกมตลอด
        POINT origin = { 0, 0 };
        ClientToScreen(game, &origin);
        SetWindowPos(overlay, HWND_TOPMOST, origin.x, origin.y, W, H, SWP_NOACTIVATE);

        // ล้างจอ (ดำ = โปร่งใส)
        RECT full = { 0, 0, W, H };
        FillRect(memDC, &full, blackBrush);

        // อ่าน view matrix 16 ค่า
        float vm[16];	
        for (int i = 0; i < 16; i++)
            vm[i] = mem.Read<float>(viewMatrix + i * 4);

        uintptr_t local  = mem.ReadPtr(localPlayerAddr);
        int       myTeam = mem.Read<int>(local + OFF_TEAM);

        uintptr_t list  = mem.ReadPtr(entityList);              // array ของ pointer (4 byte ต่อช่อง)
        int       count = mem.Read<int>(playerCountAddr);

        for (int i = 1; i < count; i++)                          // index 0 ว่าง เริ่มที่ 1
        {
            // เกมนี้ object entity จะบวกด้วย 4 แต่บางเกมอาจจะ +10 หรือ +4 ต้องสังเกตตอนหาค่าเอาเอง
            uintptr_t ent = mem.ReadPtr(list + i * 4);
            if (!ent) continue;

            int health = mem.Read<int>(ent + OFF_HEALTH);
            if (health <= 0 || health > 100) continue;           // ตายแล้ว / ค่าแปลก

            // เนื่องจาก เกมนี้เราสามารถหาค่าได้ทั้ง x y z position ของ head และ feet ทำให้เราไม่ต้องคำนวน ค่า head หรือ feet อันใดอันนึงเอง
            // สมมตืว่าเรารู้ค่า feet เราก็สามารถคำนวนหาค่า head ได้
            // feet เราทำเป็น vec3 ข้างในเป็น x y z มันจะ 4byte พอดี แล้วตัว mem.Read มันจะคิด offset + 4 ตามลำดับของ x y z ให้
            V3 head = mem.Read<V3>(ent + OFF_HEAD);
            V3 feet = mem.Read<V3>(ent + OFF_FEET);
            head.z += 0.75f;                                     // เผื่อเหนือหัวนิดหน่อย

            V2 head2, feet2;
            if (!WorldToScreen(head, head2, vm, (float)W, (float)H)) continue;
            if (!WorldToScreen(feet, feet2, vm, (float)W, (float)H)) continue;

            // คำนวนความสูงจาก ค่า y ที่ได้ตำแหน่งบนจอ มาจาก worldtoscreen เอามาลบกันจะได้ ความสูงของตัวละคร หรือกรอบแนวตั้ง
            float height = feet2.y - head2.y;
            if (height <= 0) continue;

            // ความกว้างของขอบ ประมาณเอา คิดว่า ความสูงเป็น 2 เท่าของความกว้าง เลยจับหาร หรือถ้ากรอบ ใหญ่ไป สามารถปรับเป็นหาร 1.75f หรือ 1.5 ได้
            float width = height / 2.0f;

            RECT box = {
                (LONG)(head2.x - width / 2), (LONG)head2.y,
                (LONG)(head2.x + width / 2), (LONG)feet2.y
            };


            // เป็นการคิดกรอบสี ถ้าฝั่งเดียวกันก็จะเป็นสีเขียว แต่ถ้าคนละฝั่งก็จะกรอบสีแดง
            int team = mem.Read<int>(ent + OFF_TEAM);
            FrameRect(memDC, &box, team == myTeam ? greenBrush : redBrush);
        }

        BitBlt(winDC, 0, 0, W, H, memDC, 0, 0, SRCCOPY);         // ส่งภาพที่วาดเสร็จขึ้นจอทีเดียว
	        Sleep(5);
    }

    // ---------- ล้างทรัพยากร ----------
    DeleteObject(bmp);
    DeleteDC(memDC);
    ReleaseDC(overlay, winDC);
    DeleteObject(redBrush);
    DeleteObject(greenBrush);
    DestroyWindow(overlay);
    return 0;
}