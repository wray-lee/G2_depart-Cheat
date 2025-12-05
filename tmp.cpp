#include "stdafx.h" 

using namespace SDK;

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
    bool bNoclip = false;        // [修复] 补上分号
    bool bInstantSkill = false;  // [新增] 声明变量

    float fMoveSpeedMultiplier = 1.0f;
    float test = 0.0f;

    // --- 辅助函数：获取本地玩家 ---
    // 将获取玩家的逻辑提取出来，方便在 Loop 和 Init 中复用
    APlayer_char_main_C* GetLocalPlayerChar() {
        UWorld* World = UWorld::GetWorld();
        if (!World || !World->OwningGameInstance || World->OwningGameInstance->LocalPlayers.Num() <= 0) return nullptr;
        
        ULocalPlayer* LocalPlayer = World->OwningGameInstance->LocalPlayers[0];
        if (!LocalPlayer || !LocalPlayer->PlayerController) return nullptr;
        
        return static_cast<APlayer_char_main_C*>(LocalPlayer->PlayerController->Pawn);
    }
	APlayerController* GetPlayerController() {
        APlayerController* MyController = nullptr;
        UWorld* World = UWorld::GetWorld();
        if (World && World->OwningGameInstance && World->OwningGameInstance->LocalPlayers.Num() > 0) {
            return static_cast<APlayerController*>(World->OwningGameInstance->LocalPlayers[0]->PlayerController);
        }
	}



    void Loop() {
        // 使用封装好的函数获取角色
        auto MyChar = GetLocalPlayerChar();
		auto MyController = GetPlayerController();
        if (!MyChar) return;

        // [无敌模式] 
        if (bGodMode) {
            if (MyChar->HP_current < MyChar->HP_max) MyChar->HP_current = MyChar->HP_max;
            if (MyChar->shield_current < MyChar->shield_max) MyChar->shield_current = MyChar->shield_max;
            MyChar->Is_down_ = false;
            MyChar->Is_death_ = false;
        }

        // [无限耐力]
        if (bUStamina) {
            if(MyChar->stamina_current < MyChar->stamina_max)MyChar->stamina_current = MyChar->stamina_max;
        }

        // [无限子弹]
        if (bInfiniteAmmo) {
            MyChar->CW_BulletCount = 99;
            MyChar->Ammo_current_state = 9999;
        }

        // [无后坐力]
        if (bNoRecoil) {
            MyChar->recoil_up = 0.0f;
            MyChar->recoil_right = 0.0f;
            MyChar->recoil_state_ = 0.0f; // 修正变量名，SDK里通常是 recoil_ 开头

            // 扩散
            MyChar->shooting_spread_current = 0.0f;
            MyChar->shooting_spread_state = 0.0f;
            MyChar->CW_Spread = 0.0f;
        }

        // [无限射程]
        if (bShootRange) {
            MyChar->shooting_range_current = 99999.0f;
            MyChar->CW_MaxRange = 99999.0f;
        }

        // [技能无CD]
        if (bInstantSkill) {
            MyChar->skill_CD = 0.0f;
            MyChar->skill_on_CD = false;
            MyChar->U_skill_CD = 0.0f;
            MyChar->U_skill_on_CD = false;
        }

        // [无限金币]

        if (bInfiniteMoney) {
            MyChar->get_ECO_money(2140000.0f);
            MyChar->get_ECO_tech((SDK::EZero9_TECH_get_Enum)1, 2140000.0f);
            MyChar->get_ECO_bullet((SDK::EZero9_bullet_get_Enum)0, 2140000.0f);
        }

        // [速度修改]
        if (bSpeedHack) {
            auto DefaultChar = APlayer_char_main_C::GetDefaultObj();
            if (DefaultChar) {
                MyChar->move_speed_sprint = DefaultChar->move_speed_sprint * fMoveSpeedMultiplier;
                MyChar->move_speed_walk = DefaultChar->move_speed_walk * fMoveSpeedMultiplier;
                MyChar->move_speed_crouch = DefaultChar->move_speed_crouch * fMoveSpeedMultiplier;
                MyChar->move_speed_walk_aim = DefaultChar->move_speed_walk * fMoveSpeedMultiplier;
                MyChar->move_speed_crouch_aim = DefaultChar->move_speed_crouch * fMoveSpeedMultiplier;
            }
        }
        else {
            auto DefaultChar = APlayer_char_main_C::GetDefaultObj();
            if (DefaultChar && MyChar->move_speed_sprint > DefaultChar->move_speed_sprint) {
                MyChar->move_speed_sprint = DefaultChar->move_speed_sprint;
                MyChar->move_speed_walk = DefaultChar->move_speed_walk;
                MyChar->move_speed_crouch = DefaultChar->move_speed_crouch;
                MyChar->move_speed_walk_aim = DefaultChar->move_speed_walk;
                MyChar->move_speed_crouch_aim = DefaultChar->move_speed_crouch;
            }
        }

        // [穿墙模式 Noclip]
        // 移动到这里，确保 CharMove 作用域正确
    auto CharMove = MyChar->CharacterMovement;
    
    // Noclip 速度 (建议在菜单里加个 Slider 调节)
    static float fFlySpeed = 25.0f; 
    // 状态记录
    static bool last_bNoclip = false;

    if (bNoclip)
    {
        // --- 1. 防止坠落 & 消除惯性 ---
        if (CharMove) {
            // [关键] 不要调用 SetMovementMode 函数，直接改变量！
            // 5 = Flying (飞行模式), 这样引擎就不会应用重力
            if (CharMove->MovementMode != (SDK::EMovementMode)5) {
                CharMove->MovementMode = (SDK::EMovementMode)5; 
            }
            
            // [关键] 每一帧强制把速度清零
            // 解决 "松开按键还在滑行" 的问题
            CharMove->Velocity = { 0.0f, 0.0f, 0.0f };
        }

        // --- 2. 3D 飞行逻辑 ---
        
        // 获取当前视角和位置
        FRotator CamRot = MyController->GetControlRotation();
        FVector Pos = MyChar->K2_GetActorLocation();
        
        // 计算方向
        FVector Fwd = GetCameraForward(CamRot); // 准星指哪，Fwd就是哪
        FVector Rgt = GetCameraRight(CamRot);   // 纯侧身方向

        bool bKeyPressed = false;

        // W: 向准星飞 (包括天上/地下)
        if (GetAsyncKeyState('W') & 0x8000) {
            Pos.X += Fwd.X * fFlySpeed;
            Pos.Y += Fwd.Y * fFlySpeed;
            Pos.Z += Fwd.Z * fFlySpeed;
            bKeyPressed = true;
        }
        // S: 向后退
        if (GetAsyncKeyState('S') & 0x8000) {
            Pos.X -= Fwd.X * fFlySpeed;
            Pos.Y -= Fwd.Y * fFlySpeed;
            Pos.Z -= Fwd.Z * fFlySpeed;
            bKeyPressed = true;
        }
        // D: 向右平移
        if (GetAsyncKeyState('D') & 0x8000) {
            Pos.X += Rgt.X * fFlySpeed;
            Pos.Y += Rgt.Y * fFlySpeed;
            bKeyPressed = true;
        }
        // A: 向左平移
        if (GetAsyncKeyState('A') & 0x8000) {
            Pos.X -= Rgt.X * fFlySpeed;
            Pos.Y -= Rgt.Y * fFlySpeed;
            bKeyPressed = true;
        }
        
        // Space: 垂直上升 (可选辅助)
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            Pos.Z += fFlySpeed;
            bKeyPressed = true;
        }
        // Ctrl: 垂直下降
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
            Pos.Z -= fFlySpeed;
            bKeyPressed = true;
        }

        // --- 3. 强制位移 (穿墙的核心) ---
        // 只有按键时才更新位置，省性能且防抖动
        if (bKeyPressed) {
            // 参数2 (bSweep): 必须是 false！这样才能无视墙壁直接穿过去。
            // 参数4 (bTeleport): 必须是 true！告诉物理引擎这是瞬移。
            MyChar->K2_SetActorLocation(Pos, false, nullptr, true);
        }
    }
    else
    {
        // --- 4. 关闭时恢复 ---
        if (last_bNoclip) {
            if (CharMove) {
                // 恢复为步行 (1 = Walking)
                CharMove->MovementMode = (SDK::EMovementMode)1; 
                // 清空速度，防止恢复瞬间摔死
                CharMove->Velocity = { 0.0f, 0.0f, 0.0f };
            }
            // 注意：我们从未调用 SetActorEnableCollision(false)，所以这里不需要恢复碰撞
            // 只要 K2_SetActorLocation 不再强制执行，碰撞自然就有了
        }
    }
    
    last_bNoclip = bNoclip;
    }

    void Init() {
        ImGuiIO& io = ImGui::GetIO();

        // 获取控制器以控制鼠标显示 (仅用于菜单开关逻辑)
        // 注意：这里可能获取不到，需要判空
		auto MyController = GetPlayerController();

        if (isOpen) {
            io.MouseDrawCursor = true;
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            if (MyController) MyController->bShowMouseCursor = false;
        } else {
            io.MouseDrawCursor = false;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
            if (MyController) MyController->bShowMouseCursor = false;
            Loop();
            return;
        }

        static bool styled = false;
        if (!styled) {
            ImGui::StyleColorsDark();
            styled = true;
        }

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        if (noTitleBar) flags |= ImGuiWindowFlags_NoTitleBar;

        ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Zero Sievert Internal", &isOpen, flags)) {
            ImGui::Text("Press INSERT/HOME to Toggle Menu");
            ImGui::Separator();

            if (ImGui::CollapsingHeader("Player Features", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("God Mode", &bGodMode);
                ImGui::Checkbox("Unlimited Stamina", &bUStamina);
                ImGui::Checkbox("Infinite Ammo", &bInfiniteAmmo);
                ImGui::Checkbox("No Recoil", &bNoRecoil);
                ImGui::Checkbox("Unlimited Range", &bShootRange);
                ImGui::Checkbox("Instant Skill", &bInstantSkill); // [新增] UI

                ImGui::Spacing();
                ImGui::Checkbox("Noclip", &bNoclip); 

                ImGui::Checkbox("Speed Hack", &bSpeedHack);
                if (bSpeedHack) {
                    ImGui::SliderFloat("Multiplier", &fMoveSpeedMultiplier, 1.0f, 10.0f, "%.1fx");
                }
            }

            if (ImGui::CollapsingHeader("Resources")) {
                ImGui::Checkbox("Lock Money/Res (999k)", &bInfiniteMoney);
                
                // [重点修复] 按钮增加金币逻辑
                if (ImGui::Button("Add 50k Money")) {
                    auto MyChar = GetLocalPlayerChar();
                    if (MyChar) {
                        // 方式1: 直接修改变量
                        //MyChar->ECO_money += 50000.0f;
                        
                        // 方式2: 尝试调用SDK函数 (如果链接器报错则注释掉)
                         MyChar->get_ECO_money(2140000.0f); 
                         MyChar->get_ECO_tech((SDK::EZero9_TECH_get_Enum)0, 2140000.0f);
                         MyChar->get_ECO_bullet((SDK::EZero9_bullet_get_Enum)0, 2140000.0f);
                        
                        // 方式3: 强制刷新UI (如果能找到UI函数)
                        // MyChar->UI_update_money(MyChar->ECO_money);
                        
                        DebugLog("[Menu] Added 50k money. Current: %f", MyChar->ECO_money);
                    }
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(?) Try buying something to update UI");
            }

            if (ImGui::CollapsingHeader("Settings")) {
                ImGui::Checkbox("No Title Bar", &noTitleBar);
            }
        }
        ImGui::End();
        Loop();
    }
}