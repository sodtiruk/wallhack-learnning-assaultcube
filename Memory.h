// Memory.h
#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <string>
#include <initializer_list>

class Memory {
public:
    Memory() = default;
    ~Memory() { Detach(); }
    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;

    // เชื่อมต่อกับเกมจากชื่อหน้าต่าง
    bool Attach(const char* windowTitle) {
        hwnd = FindWindowA(nullptr, windowTitle);
        if (!hwnd) return false;

        GetWindowThreadProcessId(hwnd, &pid);
        handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        return handle != nullptr;
    }

    void Detach() {
        if (handle) { CloseHandle(handle); handle = nullptr; }
    }

    HWND GetHwnd() const { return hwnd; }
    DWORD GetPid() const { return pid; }

    // หา base address ของโมดูล เช่น L"ac_client.exe"
    uintptr_t GetModuleBase(const wchar_t* moduleName) const {
        uintptr_t base = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap == INVALID_HANDLE_VALUE) return 0;

        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);
        if (Module32FirstW(snap, &me)) {
            do {
                if (_wcsicmp(me.szModule, moduleName) == 0) {
                    base = (uintptr_t)me.modBaseAddr;
                    break;
                }
            } while (Module32NextW(snap, &me));
        }
        CloseHandle(snap);
        return base;
    }

    // อ่านค่าชนิดอะไรก็ได้ ถ้าอ่านไม่ได้จะคืนค่า 0
    template <typename T>
    T Read(uintptr_t address) const {
        T value{};
        ReadProcessMemory(handle, (LPCVOID)address, &value, sizeof(T), nullptr);
        return value;
    }

    // เหมือน Read แต่บอกได้ว่าสำเร็จหรือไม่
    template <typename T>
    bool TryRead(uintptr_t address, T& out) const {
        return ReadProcessMemory(handle, (LPCVOID)address, &out, sizeof(T), nullptr) != 0;
    }

    // อ่านพอยน์เตอร์ของเกม (เกมเป็น 32-bit จึงอ่านแค่ 4 ไบต์)
    uintptr_t ReadPtr(uintptr_t address) const {
        return Read<uint32_t>(address);
    }

    // ตามพอยน์เตอร์เป็นทอด ๆ แล้วอ่านค่าสุดท้าย
    // offset ตัวสุดท้ายคือ offset ของค่าที่ต้องการอ่าน
    template <typename T>
    T ReadChain(uintptr_t base, std::initializer_list<uintptr_t> offsets) const {
        uintptr_t addr = base;
        size_t i = 0;
        for (uintptr_t off : offsets) {
            if (++i == offsets.size())
                return Read<T>(addr + off);
            addr = ReadPtr(addr + off);
            if (!addr) return T{};
        }
        return Read<T>(addr);
    }

    // อ่านสตริง เช่น ชื่อผู้เล่น
    std::string ReadString(uintptr_t address, size_t maxLen = 32) const {
        std::string buf(maxLen, '\0');
        ReadProcessMemory(handle, (LPCVOID)address, buf.data(), maxLen, nullptr);
        buf.resize(strnlen(buf.c_str(), maxLen));
        return buf;
    }

private:
    HWND hwnd = nullptr;
    DWORD pid = 0;
    HANDLE handle = nullptr;
};

/*

    Memory mem;
    if (!mem.Attach("AssaultCube")) {
        std::cout << "Game not found\n";
        return 1;
    }

    uintptr_t base = mem.GetModuleBase(L"ac_client.exe");
    uintptr_t localPlayerAddr = base + 0x0017E0A8;

    // แบบสั้น: ตามพอยน์เตอร์ localPlayer แล้วอ่าน health ที่ +0xEC
    int health = mem.ReadChain<int>(localPlayerAddr, {0x0, 0xEC});
    std::cout << "Health: " << health << "\n";

    // แบบแยกขั้นตอน เหมาะเมื่อจะอ่านหลายค่าจาก player เดียวกัน
    uintptr_t player = mem.ReadPtr(localPlayerAddr);
    int armor = mem.Read<int>(player + 0xF0);



*/