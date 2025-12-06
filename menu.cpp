#include "stdafx.h"

using namespace SDK;

// --- [Noclip 数学辅助函数] ---
#define M_PI 3.14159265358979323846f
#define DEG2RAD(x) ((x) * (M_PI / 180.0f))

namespace Math
{
    float GetDistance2D(FVector2D A, FVector2D B)
    {
        return sqrt(pow(A.X - B.X, 2) + pow(A.Y - B.Y, 2));
    }

    FRotator VectorToRotator(FVector Start, FVector Target)
    {
        FVector Delta = Target - Start;
        float Hyp = sqrt(Delta.X * Delta.X + Delta.Y * Delta.Y);
        FRotator Rot;
        Rot.Yaw = atan2(Delta.Y, Delta.X) * (180.0f / M_PI);
        // ue中pitch相反
        Rot.Pitch = -atan2(Delta.Z, Hyp) * (180.0f / M_PI);
        Rot.Roll = 0.0f;
        return Rot;
    }

    // 将角度标准化到 -180 ~ 180
    // 有引擎原生 FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(Target, Current);
    // Result.Normalize();
    FRotator ClampAngles(FRotator Rot)
    {
        // 1. 先处理 Yaw 的归一化
        while (Rot.Yaw > 180.0f)
            Rot.Yaw -= 360.0f;
        while (Rot.Yaw < -180.0f)
            Rot.Yaw += 360.0f;

        // 2. 先处理 Pitch 的归一化，再进行 Clamp
        // 如果不加这个，当 Delta 超过 180 度时（比如 360），会被错误地截断成 89
        while (Rot.Pitch > 180.0f)
            Rot.Pitch -= 360.0f;
        while (Rot.Pitch < -180.0f)
            Rot.Pitch += 360.0f;

        // 3. 最后再限制俯仰角在合法范围内
        if (Rot.Pitch > 89.0f)
            Rot.Pitch = 89.0f;
        if (Rot.Pitch < -89.0f)
            Rot.Pitch = -89.0f;

        Rot.Roll = 0.0f;
        return Rot;
    }

    // 平滑插值
    FRotator SmoothAngle(FRotator Current, FRotator Target, float Smooth)
    {
        FRotator Delta = Target - Current;
        Delta = ClampAngles(Delta);
        return ClampAngles(Current + Delta * Smooth);
    }
    // noclip

    FVector GetCameraForward(FRotator Rot)
    {
        float sp = sin(DEG2RAD(Rot.Pitch));
        float cp = cos(DEG2RAD(Rot.Pitch));
        float sy = sin(DEG2RAD(Rot.Yaw));
        float cy = cos(DEG2RAD(Rot.Yaw));
        return {cp * cy, cp * sy, sp};
    }

    FVector GetCameraRight(FRotator Rot)
    {
        float sy = sin(DEG2RAD(Rot.Yaw));
        float cy = cos(DEG2RAD(Rot.Yaw));
        return {-sy, cy, 0.0f};
    }
}

// ----------------------------

namespace menu
{
    bool isOpen = false;
    static bool noTitleBar = false;

    // 功能开关
    bool bGodMode = false;
    bool bUStamina = false;
    bool bInfiniteAmmo = false;
    bool bInfiniteMoney = false;
    bool bNoRecoil = false;

    bool bAttackSpeed = false;
    float default_shootint_delay;
    float default_shooting_delay_current = -1.0f;
    float default_CW_Fire_Delay = -1.0f;
    float default_CW_Burst_Delay = -1.0f;




    bool bShootRange = false;

    bool bSpeedHack = false;
    float fOrg_SprintSpeed = -1.0f;
    float fOrg_WalkSpeed = -1.0f;
    float fMoveSpeedMultiplier = 1.1f;

    bool bHighJump = false;
    float fOrg_JumpZVelocity = -0.1f;
    float fJumpMultiplier = 2.0f;
    bool bMultipleJumpTimes = false;
    int iJumpTimes = 2147483647;


    bool bNoclip = false;
    bool bInstantSkill = false;
    bool bExpRate = false;

    float fExpRateMultiplier = 2.0f;
    



    // ESP
    bool bEspEnable = false;                         // ESP 总开关
    bool bEspChildEnable[3] = {false, false, false}; // 子开关
    bool bEspMonster = false;                        // 显示怪物
    bool bEspPlayer = false;                         // 显示其他玩家
    bool bEspLT = false;                             // 显示掉落
    bool bEspFumo = false;                             // 显示fumo

    enum class EActorType
    {
        Unknown = 0,
        Player,
        Enemy,
        Chest,
        Fumo
    };
    float col_Player[4] = {0.2f, 1.0f, 0.2f, 1.0f};  // 默认绿色
    float col_Monster[4] = {1.0f, 0.2f, 0.2f, 1.0f}; // 默认红色
    float col_Loot[4] = {1.0f, 1.0f, 0.0f, 1.0f};    // 默认黄色
	float col_Fumo[4] = { 0.0f, 0.0f, 1.0f, 1.0f };	// 默认蓝色

    // Aimbot
    bool bAimbot = false;
    bool bMagicBullet = false;
    float fAimbotFOV = 150.0f;  // 自瞄范围
    float fAimbotSmooth = 1.0f; // 平滑度 (1.0 = 锁死)
    bool bDrawFOV = true;       // 绘制 FOV 圈


    // Debug
    bool bDebugScanner = false;
    float fDebugScanDist = 500.0f; // 默认扫描 5米 (UE单位通常是厘米，500 = 5m)
    bool bShowClassOnly = true;    // 只显示类名(看起来简洁点)

    bool SafeProjectWorldToScreen(APlayerController* PC, FVector WorldLoc, FVector2D* OutScreenPos);
    void DrawText3D(APlayerController* PC, const FVector& WorldPos, const char* Text, float* ColorFloat);
    APlayer_char_main_C* GetLocalPlayerChar();
    APlayerController* GetPlayerController();
    EActorType GetActorType(AActor* Actor);


    // 安全检查函数
    bool SafeProjectWorldToScreen(APlayerController* PC, FVector WorldLoc, FVector2D* OutScreenPos)
    {
        bool bResult = false;
        __try
        {
            // 这里进行最危险的内存访问
            if (PC && PC->Player)
            {
                bResult = PC->ProjectWorldLocationToScreen(WorldLoc, OutScreenPos, true);
            }
        }   
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // 如果崩溃了，吞掉异常，返回 false
            bResult = false;
        }
        return bResult;
    }
    // 绘制函数
    void DrawText3D(APlayerController* PC, const FVector& WorldPos, const char* Text, float* ColorFloat)
    {
        FVector2D ScreenPos;
        if (PC->ProjectWorldLocationToScreen(WorldPos, &ScreenPos, true))
        {
            // 简单防出界
            auto io = ImGui::GetIO();
            if (ScreenPos.X > 0 && ScreenPos.Y > 0 && ScreenPos.X < io.DisplaySize.x && ScreenPos.Y < io.DisplaySize.y)
            {
                ImColor Color = ImColor(ColorFloat[0], ColorFloat[1], ColorFloat[2], ColorFloat[3]);
                ImVec2 TextSize = ImGui::CalcTextSize(Text);
                // 文字居中
                ImGui::GetBackgroundDrawList()->AddText(
                    ImVec2(ScreenPos.X - (TextSize.x / 2), ScreenPos.Y),
                    Color,
                    Text);
            }
        }
    }
    // 实体类型判断
    EActorType GetActorType(AActor* Actor)
    {
        if (!Actor)
            return EActorType::Unknown;

        // 判断是否为玩家
        if (Actor->IsA(APlayer_char_main_C::StaticClass()))
        {
            return EActorType::Player;
        }

        // 判断是否为怪物 AI
        if (Actor->IsA(AMG_AI_actor_master_C::StaticClass()))
        {
            return EActorType::Enemy;
        }

        // 判断是否为掉落物
        if (Actor->IsA(ALT_loot_box_main_C::StaticClass()))
            return EActorType::Chest;
        
        // 判断是否为fumo
        if (Actor->IsA(AQS_quest_check_actor_04_FUMO_C::StaticClass()))
            return EActorType::Fumo;

        return EActorType::Unknown;
    }
	// 获取本地玩家控制器和角色
    APlayerController* GetPlayerController()
    {
        UWorld* World = UWorld::GetWorld();
        if (!World || !World->OwningGameInstance || World->OwningGameInstance->LocalPlayers.Num() <= 0)
            return nullptr;
        ULocalPlayer* LocalPlayer = World->OwningGameInstance->LocalPlayers[0];
        if (!LocalPlayer || !LocalPlayer->PlayerController)
            return nullptr;

        return static_cast<APlayerController*>(LocalPlayer->PlayerController);
    }
    APlayer_char_main_C* GetLocalPlayerChar()
    {
        APlayerController* MyController = GetPlayerController();

        if (MyController != nullptr)
        {
            return static_cast<APlayer_char_main_C*>(MyController->Pawn);
        }

        return nullptr;
    }

    void RunDebugScanner(APlayerController* PC, AActor* MyChar)
    {
        if (!bDebugScanner) return;

        UWorld* World = UWorld::GetWorld();
        if (!World || !World->PersistentLevel) return;

        // 获取当前关卡所有 Actor
        TArray<AActor*> Actors = World->PersistentLevel->Actors;

        // 获取屏幕中心，防止后续计算用到
        auto io = ImGui::GetIO();
        FVector2D ScreenCenter = { io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f };

        for (int i = 0; i < Actors.Num(); i++)
        {
            AActor* Actor = Actors[i];

            // 1. 基础过滤
            if (!Actor || Actor == MyChar) continue;

            // 2. 距离过滤 (优化性能的关键)
            // 简单的距离计算，避免开根号带来的性能损耗，用距离平方判断也行，这里为了直观用 GetDistanceTo
            float Dist = MyChar->GetDistanceTo(Actor);
            if (Dist > fDebugScanDist) continue;

            // 3. 获取位置
            FVector Pos = Actor->K2_GetActorLocation();
            FVector2D ScreenPos;

            // 4. 安全的坐标转换 (加上之前的 SEH 保护)

            if (SafeProjectWorldToScreen(PC, Pos, &ScreenPos))
            {
                // 5. 获取名字 (核心)
                std::string ClassName = "NullClass";
                std::string ObjName = "NullObj";

                if (Actor->Class) {
                    // 获取类名，例如 "BP_Goblin_Warrior_C"
                    // 你的 SDK 可能叫 GetName() 或者 GetFullName()
                    // 如果是 Dumper-7 生成的 SDK，通常用 Actor->Class->GetName()
                    ClassName = Actor->Class->GetName();
                }

                // 获取对象名，例如 "BP_Goblin_Warrior_C_24"
                ObjName = Actor->GetName();

                // 6. 过滤掉不需要的垃圾 (可选)
                // 场景里会有很多莫名其妙的东西，比如 "Brush", "Info", "Volume"，可以过滤掉让视野干净点
                if (ClassName.find("Brush") != std::string::npos ||
                    ClassName.find("Volume") != std::string::npos ||
                    ClassName.find("Info") != std::string::npos)
                    continue;

                // 7. 拼接显示文本
                char Buf[256];
                if (bShowClassOnly) {
                    sprintf_s(Buf, "[%s]\ndist: %.1fm", ClassName.c_str(), Dist / 100.0f);
                }
                else {
                    sprintf_s(Buf, "Class: %s\nObj: %s\ndist: %.1fm", ClassName.c_str(), ObjName.c_str(), Dist / 100.0f);
                }

                // 8. 绘制 (用青色，比较显眼)
                float col_Debug[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
                DrawText3D(PC, Pos, Buf, col_Debug);

                // 在物体位置画个小圈
                ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(ScreenPos.X, ScreenPos.Y), 3.0f, ImColor(0, 255, 255));
            }
        }
    }

    // 获取敌人头部位置的函数
    FVector GetEnemyHeadPos(AActor *Actor)
    {
        if (!Actor)
            return {0, 0, 0};

        auto Monster = static_cast<AMG_AI_actor_master_C *>(Actor);

        if (Monster->is_death || Monster->HP_current <= 0.0f) {
            // 怪物死了就返回脚底板坐标或者直接返回0，防止崩溃
            return Monster->K2_GetActorLocation();
        }

        // 使用 bone_head (需要 ACharacter 的 Mesh 有效)
        // Monster->bone_head 存储了头部骨骼的名字
        if ((!Monster->is_death || Monster->HP_current <= 0.0f) && Monster->Mesh && Monster->bone_head.ComparisonIndex!=0) {
            return Monster->Mesh->GetSocketLocation(Monster->bone_head);
        }

        // 动态胶囊体高度
        //  不要用写死的 130.0f，改用胶囊体的一半高度 * 缩放
        //ACharacter 都有 CapsuleComponent
         if ((!Monster->is_death || Monster->HP_current <= 0.0f) && Monster->CapsuleComponent) {
             float CapsuleHalfHeight = Monster->CapsuleComponent->GetScaledCapsuleHalfHeight();
             FVector Pos = Monster->K2_GetActorLocation();

            // 胶囊体中心在腰部，所以 Z + HalfHeight 大概就是头顶
            // 稍微减一点(比如 * 0.8) 也就是瞄眉毛的位置，防止射飞
            Pos.Z += (CapsuleHalfHeight * 0.8f);
            return Pos;
         }


        // 使用 see_Scene 组件
        // SDK 显示偏移 0x04E0 有一个 see_Scene，通常用于AI视野检测，位于头部/眼睛
        if ((!Monster->is_death || Monster->HP_current <= 0.0f) && Monster->see_Scene)
        {
            return Monster->see_Scene->K2_GetComponentLocation();
        }





        // 简单的坐标偏移 (保底)
        // FVector Pos = Monster->K2_GetActorLocation();
        // Pos.Z += 130.0f; // 假设怪物头部高度
        // return Pos;
        return Monster->K2_GetActorLocation();
    }
    FVector GetEnemyWaistPos(AActor* Actor) {
        if (!Actor)
            return { 0, 0, 0 };
        auto Monster = static_cast<AMG_AI_actor_master_C*>(Actor);
        if (Monster->CapsuleComponent) {
            float CapsuleHalfHeight = Monster->CapsuleComponent->GetScaledCapsuleHalfHeight();
            FVector Pos = Monster->K2_GetActorLocation();

            return Pos;
        }

        return Monster->K2_GetActorLocation();
    
    }


    // 自瞄和魔法子弹
    void RunAimbot(APlayerController* PC, APlayer_char_main_C* MyChar)
    {
        AActor* CurrentTarget = nullptr;
        UWorld* World = UWorld::GetWorld();
        if (!World || !World->PersistentLevel)
            return;

        FVector2D ScreenCenter = { ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f };
        TArray<AActor*> Actors = World->PersistentLevel->Actors;

        AActor* BestTarget = nullptr;
        float BestDist = fAimbotFOV; // 初始距离设为FOV范围

        // --- 1. 寻找最佳目标 ---
        if (bAimbot || bMagicBullet)
        {
            for (AActor* Actor : Actors)
            {
                if (!Actor || Actor == MyChar)
                    continue;

                // 筛选: 必须是怪物类型
                if (GetActorType(Actor) != EActorType::Enemy)
                    continue;

                auto Monster = static_cast<AMG_AI_actor_master_C*>(Actor);
                if (Monster->is_death || Monster->HP_current <= 0)
                    continue;

                // 获取头部位置
                FVector HeadPos = GetEnemyHeadPos(Actor);
                FVector2D ScreenPos;
                // 空指针安全检查
                if (!PC) return;
                if (!PC->Player) return;
                if (!PC->PlayerCameraManager) return;

                // 转换并判断是否在 FOV 内

                if (PC->ProjectWorldLocationToScreen(HeadPos, &ScreenPos, true))
                {
                    float Dist = Math::GetDistance2D(ScreenCenter, ScreenPos);
                    if (Dist < BestDist)
                    {
                        BestDist = Dist;
                        BestTarget = Actor;
                    }
                }
            }
            CurrentTarget = BestTarget; // 更新全局目标
        }

        // 如果没有目标，后续都不执行
        if (!CurrentTarget)
            return;

        FVector TargetLoc = GetEnemyHeadPos(CurrentTarget);
        FVector TargetWaistLoc = GetEnemyWaistPos(CurrentTarget);
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!---------------------debug---------------!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (bAimbot || bMagicBullet)
        {
            FVector2D ScreenPos;
            // 安全检查，避免绘制空指针
            if (!PC) return;
            if (!PC->Player) return;
            if (!PC->PlayerCameraManager) return;

            if (SafeProjectWorldToScreen(PC, TargetLoc, &ScreenPos))
            {
                auto DrawList = ImGui::GetBackgroundDrawList();

                // 1. 画一条线连过去
                FVector2D ScreenCenter = { ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f };
                DrawList->AddLine(ImVec2(ScreenCenter.X, ScreenCenter.Y), ImVec2(ScreenPos.X, ScreenPos.Y), ImColor(255, 0, 0), 1.0f);

                // 2. 在目标位置画一个红点
                DrawList->AddCircleFilled(ImVec2(ScreenPos.X, ScreenPos.Y), 5.0f, ImColor(255, 0, 0, 255));

                // 3. (可选) 打印出 Z 轴偏移量，看看是不是太高了
                //FVector RootPos = CurrentTarget->K2_GetActorLocation();
                //float HeightDiff = TargetLoc.Z - RootPos.Z;
                //char HBuf[32];
                //sprintf_s(HBuf, "H: %.1f", HeightDiff);
                //DrawList->AddText(ImVec2(ScreenPos.X + 10, ScreenPos.Y), ImColor(255, 255, 0), HBuf);
            }
            //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!---------------------debug---------------!!!!!!!!!!!!!!!!!!!!!!!!!!!
        }
        // --- 2. 自瞄逻辑 (按住鼠标右键) ---
        if (bAimbot && (GetAsyncKeyState(VK_RBUTTON) & 0x8000))
        {
            FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();

            FRotator CurrentRot = PC->GetControlRotation();

            // 计算目标角度
            //FRotator TargetRot = Math::VectorToRotator(CamLoc, TargetLoc);
            // 引擎原生函数计算目标角度
            FRotator TargetRot = UKismetMathLibrary::FindLookAtRotation(CamLoc, TargetLoc);

            // 平滑移动
            //FRotator FinalRot = Math::SmoothAngle(CurrentRot, TargetRot, fAimbotSmooth);
            // 引擎原生函数平滑
            FRotator FinalRot = UKismetMathLibrary::RLerp(CurrentRot, TargetRot, fAimbotSmooth, true);
            // 方案 B: 使用 RInterpTo (更像真人的平滑移动，需要 DeltaTime)
            // float DeltaTime = UWorld::GetWorld()->GetDeltaSeconds();
            // float InterpSpeed = fAimbotSmooth * 100.0f; // 转换一下速度感
            // FinalRot = UKismetMathLibrary::RInterpTo(CurrentRot, TargetRot, DeltaTime, InterpSpeed);

            //PC->SetControlRotation(FinalRot);
            // 修复偶尔的崩溃，越过SetControlRotation检查
            if (FinalRot.Pitch != FinalRot.Pitch || FinalRot.Yaw != FinalRot.Yaw || FinalRot.Roll != FinalRot.Roll)
                return;

            PC->ControlRotation = FinalRot;
        }

        // --- 3. 魔法子弹逻辑 ---
        //if (bMagicBullet && CurrentTarget && MyChar && PC)
        //{
        //    UWorld* World = UWorld::GetWorld();
        //    if (!World || !World->PersistentLevel)
        //        return;

        //    TArray<AActor*> Actors = World->PersistentLevel->Actors;

        //    FVector TargetPos = GetEnemyWaistPos(CurrentTarget);
        //    if (TargetPos.X == 0 && TargetPos.Y == 0 && TargetPos.Z == 0)
        //        return;

        //    for (AActor* Actor : Actors)
        //    {
        //        if (!Actor)
        //            continue;

        //        if (!Actor->IsA(ABullet_Projectile_C::StaticClass()))
        //            continue;

        //        auto Bullet = static_cast<ABullet_Projectile_C*>(Actor);
        //        if (!Bullet)
        //            continue;

        //        // 只控制自己发射的子弹
        //        APawn* InstigatorPawn = static_cast<APawn*>(Bullet->GetInstigator());
        //        if (InstigatorPawn != MyChar)
        //            continue;

        //        // 距离检查：太远的不引导，太近的让引擎自己判定命中
        //        float DistFromMe = MyChar->GetDistanceTo(Bullet);
        //        float DistToTarget = CurrentTarget->GetDistanceTo(Bullet);

        //        if (DistFromMe > 4000.0f)      // 超过 40m 不管
        //            continue;
        //        if (DistToTarget < 50.0f)      // 已经贴身 0.5m 不拉扯
        //            continue;

        //        FVector BulletLoc = Bullet->K2_GetActorLocation();

        //        // 重新计算方向 + 位置
        //        FVector ToTarget = TargetPos - BulletLoc;
        //        float   Dist = ToTarget.Magnitude();
        //        if (Dist < 100.0f)             // 太近就算了
        //            continue;

        //        FVector Dir = ToTarget / Dist;

        //        // 传送到目标前方 50cm
        //        FVector TeleportPos = TargetPos - Dir * 50.0f;
        //        Bullet->K2_SetActorLocation(TeleportPos, true, nullptr, true);

        //        // 再次基于 TeleportPos 重算方向，保证完全一致
        //        FVector FinalToTarget = TargetPos - TeleportPos;
        //        float   FinalDist = FinalToTarget.Magnitude();
        //        if (FinalDist < 10.0f)
        //            continue;

        //        FVector FinalDir = FinalToTarget / FinalDist;

        //        // 旋转校正
        //        FRotator LookRot = UKismetMathLibrary::FindLookAtRotation(TeleportPos, TargetPos);
        //        Bullet->K2_SetActorRotation(LookRot, false);

        //        // 速度校正
        //        if (Bullet->Projectile)
        //        {
        //            float Speed = Bullet->Projectile->Velocity.Magnitude();
        //            if (Speed < 1000.0f)
        //                Speed = 3000.0f;    // 一个你自己游戏里正常的飞行速度

        //            Bullet->Projectile->Velocity = FinalDir * Speed;

        //            if (Bullet->Projectile->UpdatedComponent)
        //                Bullet->Projectile->UpdatedComponent->ComponentVelocity = Bullet->Projectile->Velocity;
        //        }
        //        auto Draw = ImGui::GetBackgroundDrawList();
        //        FVector2D S1, S2;
        //        if (PC->ProjectWorldLocationToScreen(TeleportPos, &S1, true) &&
        //            PC->ProjectWorldLocationToScreen(TargetPos, &S2, true))
        //        {
        //            Draw->AddCircleFilled(ImVec2(S1.X, S1.Y), 4, ImColor(0, 255, 0));
        //            Draw->AddCircleFilled(ImVec2(S2.X, S2.Y), 4, ImColor(255, 0, 0));
        //            Draw->AddLine(ImVec2(S1.X, S1.Y), ImVec2(S2.X, S2.Y), ImColor(255, 255, 0), 1.5f);
        //        }
        //    }
        //}
        
        // 魔法子弹无效，直接造成伤害
        //if (bMagicBullet && CurrentTarget && (GetAsyncKeyState(VK_LBUTTON) & 0x8000))
        //{
        //    // 转换指针
        //    auto Monster = static_cast<AMG_AI_actor_master_C*>(CurrentTarget);

        //    // 简单的冷却时间检查，防止一帧调用60次导致崩溃或掉线
        //    static float LastAttackTime = 0.0f;
        //    float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()); // 或者使用 GetTickCount64() / 1000.0f

        //    if (CurrentTime - LastAttackTime > 0.1f) // 0.1秒攻击一次，即每秒10次
        //    {
        //        // 方案 A: 标准伤害函数 (推荐)
        //        // 参数1: 伤害数值 (比如 9999)
        //        // 参数2: 伤害来源 (MyChar)
        //        Monster->apply_custom_damage_self(1000.0f, MyChar);

        //        // 方案 B: 如果方案A没反应，尝试 P2P 击杀接口 (这通常是联机用的)
        //        // Monster->API_kill_enemy_P2P(
        //        //     10000.0f,            // Damage
        //        //     nullptr,             // DamageType (可能需要创建一个 UDamageType)
        //        //     Monster->K2_GetActorLocation(), // HitLocation
        //        //     FVector(0,0,1),      // HitNormal
        //        //     nullptr,             // HitComponent
        //        //     Monster->bone_head,  // BoneName
        //        //     FVector(1,0,0),      // ShotDirection
        //        //     MyChar->Controller,  // Instigator
        //        //     MyChar,              // DamageCauser
        //        //     FHitResult(),        // HitInfo (空结构体)
        //        //     true,                // is_Crit
        //        //     Monster,             // HitActor
        //        //     0, 0                 // indexs
        //        // );

        //        LastAttackTime = CurrentTime;
        //    }
        //}
    }




    void Loop()
    {

        auto MyController = GetPlayerController();
        if (!MyController) return;
        auto MyChar = GetLocalPlayerChar();
        if (!MyChar) return;
        auto DefaultChar = APlayer_char_main_C::GetDefaultObj();
		if (!DefaultChar) return;
        
        RunAimbot(MyController, MyChar);


        // [无敌模式] (保持之前的状态位修改，这对联机最有效)
        if (bGodMode)
        {
            if (MyChar->HP_current < MyChar->HP_max)
                MyChar->HP_current = MyChar->HP_max * 100.0f;
            if (MyChar->shield_current < MyChar->shield_max)
                MyChar->shield_current = MyChar->shield_max * 100.0f;
            MyChar->Is_down_ = false;
            MyChar->Is_death_ = false;
            MyChar->resistence_god_mode = true;
            MyChar->rolling_no_damage = true;
            MyChar->rolling_GOD_timer_state_ = 5.0f;
        }

        // [无限耐力] (保持之前)
        if (bUStamina)
        {
            MyChar->stamina_current = MyChar->stamina_max;
            MyChar->stamina_cost_sprint_base = 0.0f;
        }

        // [无限子弹] (保持之前，增加弹夹锁定)
        if (bInfiniteAmmo)
        {
            MyChar->CW_BulletCount = 9999;
            MyChar->Ammo_current_state = 9999;
            MyChar->CW_MaxAmmo_Clip = 999;
            if (MyChar->CW_AmmoClip.Num() > 0)
            {
                for (int i = 0; i < MyChar->CW_AmmoClip.Num(); i++)
                    MyChar->CW_AmmoClip[i] = 999;
            }
        }

        // [无后坐力/无限射程]
        if (bNoRecoil)
        {
            MyChar->recoil_up = 0.0f;
            MyChar->recoil_right = 0.0f;
            MyChar->shooting_recoil_up = 0.0f;
            MyChar->shooting_recoil_left = 0.0f;
            MyChar->shooting_recoil_right = 0.0f;
            MyChar->shooting_spread_current = 0.0f;
            MyChar->CW_Spread = 0.0f;
            MyChar->UI_crosshair_Spread_aiming = 0.0f;
        }
        else
        {
            // 还原后坐力
            float default_recoil_up = MyChar->recoil_up;
            float default_recoil_right = MyChar->recoil_right;
            float default_shooting_recoil_up = MyChar->shooting_recoil_up;
            float default_shooting_recoil_left = MyChar->shooting_recoil_left;
            float default_shooting_recoil_right = MyChar->shooting_recoil_right;
            float default_shooting_spread_current = MyChar->shooting_spread_current;
            float default_CW_Spread = MyChar->CW_Spread;
            float default_UI_crosshair_Spread_aiming = MyChar->UI_crosshair_Spread_aiming;

            MyChar->recoil_up = default_recoil_up;
            MyChar->recoil_right = default_recoil_right;
            MyChar->shooting_recoil_up = default_shooting_recoil_up;
            MyChar->shooting_recoil_left = default_shooting_recoil_left;
            MyChar->shooting_recoil_right = default_shooting_recoil_right;
            MyChar->shooting_spread_current = default_shooting_spread_current;
            MyChar->CW_Spread = default_CW_Spread;
            MyChar->UI_crosshair_Spread_aiming = default_UI_crosshair_Spread_aiming;
        }

        // [射程]
        if (bShootRange)
        {
            MyChar->Melee_range_gain_ = 2147480000.0f;
            MyChar->shooting_range_current = 2147480000.0f;
            MyChar->CW_MaxRange = 2147480000.0f;
        }
        else
        {
            // 还原射程
            float default_Melee_range_gain_ = MyChar->Melee_range_gain_;
            float default_shooting_range_current = MyChar->shooting_range_current;
            float default_CW_MaxRange = MyChar->CW_MaxRange;

            MyChar->Melee_range_gain_ = default_Melee_range_gain_;
            MyChar->shooting_range_current = default_shooting_range_current;
            MyChar->CW_MaxRange = default_CW_MaxRange;
        }

        // [射速]
        if (bAttackSpeed)
        {
            MyChar->shooting_delay = 0.0f;
            MyChar->shooting_delay_current = 0.0f;
            MyChar->CW_Fire_Delay = 0.0f;
            MyChar->CW_Burst_Delay = 0.0f;
        }
        else
        {
            // 还原射速
            float default_shootint_delay = MyChar->shooting_delay;
            float default_shooting_delay_current = MyChar->shooting_delay_current;
            float default_CW_Fire_Delay = MyChar->CW_Fire_Delay;
            float default_CW_Burst_Delay = MyChar->CW_Burst_Delay;
            MyChar->shooting_delay = default_shootint_delay;
            MyChar->shooting_delay_current = default_shooting_delay_current;
            MyChar->CW_Fire_Delay = default_CW_Fire_Delay;
            MyChar->CW_Burst_Delay = default_CW_Burst_Delay;
        }

        // ==============================================================
        // [经济修改 - 进阶版]
        // 不再强行锁 ECO_money (会被回弹)，改为修改"物价"和"获取率"
        // ==============================================================
        if (bInfiniteMoney)
        {
            // ----------------------------------------------------------
            // 策略 A: 0元购 (修改价格系数)
            // SDK 显示有 price_Store_ 等变量，通常默认为 1.0f
            // 将其设为 0.0f，客户端计算出的购买价格就是 0，服务器大概率会通过
            // ----------------------------------------------------------
            MyChar->price_Store_ = 0.0f;             // 商店价格
            MyChar->price_Build_ = 0.0f;             // 建造价格
            MyChar->price_weapon_attach_ = 0.0f;     // 配件价格
            MyChar->price_Store_skill_gain_ = -1.0f; // 尝试通过技能增益进一步压低价格

            // ----------------------------------------------------------
            // 策略 B: 超级掉落 (修改获取倍率)
            // 当你在游戏中捡钱时，倍率会生效
            // ----------------------------------------------------------
            MyChar->money_loot_rate_ = 100000.0f;            // 金钱掉落倍率 1000倍
            MyChar->money_loot_rate_skill_gain_ = 100000.0f; // 技能带来的金钱倍率
            MyChar->item_loot_rate_ = 100000.0f;             // 物品掉落倍率

            // ----------------------------------------------------------
            // 策略 C: 科技点获取 (修改击杀奖励)
            // ECO_tech 改不动，就改"击杀获取量"
            // ----------------------------------------------------------
            MyChar->tech_get_kill_skill_gain = 100000.0f; // 杀一个怪给 5000 科技
            MyChar->tech_get_day_skill_gain = 100000.0f;  // 每天给 5000 科技

            // ----------------------------------------------------------
            // 策略 D: 死亡不掉落
            // ----------------------------------------------------------
            MyChar->death_cost_state_rate_ = 0.0f;
            MyChar->death_cost_Build_state_rate_ = 0.0f;

            // ----------------------------------------------------------
            // 策略 E: 还是尝试锁一下本地值，用于欺骗本地 UI 的"可购买"检查
            // 有些游戏是: if (UI_Money >= Item_Price) { SendPacket; }
            // 所以本地改大可以让按钮亮起来，配合 0 元购使用效果极佳
            // ----------------------------------------------------------
            if (MyChar->ECO_money < 99999.0f)
                MyChar->ECO_money = 99999.0f;
            if (MyChar->ECO_tech < 99999.0f)
                MyChar->ECO_tech = 99999.0f;
            // ECO_bullet 你说这个有效，那就继续锁
            if (MyChar->ECO_bullet < 99999.0f)
                MyChar->ECO_bullet = 99999.0f;
        }
        else
        {
            // [可选] 关闭功能时还原物价 (防止负面影响)
            // 这里为了简单不写还原逻辑，建议开启后不要关闭，或者重启游戏还原
        }

        if (bSpeedHack)
        {
            if (DefaultChar->CharacterMovement) 
            {
                fOrg_SprintSpeed = DefaultChar->move_speed_sprint;
                fOrg_WalkSpeed = DefaultChar->move_speed_walk;
            }
            
            if (MyChar->CharacterMovement && (fOrg_SprintSpeed <= 0.0f || fOrg_WalkSpeed <= 0.0f))
            {
                fOrg_SprintSpeed = MyChar->move_speed_sprint;
                fOrg_WalkSpeed = MyChar->move_speed_walk;
            }


            MyChar->CharacterMovement->MaxWalkSpeed *= fMoveSpeedMultiplier;
            MyChar->CharacterMovement->MaxWalkSpeedCrouched *= fMoveSpeedMultiplier;
            MyChar->move_speed_sprint = DefaultChar->move_speed_sprint * fMoveSpeedMultiplier;
            MyChar->move_speed_walk = DefaultChar->move_speed_walk * fMoveSpeedMultiplier;
        }
        else 
        {
            // [第三步] 还原：如果存有备份数据，说明刚才开过挂，现在需要还原
            if (fOrg_SprintSpeed > 0.0f)
            {
                if (MyChar->CharacterMovement) {
                    MyChar->CharacterMovement->MaxWalkSpeed = fOrg_SprintSpeed;
                }

                MyChar->move_speed_sprint = fOrg_SprintSpeed;
                MyChar->move_speed_walk = fOrg_WalkSpeed;

                // [第四步] 重置：将备份标记设回 -1，等待下次开启时重新备份
                fOrg_SprintSpeed = -1.0f;
                fOrg_WalkSpeed = -1.0f;
			}
		}
        if (bMultipleJumpTimes) 
        {
            MyChar->JumpMaxCount = iJumpTimes;
        }
        else 
        {
            MyChar->JumpMaxCount = 1;
        }

        if (bHighJump)
        {
            if (fOrg_JumpZVelocity < 0.0f)
            {
                // 尝试从默认对象获取，如果默认是0则从当前对象获取
                if (DefaultChar->CharacterMovement)
                    fOrg_JumpZVelocity = DefaultChar->CharacterMovement->JumpZVelocity;

                // 双重保险：如果默认值为0（某些游戏可能是动态初始化的），就取当前的
                if (fOrg_JumpZVelocity <= 0.0f)
                    fOrg_JumpZVelocity = MyChar->CharacterMovement->JumpZVelocity;
            }
            MyChar->CharacterMovement->JumpZVelocity = fOrg_JumpZVelocity * fJumpMultiplier;
        }
        else
        {
            // 还原逻辑
            // 只有当有备份数据时才执行还原，避免每帧重复赋值
            if (fOrg_JumpZVelocity > 0.0f)
            {
                if (MyChar->CharacterMovement)
                {
                    MyChar->CharacterMovement->JumpZVelocity = fOrg_JumpZVelocity;
                }
                // 重置备份标志，等待下次开启
                fOrg_JumpZVelocity = -1.0f;
            }
        }

        if (bExpRate)
        {
            auto DefaultChar = APlayer_char_main_C::GetDefaultObj();
            float base;
            base = DefaultChar->EXP_rate__state_gain_;
            MyChar->EXP_rate__state_gain_ = base * fExpRateMultiplier;
        }
    }

    void Esp()
    {
        // 自瞄绘制
        if (bAimbot && bDrawFOV)
        {
            auto io = ImGui::GetIO();
            ImGui::GetBackgroundDrawList()->AddCircle(
                ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2),
                fAimbotFOV,
                ImColor(255, 255, 255, 60),
                64);
        }
        if (!bEspEnable)
            return;

        APlayerController *PC = GetPlayerController();
        APlayer_char_main_C *MyChar = GetLocalPlayerChar();
        UWorld *World = UWorld::GetWorld();

        if (!PC || !MyChar || !World || !World->PersistentLevel)
            return;

        TArray<AActor *> Actors = World->PersistentLevel->Actors;

        for (int i = 0; i < Actors.Num(); i++)
        {
            AActor *Actor = Actors[i];
            if (!Actor || Actor == MyChar)
                continue;

            // 距离计算 (UE单位通常是厘米，/100 换算成米)
            float Distance = MyChar->GetDistanceTo(Actor) / 100.0f;
            // if (Distance > 400.0f) continue; // 400米外不显示

            FVector Pos = Actor->K2_GetActorLocation();

            // --- 核心：使用 switch 进行分流 ---
            switch (GetActorType(Actor))
            {
            case EActorType::Player:
            {
                if (!bEspPlayer)
                    break; // 开关判断

                auto Player = static_cast<APlayer_char_main_C *>(Actor);
                // if (Player->Is_death_ || Player->Is_down_) break; // 死了不画
                std::string PlayerName = Player->PlayerState->PlayerNamePrivate.ToString();
                char Buf[64];
                sprintf_s(Buf, "Player-%s [%.0fm]\nHP: %d",PlayerName, Distance, (int)Player->HP_current);

                // 绿色显示
                DrawText3D(PC, Pos + FVector(0, 0, 100), Buf, col_Player);
                break;
            }

            case EActorType::Enemy:
            {
                if (!bEspMonster)
                    break;

                auto Monster = static_cast<AMG_AI_actor_master_C *>(Actor);
                if (Monster->is_death)
                    break; // 死了不画

                // 获取怪物名字
                //std::string mName = UKismetSystemLibrary::GetClassDisplayName(Monster->Class).ToString();

                char Buf[128];
				// !!!!!!!!--debug--!!!!!!!!!!
                //std::string ClassName = "Unknown";
                //ClassName = Monster->Class->GetName().c_str();
                // std::string ObjName = Monster->GetName().c_str(); // 获取对象名
                //sprintf_s(Buf, "Class: %s -> %s [%.0fm]\nHP: %d", ClassName, ObjName, Distance, (int)Monster->HP_current);
                
                //int ClassID = Monster->Class->Name.ComparisonIndex : -1;
                //int ObjID = Monster->Name.ComparisonIndex;
                //sprintf_s(Buf, "IDs: %d -> %d\nHP: %d", ClassID, ObjID, (int)Monster->HP_current);
                // !!!!!!!!--debug--!!!!!!!!!!

                sprintf_s(Buf, "Enemy [%.0fm]\nHP: %d",Distance, (int)Monster->HP_current);

                // 红色显示
                DrawText3D(PC, Pos + FVector(0, 0, 100), Buf, col_Monster);
                break;
            }

            case EActorType::Chest:
            {
                if (!bEspLT)
                    break; // 假设 LT 也是控制掉落的
                auto LT = static_cast<ALT_loot_box_main_C*>(Actor);
                char Buf[64];
                //sprintf_s(Buf, "Item [%.0fm]", Distance);

                //std::string lName = UKismetSystemLibrary::GetClassDisplayName(LT->Class).ToString();
                sprintf_s(Buf, "Item [%.0fm]", Distance);
                

                // 黄色显示
                DrawText3D(PC, Pos, Buf, col_Loot);
                break;
            }
            case EActorType::Fumo:
            {
                if (!bEspFumo)
                    break; 
                auto fumo = static_cast<AQS_quest_check_actor_04_FUMO_C*>(Actor);
                char Buf[64];
                //sprintf_s(Buf, "Item [%.0fm]", Distance);

                //std::string lName = UKismetSystemLibrary::GetClassDisplayName(LT->Class).ToString();
                sprintf_s(Buf, "Fumo [%.0fm]", Distance);


                // 黄色显示
                DrawText3D(PC, Pos, Buf, col_Fumo);
                break;
            }

            case EActorType::Unknown:
            default:
                // 未知类型直接跳过
                break;
            }
        }
    }

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //!!!!!!!!!!!!!-- debug esp --!!!!!!!!!!!!!!!!!!!!!
    // void Esp() {
    //     // 1. 强制开启绘制，方便调试
    //     // if (!bEspEnable) return;

    //    APlayerController* PC = GetPlayerController();
    //    auto MyChar = GetLocalPlayerChar();
    //    UWorld* World = UWorld::GetWorld();
    //    ImDrawList* Draw = ImGui::GetBackgroundDrawList();

    //    // --- [Debug 信息面板] ---
    //    // 在屏幕左上角显示核心指针状态
    //    char DebugBuf[256];
    //    sprintf_s(DebugBuf, "Debug Info:\nPC: %p\nMyChar: %p\nWorld: %p", PC, MyChar, World);
    //    Draw->AddText(ImVec2(10, 10), ImColor(255, 255, 255), DebugBuf);

    //    if (!PC || !World) return;

    //    // 检查 Level 和 Actor 数量
    //    if (!World->PersistentLevel) {
    //        Draw->AddText(ImVec2(10, 80), ImColor(255, 0, 0), "Error: PersistentLevel is NULL");
    //        return;
    //    }

    //    TArray<AActor*> Actors = World->PersistentLevel->Actors;
    //    sprintf_s(DebugBuf, "Actor Count: %d", Actors.Num());
    //    Draw->AddText(ImVec2(10, 80), ImColor(0, 255, 0), DebugBuf);

    //    if (Actors.Num() == 0) return;

    //    // --- [遍历 Actor] ---
    //    int DrawCount = 0;
    //    int W2SFailCount = 0;

    //    for (int i = 0; i < Actors.Num(); i++) {
    //        AActor* Actor = Actors[i];
    //        if (!Actor || Actor == (AActor*)MyChar) continue;

    //        FVector Pos = Actor->K2_GetActorLocation();
    //        FVector2D ScreenPos;

    //        // 2. 测试坐标转换
    //        bool bW2S = PC->ProjectWorldLocationToScreen(Pos, &ScreenPos, true);

    //        // 如果转换成功，画一个小圆点，不进行任何类型过滤
    //        if (bW2S) {
    //            // 简单的边界检查
    //            if (ScreenPos.X > 0 && ScreenPos.Y > 0) {
    //                Draw->AddCircleFilled(ImVec2(ScreenPos.X, ScreenPos.Y), 3.0f, ImColor(255, 255, 255));

    //                // 3. 打印前几个 Actor 的名字，看看 IsA 为什么失败
    //                // 只显示前 5 个距离最近的，防止刷屏
    //                if (DrawCount < 5) {
    //                    std::string CName = Actor->GetName();

    //                    // 如果 SDK 没有 ToString，尝试用 Actor->Name.GetName() 或者手动查看内存
    //                    // 这里假设你可以获取名字字符串

    //                    char NameBuf[128];
    //                    // 如果无法获取名字，只显示距离
    //                    float Dist = MyChar ? (MyChar->GetDistanceTo(Actor) / 100.0f) : 0.0f;

    //                    // 尝试判断类型并显示结果
    //                    EActorType Type = GetActorType(Actor);
    //                    const char* TypeStr = "Unknown";
    //                    if (Type == EActorType::Player) TypeStr = "Player";
    //                    else if (Type == EActorType::Enemy) TypeStr = "Enemy";
    //                    else if (Type == EActorType::Chest) TypeStr = "Chest";

    //                    sprintf_s(NameBuf, "%d: [%s] Type:%s Dist:%.0f", i, CName.c_str(), TypeStr, Dist);
    //                    Draw->AddText(ImVec2(ScreenPos.X + 5, ScreenPos.Y), ImColor(255, 255, 255), NameBuf);
    //                }
    //                DrawCount++;
    //            }
    //        }
    //        else {
    //            W2SFailCount++;
    //        }
    //    }

    //    // 打印 W2S 统计
    //    char StatBuf[64];
    //    sprintf_s(StatBuf, "Drawn: %d | W2S Failed: %d", DrawCount, W2SFailCount);
    //    Draw->AddText(ImVec2(10, 100), ImColor(255, 255, 0), StatBuf);
    //}

    //!!!!!!!--debug magic bullet--!!!!!!!!!!
    // 独立的子弹调试开关
    //bool bDebugBullets = true;   // 你可以先默认 true，确定完类名后关掉
    //
    //void DebugBulletsStandalone() {

    //    if (bDebugBullets)
    //    {
    //        APlayerController* PC = GetPlayerController();
    //        APlayer_char_main_C* MyChar = GetLocalPlayerChar();
    //        UWorld* World = UWorld::GetWorld();

    //        if (!PC || !World || !World->PersistentLevel || !MyChar)
    //            return;

    //        ImDrawList* Draw = ImGui::GetBackgroundDrawList();
    //        TArray<AActor*> Actors = World->PersistentLevel->Actors;

    //        int BulletLikeCount = 0;

    //        for (int i = 0; i < Actors.Num(); ++i)
    //        {
    //            AActor* Actor = Actors[i];
    //            if (!Actor)
    //                continue;

    //            float Dist = MyChar->GetDistanceTo(Actor) / 100.0f;
    //            if (Dist > 80.0f)  // 只看 80m 内的东西，防止刷屏
    //                continue;

    //            FVector Pos = Actor->K2_GetActorLocation();
    //            FVector2D sp;
    //            if (!PC->ProjectWorldLocationToScreen(Pos, &sp, true))
    //                continue;

    //            std::string Name = Actor->GetName();

    //            // 只筛选名字里可能是子弹的
    //            if (Name.find("Bullet") == std::string::npos &&
    //                Name.find("Projectile") == std::string::npos &&
    //                Name.find("Arrow") == std::string::npos &&
    //                Name.find("Tracer") == std::string::npos)
    //                continue;

    //            // 给这类东西画个标记
    //            Draw->AddCircleFilled(ImVec2(sp.X, sp.Y), 4.0f, ImColor(0, 255, 255, 200));

    //            char buf[160];
    //            sprintf_s(buf, "%d: %s (%.1fm)", i, Name.c_str(), Dist);
    //            Draw->AddText(ImVec2(sp.X + 6, sp.Y), ImColor(0, 255, 255), buf);

    //            BulletLikeCount++;
    //            if (BulletLikeCount >= 12)  // 最多显示 12 个，够你看名字了
    //                break;
    //        }

    //        char stat[64];
    //        sprintf_s(stat, "[DebugBullets] Count = %d", BulletLikeCount);
    //        Draw->AddText(ImVec2(10, 120), ImColor(255, 255, 0), stat);
    //    }
    //}
    //!!!!!!!--debug--!!!!!!!!!!

    //直接改钱无效，调用API给予
    AMG_AI_actor_master_C* GetAnyValidMonster()
    {
        UWorld* World = UWorld::GetWorld();
        if (!World || !World->PersistentLevel) return nullptr;

        TArray<AActor*> Actors = World->PersistentLevel->Actors;
        for (AActor* Actor : Actors)
        {
            if (!Actor || !Actor->IsA(AMG_AI_actor_master_C::StaticClass()))
                continue;

            auto Monster = static_cast<AMG_AI_actor_master_C*>(Actor);
            // 最好找一个没死的，虽然有些逻辑死了也能调用
            if (!Monster->is_death && Monster->HP_current > 0)
            {
                return Monster;
            }
        }
        return nullptr;
    }


   

    void Init()
    {
        ImGuiIO &io = ImGui::GetIO();
        auto MyController = GetPlayerController();
        Loop();
        Esp();
        RunDebugScanner(GetPlayerController(), GetLocalPlayerChar());
        //DebugBulletsStandalone();

        if (isOpen)
        {
            io.MouseDrawCursor = true;
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
            if (MyController)
                MyController->bShowMouseCursor = false;
            while (ShowCursor(TRUE) < 0)
                ;
        }
        else
        {
            // [菜单关闭]
            io.MouseDrawCursor = false;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

            return;
        }

        static bool styled = false;
        if (!styled)
        {
            ImGui::StyleColorsDark();
            styled = true;
        }

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        if (noTitleBar)
            flags |= ImGuiWindowFlags_NoTitleBar;

        ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Wray-lee's G2_depart cheat", &isOpen, flags))
        {
            if (ImGui::BeginTabBar("idTab")) 
            {
                if (ImGui::BeginTabItem("Cheat"))
                {
                    if (ImGui::CollapsingHeader("Player Features", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::Checkbox("God Mode", &bGodMode);
                        ImGui::Checkbox("Unlimited Stamina", &bUStamina);
                        ImGui::Checkbox("Infinite Ammo", &bInfiniteAmmo);
                        ImGui::Checkbox("No Recoil", &bNoRecoil);
                        ImGui::Checkbox("Shooting Speed", &bAttackSpeed);
                        ImGui::Checkbox("Unlimited Range", &bShootRange);
                        ImGui::Checkbox("Instant Skill", &bInstantSkill);
                        ImGui::Spacing();
                        // ImGui::Checkbox("Noclip", &bNoclip);
                        ImGui::Checkbox("EXP Rate Hack", &bExpRate);
                        if (bExpRate)
                            ImGui::SliderFloat("MultiplierExp", &fExpRateMultiplier, 1.0f, 10.0f, "%.1fx");
                        ImGui::Checkbox("High Jump ", &bHighJump);
                        if (bHighJump)
                            ImGui::SliderFloat("MultiplierJump", &fJumpMultiplier, 1.0f, 3.0f, "%.1fx");
                        ImGui::Checkbox("Unlimited Jump Times", &bMultipleJumpTimes);
                        ImGui::Checkbox("Speed Hack", &bSpeedHack);
                        if (bSpeedHack)
                            ImGui::SliderFloat("MultiplierSpeed", &fMoveSpeedMultiplier, 1.0f, 3.0f, "%.1fx");
                    }
                    // --- [ESP Visuals] --- (新增部分)
                    if (ImGui::CollapsingHeader("ESP Visuals", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        // 自动开启两种esp
                        if (ImGui::Checkbox("Enable ESP", &bEspEnable))
                        {
                            bEspPlayer = true;
                            bEspMonster = true;
                        }

                        if (bEspEnable)
                        {
                            ImGui::Indent(); // 缩进

                            // 1. 玩家 ESP + 颜色选择
                            ImGui::Checkbox("Show Players", &bEspPlayer);
                            ImGui::SameLine();
                            // ColorEdit4 显示颜色框，参数: 标签, float数组, 标志位
                            ImGui::ColorEdit4("##ColPly", col_Player, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview);

                            // 2. 怪物 ESP + 颜色选择
                            ImGui::Checkbox("Show Monsters", &bEspMonster);
                            ImGui::SameLine();
                            ImGui::ColorEdit4("##ColMon", col_Monster, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview);

                            // 3. 物品 ESP + 颜色选择
                            ImGui::Checkbox("Show Loot/Box", &bEspLT);
                            ImGui::SameLine();
                            ImGui::ColorEdit4("##ColLoot", col_Loot, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview);

							// 4. Fumo ESP + 颜色选择
                            ImGui::Checkbox("Show Fumo", &bEspFumo);
                            ImGui::SameLine();
                            ImGui::ColorEdit4("##ColLoot", col_Fumo, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview);

                            ImGui::Unindent(); // 取消缩进
                        }
                    }
                    if (ImGui::CollapsingHeader("Combat (Aimbot/Magic)", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        // 自瞄部分
                        ImGui::Checkbox("Aimbot (Right Mouse)", &bAimbot);
                        if (bAimbot)
                        {
                            ImGui::SliderFloat("FOV Radius", &fAimbotFOV, 50.0f, 1000.0f);
                            ImGui::SliderFloat("Smoothness", &fAimbotSmooth, 0.05f, 1.0f); // 越小越滑，1.0直接锁
                            ImGui::Checkbox("Draw FOV", &bDrawFOV);
                        }

                        ImGui::Separator();

                        // 魔法子弹部分
                        //ImGui::Checkbox("Magic Bullet", &bMagicBullet);
                        //if (ImGui::IsItemHovered())
                        //{
                        //    ImGui::SetTooltip("Teleports bullets to enemy Head instantly.\nWorks best with 'Unlimited Range'.");
                        //}
                    }

                    if (ImGui::CollapsingHeader("Resources"))
                    {
                        if (ImGui::Button("Add 10000 EXP"))
                        {
                            auto MyChar = GetLocalPlayerChar();
                            if (MyChar)
                            {
                                MyChar->Get_EXP(10000.0f);
                                MyChar->API_MP_EXP_get(10000.0f);
                            }
                        }
                        ImGui::Checkbox("Lock Money/Res (999k)", &bInfiniteMoney);

                        // [金钱修改按钮]
                        if (ImGui::Button("Add 50k Money"))
                        {
                            auto MyChar = GetLocalPlayerChar();
                            if (MyChar)
                            {
                                // 方法 A: 直接修改变量 (最稳)
                                MyChar->ECO_money += 50000.0f;

                                // 方法 B: 调用函数 (可能失败)
                                MyChar->get_ECO_money(50000.0f);

                                MyChar->UI_update_money(MyChar->ECO_money);
                            }
                        }
                        ImGui::SameLine();
                        ImGui::TextDisabled("(Try buying something to update UI)");

                        // [Tech 修改按钮]
                        if (ImGui::Button("Add 50k Tech"))
                        {
                            auto MyChar = GetLocalPlayerChar();
                            if (MyChar && MyChar->IsA(SDK::APlayer_BP_Child_C::StaticClass())) {
                                auto ChildPlayer = reinterpret_cast<SDK::APlayer_BP_Child_C*>(MyChar);
                                auto Monster = GetAnyValidMonster();
                                if (Monster && MyChar)
                                {
                                    // 尝试使用枚举 1 (NewEnumerator0)
                                    //MyChar->get_ECO_tech((SDK::EZero9_TECH_get_Enum)2, 50000.0f);
                                    //// 双重保险：直接修改变量
                                    //MyChar->ECO_tech += 50000.0f;

                                    Monster->give_ECO_Tech_to_target(ChildPlayer, (SDK::EZero9_TECH_get_Enum)1, 5000.0f);
                                    Monster->SVR_give_ECO_Tech_to_target(ChildPlayer, (SDK::EZero9_TECH_get_Enum)1, 5000.0f);

                                    MyChar->UI_update_money(MyChar->ECO_money);
                                }
                            }
                        }

                        // [Bullet Res 修改按钮]
                        if (ImGui::Button("Add 50k Res"))
                        {
                            auto MyChar = GetLocalPlayerChar();
                            if (MyChar)
                            {
                                // 尝试使用枚举 0
                                MyChar->get_ECO_bullet((SDK::EZero9_bullet_get_Enum)1, 50000.0f);
                                // 双重保险
                                MyChar->ECO_bullet += 50000.0f;

                                MyChar->UI_update_money(MyChar->ECO_money);
                            }
                        }
                    }
                    ImGui::EndTabItem();
                }
                

				// Debug Tab
                if (ImGui::BeginTabItem("Debug"))
                {
                    if (ImGui::CollapsingHeader("Debug Functions", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::Checkbox("World Scanner (Show Names)", &bDebugScanner);
                        if (bDebugScanner) 
                        {
                            ImGui::SliderFloat("Scan Distance", &fDebugScanDist, 100.0f, 5000.0f, "%.0f");

                            ImGui::Checkbox("Show Class Name Only", &bShowClassOnly);
                            ImGui::TextDisabled("Walk close to an object to see its Class Name.");
                            ImGui::TextDisabled("Use this Class Name in your code to filter ESP/Aimbot.");
                        
                        }
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
}