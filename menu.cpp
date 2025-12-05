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
    bool bShootRange = false;
    bool bSpeedHack = false;
    bool bNoclip = false;
    bool bInstantSkill = false;
    bool bExpRate = false;

    float fExpRateMultiplier = 2.0f;
    float fMoveSpeedMultiplier = 2.0f;

    // ESP
    bool bEspEnable = false;                         // ESP 总开关
    bool bEspChildEnable[3] = {false, false, false}; // 子开关
    bool bEspMonster = false;                        // 显示怪物
    bool bEspPlayer = false;                         // 显示其他玩家
    bool bEspLT = false;                             // 显示掉落

    enum class EActorType
    {
        Unknown = 0,
        Player,
        Enemy,
        Chest
    };
    float col_Player[4] = {0.2f, 1.0f, 0.2f, 1.0f};  // 默认绿色
    float col_Monster[4] = {1.0f, 0.2f, 0.2f, 1.0f}; // 默认红色
    float col_Loot[4] = {1.0f, 1.0f, 0.0f, 1.0f};    // 默认黄色

    // Aimbot
    bool bAimbot = false;
    bool bMagicBullet = false;
    float fAimbotFOV = 150.0f;  // 自瞄范围
    float fAimbotSmooth = 1.0f; // 平滑度 (1.0 = 锁死)
    bool bDrawFOV = true;       // 绘制 FOV 圈

    // 获取敌人头部位置的函数
    FVector GetEnemyHeadPos(AActor *Actor)
    {
        if (!Actor)
            return {0, 0, 0};

        auto Monster = static_cast<AMG_AI_actor_master_C *>(Actor);

        // 使用 bone_head (需要 ACharacter 的 Mesh 有效)
        // Monster->bone_head 存储了头部骨骼的名字
        if (Monster->Mesh && Monster->bone_head.ComparisonIndex!=0) {
            return Monster->Mesh->GetSocketLocation(Monster->bone_head);
        }

        // 动态胶囊体高度
        //  不要用写死的 130.0f，改用胶囊体的一半高度 * 缩放
        //ACharacter 都有 CapsuleComponent
         if (Monster->CapsuleComponent) {
             float CapsuleHalfHeight = Monster->CapsuleComponent->GetScaledCapsuleHalfHeight();
             FVector Pos = Monster->K2_GetActorLocation();

            // 胶囊体中心在腰部，所以 Z + HalfHeight 大概就是头顶
            // 稍微减一点(比如 * 0.8) 也就是瞄眉毛的位置，防止射飞
            Pos.Z += (CapsuleHalfHeight * 0.8f);
            return Pos;
         }


        // 使用 see_Scene 组件
        // SDK 显示偏移 0x04E0 有一个 see_Scene，通常用于AI视野检测，位于头部/眼睛
        if (Monster->see_Scene)
        {
            return Monster->see_Scene->K2_GetComponentLocation();
        }





        // 简单的坐标偏移 (保底)
        // FVector Pos = Monster->K2_GetActorLocation();
        // Pos.Z += 130.0f; // 假设怪物头部高度
        // return Pos;
    }
    // 实体类型判断
    EActorType GetActorType(AActor *Actor)
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

        return EActorType::Unknown;
    }

    // 自瞄和魔法子弹
    void RunAimbot(APlayerController *PC, APlayer_char_main_C *MyChar)
    {
        AActor *CurrentTarget = nullptr;
        UWorld *World = UWorld::GetWorld();
        if (!World || !World->PersistentLevel)
            return;

        FVector2D ScreenCenter = {ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f};
        TArray<AActor *> Actors = World->PersistentLevel->Actors;

        AActor *BestTarget = nullptr;
        float BestDist = fAimbotFOV; // 初始距离设为FOV范围

        // --- 1. 寻找最佳目标 ---
        if (bAimbot || bMagicBullet)
        {
            for (AActor *Actor : Actors)
            {
                if (!Actor || Actor == MyChar)
                    continue;

                // 筛选: 必须是怪物类型
                if (GetActorType(Actor) != EActorType::Enemy)
                    continue;

                auto Monster = static_cast<AMG_AI_actor_master_C *>(Actor);
                if (Monster->is_death || Monster->HP_current <= 0)
                    continue;

                // 获取头部位置
                FVector HeadPos = GetEnemyHeadPos(Actor);
                FVector2D ScreenPos;

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
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!---------------------debug---------------!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (bAimbot || bMagicBullet)
        {
            FVector2D ScreenPos;
            if (PC->ProjectWorldLocationToScreen(TargetLoc, &ScreenPos, true))
            {
                auto DrawList = ImGui::GetBackgroundDrawList();

                // 1. 画一条线连过去
                FVector2D ScreenCenter = {ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f};
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
            PC->SetControlRotation(FinalRot);
        }

        // --- 3. 魔法子弹逻辑 ---
        if (bMagicBullet)
        {
            for (AActor *Actor : Actors)
            {
                if (!Actor)
                    continue;

                // [精准判定] 判断是否为子弹类 ABullet_Projectile_C
                if (Actor->IsA(ABullet_Projectile_C::StaticClass()))
                {

                    // 简单的归属判断：只有距离玩家一定范围内的子弹才处理 (防止吸走别人的子弹)
                    // 或者检查 Owner: if(Actor->Owner == MyChar)
                    float DistFromMe = MyChar->GetDistanceTo(Actor);

                    // 逻辑:
                    // 1. 距离玩家 > 100 (防止刚生成还没飞出去就瞬移，导致打中自己或判定失效)
                    // 2. 距离目标 > 100 (防止已经命中了还在不停瞬移)
                    if (DistFromMe > 100.0f && CurrentTarget->GetDistanceTo(Actor) > 50.0f)
                    {

                        // 核心：直接把子弹设置到敌人头部位置
                        // bSweep=false, bTeleport=true
                        Actor->K2_SetActorLocation(TargetLoc, false, nullptr, true);
                    }
                }
            }
        }
    }

    void DrawText3D(APlayerController *PC, const FVector &WorldPos, const char *Text, float *ColorFloat)
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

    APlayerController *GetPlayerController()
    {
        UWorld *World = UWorld::GetWorld();
        if (!World || !World->OwningGameInstance || World->OwningGameInstance->LocalPlayers.Num() <= 0)
            return nullptr;
        ULocalPlayer *LocalPlayer = World->OwningGameInstance->LocalPlayers[0];
        if (!LocalPlayer || !LocalPlayer->PlayerController)
            return nullptr;

        return static_cast<APlayerController *>(LocalPlayer->PlayerController);
    }
    APlayer_char_main_C *GetLocalPlayerChar()
    {
        APlayerController *MyController = GetPlayerController();

        if (MyController != nullptr)
        {
            return static_cast<APlayer_char_main_C *>(MyController->Pawn);
        }

        return nullptr;
    }

    void Loop()
    {
        auto MyController = GetPlayerController();
        auto MyChar = GetLocalPlayerChar();
        //
        RunAimbot(MyController, MyChar);

        if (!MyChar)
            return;

        // [无敌模式] (保持之前的状态位修改，这对联机最有效)
        if (bGodMode)
        {
            if (MyChar->HP_current < MyChar->HP_max)
                MyChar->HP_current = MyChar->HP_max;
            if (MyChar->shield_current < MyChar->shield_max)
                MyChar->shield_current = MyChar->shield_max;
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
            auto DefaultChar = APlayer_char_main_C::GetDefaultObj();
            float base = 600.0f;
            float walk = 300.0f;

            base = DefaultChar->move_speed_sprint;
            walk = DefaultChar->move_speed_walk;

            float safeMultiplier = fMoveSpeedMultiplier > 3.0f ? 3.0f : fMoveSpeedMultiplier;
            MyChar->move_speed_sprint = base * safeMultiplier;
            MyChar->move_speed_walk = walk * safeMultiplier;
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
                //ClassName = Monster->Class->GetName();
                // std::string ObjName = Monster->GetName(); // 获取对象名
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

    void Init()
    {
        ImGuiIO &io = ImGui::GetIO();
        auto MyController = GetPlayerController();
        Loop();
        Esp();

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
                    ImGui::SliderFloat("MultiplierExp", &fExpRateMultiplier, 1.0f, 100.0f, "%.1fx");
                ImGui::Checkbox("Speed Hack", &bSpeedHack);
                if (bSpeedHack)
                    ImGui::SliderFloat("MultiplierSpeed", &fMoveSpeedMultiplier, 1.0f, 100.0f, "%.1fx");
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
                ImGui::Checkbox("Magic Bullet", &bMagicBullet);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Teleports bullets to enemy Head instantly.\nWorks best with 'Unlimited Range'.");
                }
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
                    if (MyChar)
                    {
                        // 尝试使用枚举 1 (NewEnumerator0)
                        MyChar->get_ECO_tech((SDK::EZero9_TECH_get_Enum)2, 50000.0f);
                        // 双重保险：直接修改变量
                        MyChar->ECO_tech += 50000.0f;

                        MyChar->UI_update_money(MyChar->ECO_money);
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
            ImGui::End();
        }
    }
}