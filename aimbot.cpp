#include <cmath>
#include <numbers>

// 简单的向量类 (如果 SDK 里有 FVector 可以直接用 SDK 的)
struct FVector { float X, Y, Z; };
struct FRotator { float Pitch, Yaw, Roll; };

#define M_PI 3.14159265358979323846f
#define U2D(angle) ((angle) * (180.0f / M_PI)) // 弧度转角度

// 核心算法：计算从 Start 点看向 End 点所需的 Pitch 和 Yaw
FRotator CalculateAngle(FVector Start, FVector End) {
    FVector Delta = { End.X - Start.X, End.Y - Start.Y, End.Z - Start.Z };
    float Hypotenuse = sqrt(Delta.X * Delta.X + Delta.Y * Delta.Y);

    FRotator Rotation;
    Rotation.Yaw = U2D(atan2(Delta.Y, Delta.X));
    Rotation.Pitch = U2D(atan2(Delta.Z, Hypotenuse));
    Rotation.Roll = 0.0f;

    return Rotation;
}

// 计算距离
float GetDistance(FVector a, FVector b) {
    return sqrt(pow(a.X - b.X, 2) + pow(a.Y - b.Y, 2) + pow(a.Z - b.Z, 2));
}


// 在 menu.cpp 或 aimbot.cpp 中
AActor* GetBestTarget(UWorld* World, APlayerController* MyController, APlayer_char_main_C* MyChar) {
    if (!World || !World->PersistentLevel) return nullptr;

    float MaxDist = 999999.0f; // 或者设定一个最大瞄准距离
    AActor* BestTarget = nullptr;

    // 遍历所有 Actor
    // 注意：SDK 里的 TArray 遍历方式取决于 SDK 的实现，通常是 Actors.Data[i]
    auto& Actors = World->PersistentLevel->Actors;
    
    for (int i = 0; i < Actors.Count; i++) {
        AActor* Actor = Actors.Data[i];
        
        // 1. 基础过滤
        if (!Actor || Actor == MyChar) continue;
        
        // 2. 判断是否是敌人
        // 你的 SDK 里可能有 TeamID 或类似的判断，这里假设名字包含 "Enemy" 或者是特定的类
        // if (!Actor->IsA(AEnemy_C::StaticClass())) continue; 
        
        // 3. 判断是否存活 (根据你的 SDK 变量)
        // 假设 Actor 强转后有 Health 属性
        // auto Enemy = static_cast<APlayer_char_main_C*>(Actor);
        // if (Enemy->Health <= 0 || Enemy->Is_death_) continue;

        // 4. 获取位置
        FVector EnemyPos = Actor->K2_GetActorLocation(); 
        FVector MyPos = MyChar->K2_GetActorLocation();

        // 5. 简单逻辑：找距离最近的
        float Dist = GetDistance(MyPos, EnemyPos);
        if (Dist < MaxDist) {
            MaxDist = Dist;
            BestTarget = Actor;
        }
        
        // 进阶逻辑：计算屏幕 FOV (需要 WorldToScreen 函数)
    }
    return BestTarget;
}

// 在 menu::Loop() 内部调用
void RunAimbot() {
    if (!GetAsyncKeyState(VK_RBUTTON)) return; // 只有按住右键才瞄准

    // 1. 获取环境
    UWorld* World = UWorld::GetWorld();
    auto MyController = World->OwningGameInstance->LocalPlayers[0]->PlayerController;
    auto MyChar = static_cast<APlayer_char_main_C*>(MyController->Pawn);

    // 2. 找人
    AActor* Target = GetBestTarget(World, MyController, MyChar);
    if (!Target) return;

    // 3. 获取相机位置 (作为起于点)
    FVector CameraLoc = MyController->PlayerCameraManager->GetCameraLocation();
    
    // 4. 获取敌人头部位置 (作为终点)
    // 如果你有 Mesh 组件和 Bone Names，用 GetSocketLocation(FName("Head"))
    // 这里暂时用 Actor 坐标 + Z轴偏移来模拟头部
    FVector TargetPos = Target->K2_GetActorLocation(); 
    TargetPos.Z += 60.0f; // 假设头部在脚底往上 60 单位

    // 5. 计算角度
    FRotator AimRot = CalculateAngle(CameraLoc, TargetPos);

    // 6. 写入视角 (实现自瞄)
    MyController->SetControlRotation(AimRot);
}