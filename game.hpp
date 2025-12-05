namespace menu {
    // 变量声明
    extern bool isOpen;
    extern bool bGodMode;
    extern bool bUStamina;
    extern bool bInfiniteAmmo;
    extern bool bInfiniteMoney;
    extern bool bNoRecoil;
    extern bool bShootRange;
    extern bool bSpeedHack;
    
    extern bool bRequestAddMoney;
    extern float fMoneyToAdd;
    extern float fMoveSpeedMultiplier;

    // 【关键修复】函数声明放在这里
    // 此时编译器已经读取了 SDK.hpp，它知道 APlayer_char_main_C 是个类
    SDK::APlayer_char_main_C* GetLocalPlayerChar();
}