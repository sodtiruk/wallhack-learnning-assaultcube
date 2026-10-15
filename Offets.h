// offset.h
#pragma once
#include <cstdint>

// AssaultCube 1.3.0.2 (32-bit)
namespace Offsets {

    // offset ที่บวกกับ base ของ ac_client.exe
    namespace Module {
        constexpr uintptr_t LocalPlayer = 0x0017E0A8;
        constexpr uintptr_t EntityList  = 0x0018AC04;
        constexpr uintptr_t ViewMatrix  = 0x0017DFD0;
    }

    // offset ภายใน struct ของผู้เล่น (บวกกับ address ของ player)
    namespace PlayerOffets {
        constexpr uintptr_t HealthOffets = 0xEC;
        
    }

    namespace EntityOffets {

    }
}