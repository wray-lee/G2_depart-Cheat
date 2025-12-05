#include "stdafx.h" 
#include <cmath> // 数学库
#include <windows.h> // Windows API

using namespace SDK;

// --- [Noclip 数学辅助函数] ---
#define M_PI 3.14159265358979323846f
#define DEG2RAD(x) ((x) * (M_PI / 180.0f))

FVector GetCameraForward(FRotator Rot) {
    float sp = sin(DEG2RAD(Rot.Pitch));
    float cp = cos(DEG2RAD(Rot.Pitch));
    float sy = sin(DEG2RAD(Rot.Yaw));
    float cy = cos(DEG2RAD(Rot.Yaw));
    return { cp * cy, cp * sy, sp };
}

FVector GetCameraRight(FRotator Rot) {
    float sy = sin(DEG2RAD(Rot.Yaw));
    float cy = cos(DEG2RAD(Rot.Yaw));
    return { -sy, cy, 0.0f };
}
// ----------------------------

namespace menu {
    bool isOpen = false;
    static bool noTitleBar = false;

    // 功能开关
    bool bGodMode = false;
    bool bUStamina = false;
    bool bInfiniteAmmo = false;
    bool bInfiniteMoney = false;
    bool bNoRecoil = false;
    bool bShootRange = false;
    bool bSpeedHack = false;
    bool bNoclip = false;
    bool bInstantSkill = false;

    float fMoveSpeedMultiplier = 1.0f;
    float test = 0.0f;

    APlayerController* GetPlayerController() {
        UWorld* World = UWorld::GetWorld();
        if (!World || !World->OwningGameInstance || World->OwningGameInstance->LocalPlayers.Num() <= 0) return nullptr;
        ULocalPlayer* LocalPlayer = World->OwningGameInstance->LocalPlayers[0];
        if (!LocalPlayer || !LocalPlayer->PlayerController) return nullptr;

        return static_cast<APlayerController*>(LocalPlayer->PlayerController);
    }
    APlayer_char_main_C* GetLocalPlayerChar() {
        APlayerController* MyController = GetPlayerController();

        if (MyController != nullptr) {
            return static_cast<APlayer_char_main_C*>(MyController->Pawn);
        }

        return nullptr;
    }

    void Loop() {
        auto MyChar = GetLocalPlayerChar();
        auto MyController = GetPlayerController();
        if (!MyChar) return;

        // [无敌模式] 
        if (bGodMode) {
            MyChar->HP_current = MyChar->HP_max;
            MyChar->shield_current = MyChar->shield_max;
            MyChar->Is_down_ = false;
            MyChar->Is_death_ = false;
        }

        // [无限耐力]
        if (bUStamina) {
            MyChar-> stamina_current = 2140000.0f;
            MyChar-> stamina_reset_step_timer_state = 0.0f;
            MyChar-> stamina_reset_delay_state = 0.0f;
            MyChar-> stamina_cost_sprint_base = 0.0f;
        }

        // [无限子弹]
        if (bInfiniteAmmo) {
            MyChar->CW_BulletCount = 2140000;
            MyChar->Ammo_current_state = 2140000;
        }

        // [无后坐力]
        if (bNoRecoil) {
            MyChar->recoil_up = 0.0f;
            MyChar->recoil_right = 0.0f;
            MyChar->recoil_state_ = 0.0f;
            MyChar->shooting_recoil_up = 0.0f;
            MyChar->shooting_recoil_left = 0.0f;
            MyChar->shooting_recoil_right = 0.0f;
            MyChar->shooting_spread_current = 0.0f;
            MyChar->shooting_spread_state = 0.0f;
            MyChar->CW_Spread = 0.0f;
        }

        // [无限射程]
        if (bShootRange) {
            MyChar->shooting_range_current = 2147483000.0f;
            MyChar->CW_MaxRange = 2147483000.0f;
        }

        // [技能无CD]
        if (bInstantSkill) {
            MyChar->skill_CD = 0.0f;
            MyChar->skill_on_CD = false;
            MyChar->U_skill_CD = 0.0f;
            MyChar->U_skill_on_CD = false;
        }

        // [无限金币 - 暴力锁定]
        // 既然函数调用无效，我们直接改写内存变量
        // 只要数值改了，哪怕UI不刷新，你去买东西时也会生效
        if (bInfiniteMoney) {
            MyChar->ECO_money = 214748.0f;
            MyChar->ECO_tech = 214748.0f;
            MyChar->ECO_bullet = 214748.0f;
        }

        // [速度修改]
        if (bSpeedHack) {
            auto DefaultChar = APlayer_char_main_C::GetDefaultObj();
            if (DefaultChar) {
                MyChar->move_speed_sprint = DefaultChar->move_speed_sprint * fMoveSpeedMultiplier;
                MyChar->move_speed_walk = DefaultChar->move_speed_walk * fMoveSpeedMultiplier;
                // ... 其他速度
            }
        }
        else {
            auto DefaultChar = APlayer_char_main_C::GetDefaultObj();
            if (DefaultChar && MyChar->move_speed_sprint > DefaultChar->move_speed_sprint) {
                MyChar->move_speed_sprint = DefaultChar->move_speed_sprint;
                MyChar->move_speed_walk = DefaultChar->move_speed_walk;
            }
        }

        // [Noclip 穿墙模式]
        //auto CharMove = MyChar->CharacterMovement;
        //static float fFlySpeed = 25.0f;
        //static bool last_bNoclip = false;

        //if (bNoclip && MyController)
        //{
        //    if (CharMove) {
        //        // 强制飞行模式
        //        if (CharMove->MovementMode != (SDK::EMovementMode)5)
        //            CharMove->MovementMode = (SDK::EMovementMode)5;
        //        CharMove->Velocity = { 0.0f, 0.0f, 0.0f }; // 清空速度
        //    }

        //    FRotator CamRot = MyController->GetControlRotation();
        //    FVector Pos = MyChar->K2_GetActorLocation();
        //    FVector Fwd = GetCameraForward(CamRot);
        //    FVector Rgt = GetCameraRight(CamRot);

        //    bool bKeyPressed = false;
        //    if (GetAsyncKeyState('W') & 0x8000) { Pos = { Pos.X + Fwd.X * fFlySpeed, Pos.Y + Fwd.Y * fFlySpeed, Pos.Z + Fwd.Z * fFlySpeed }; bKeyPressed = true; }
        //    if (GetAsyncKeyState('S') & 0x8000) { Pos = { Pos.X - Fwd.X * fFlySpeed, Pos.Y - Fwd.Y * fFlySpeed, Pos.Z - Fwd.Z * fFlySpeed }; bKeyPressed = true; }
        //    if (GetAsyncKeyState('D') & 0x8000) { Pos = { Pos.X + Rgt.X * fFlySpeed, Pos.Y + Rgt.Y * fFlySpeed, Pos.Z + Rgt.Z * fFlySpeed }; bKeyPressed = true; }
        //    if (GetAsyncKeyState('A') & 0x8000) { Pos = { Pos.X - Rgt.X * fFlySpeed, Pos.Y - Rgt.Y * fFlySpeed, Pos.Z - Rgt.Z * fFlySpeed }; bKeyPressed = true; }
        //    if (GetAsyncKeyState(VK_SPACE) & 0x8000) { Pos.Z += fFlySpeed; bKeyPressed = true; }
        //    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) { Pos.Z -= fFlySpeed; bKeyPressed = true; }

        //    // 强制瞬移 (穿墙)
        //    if (bKeyPressed) {
        //        MyChar->K2_SetActorLocation(Pos, false, nullptr, true);
        //    }
        //}
        //else
        //{
        //    if (last_bNoclip && CharMove) {
        //        CharMove->MovementMode = (SDK::EMovementMode)1; // 恢复行走
        //        CharMove->Velocity = { 0.0f, 0.0f, 0.0f };
        //    }
        //}
        //last_bNoclip = bNoclip;
    }

    void Init() {
        ImGuiIO& io = ImGui::GetIO();
        auto MyController = GetPlayerController();

        if (isOpen) {
            io.MouseDrawCursor = true;
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
            if (MyController) MyController->bShowMouseCursor = false;
            Loop();
        }
        else {
            // [菜单关闭]
            io.MouseDrawCursor = false;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

            if (MyController) MyController->bShowMouseCursor = false;


            ::SetCursor(NULL);
            ::ShowCursor(FALSE);

            Loop();
            return;
        }
        if (!isOpen) {
            io.MouseDrawCursor = false;
            if (MyController) MyController->bShowMouseCursor = false;
            ::SetCursor(NULL);
            ::ShowCursor(FALSE);
        }

        static bool styled = false;
        if (!styled) {
            ImGui::StyleColorsDark();
            styled = true;
        }

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        if (noTitleBar) flags |= ImGuiWindowFlags_NoTitleBar;

        ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Wray-lee's G2_depart cheat", &isOpen, flags)) {
            // ... (其他 Checkbox 代码) ...
            if (ImGui::CollapsingHeader("Player Features", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("God Mode", &bGodMode);
                ImGui::Checkbox("Unlimited Stamina", &bUStamina);
                ImGui::Checkbox("Infinite Ammo", &bInfiniteAmmo);
                ImGui::Checkbox("No Recoil", &bNoRecoil);
                ImGui::Checkbox("Unlimited Range", &bShootRange);
                ImGui::Checkbox("Instant Skill", &bInstantSkill);
                ImGui::Spacing();
                //ImGui::Checkbox("Noclip", &bNoclip);
                ImGui::Checkbox("Speed Hack", &bSpeedHack);
                if (bSpeedHack) ImGui::SliderFloat("Multiplier", &fMoveSpeedMultiplier, 1.0f, 10.0f, "%.1fx");
            }

            if (ImGui::CollapsingHeader("Resources")) {
                ImGui::Checkbox("Lock Money/Res (999k)", &bInfiniteMoney);

                // [金钱修改按钮]
                if (ImGui::Button("Add 50k Money")) {
                    auto MyChar = GetLocalPlayerChar();
                    if (MyChar) {
                        // 方法 A: 直接修改变量 (最稳)
                        MyChar->ECO_money += 50000.0f;

                        // 方法 B: 调用函数 (可能失败)
                        MyChar->get_ECO_money(50000.0f);

                        MyChar->UI_update_money();

                    }
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(Try buying something to update UI)");

                // [Tech 修改按钮]
                if (ImGui::Button("Add 50k Tech")) {
                    auto MyChar = GetLocalPlayerChar();
                    if (MyChar) {
                        // 尝试使用枚举 1 (NewEnumerator0)
                        MyChar->get_ECO_tech((SDK::EZero9_TECH_get_Enum)2, 50000.0f);
                        // 双重保险：直接修改变量
                        MyChar->ECO_tech += 50000.0f;

                        MyChar->UI_update_money();
                    }
                }

                // [Bullet Res 修改按钮]
                if (ImGui::Button("Add 50k Res")) {
                    auto MyChar = GetLocalPlayerChar();
                    if (MyChar) {
                        // 尝试使用枚举 0
                        MyChar->get_ECO_bullet((SDK::EZero9_bullet_get_Enum)1, 50000.0f);
                        // 双重保险
                        MyChar->ECO_bullet += 50000.0f;

                        MyChar->UI_update_money();
                    }
                }
            }
            // ...
        }
        ImGui::End();
        Loop();
    }
}