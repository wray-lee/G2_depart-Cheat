// 定义原始函数类型 (Location 和 Rotation 是输出参数)
typedef void(__fastcall* GetPlayerViewPoint_t)(APlayerController* thisptr, FVector& Location, FRotator& Rotation);
GetPlayerViewPoint_t oGetPlayerViewPoint = nullptr;

// 全局变量，用于存储自瞄锁定的目标
AActor* g_SilentAimTarget = nullptr;

void __fastcall hkGetPlayerViewPoint(APlayerController* thisptr, FVector& Location, FRotator& Rotation) {
    // 1. 先调用原始函数，获取真实的相机位置和角度
    oGetPlayerViewPoint(thisptr, Location, Rotation);

    // 2. 如果开启了魔法子弹，并且有目标
    if (menu::bMagicBullet && g_SilentAimTarget) {
        // 获取目标位置
        FVector TargetPos = g_SilentAimTarget->K2_GetActorLocation();
        TargetPos.Z += 60.0f; // 瞄头

        // 3. 【欺骗引擎】：计算从相机到敌人的角度，并覆盖 Rotation
        // 这样你的画面没动，但游戏引擎认为你正看着敌人
        Rotation = CalculateAngle(Location, TargetPos);
    }
}

// 假设这是 GetControlRotation 的 Hook
FRotator __fastcall hkGetControlRotation(APlayerController* thisptr) {
    if (menu::bMagicBullet && g_SilentAimTarget) {
        // 返回欺骗后的角度
        FVector MyLoc = thisptr->PlayerCameraManager->GetCameraLocation();
        FVector TargetLoc = g_SilentAimTarget->K2_GetActorLocation();
        return CalculateAngle(MyLoc, TargetLoc);
    }
    return oGetControlRotation(thisptr);
}