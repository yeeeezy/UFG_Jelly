#include "JellyBullet.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Math/RotationMatrix.h"

AJellyBullet::AJellyBullet()
{
    PrimaryActorTick.bCanEverTick = true;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    Mesh->SetSimulatePhysics(true);
    Mesh->SetNotifyRigidBodyCollision(true);
    Mesh->OnComponentHit.AddDynamic(this, &AJellyBullet::OnMeshHit);
}

void AJellyBullet::BeginPlay()
{
    Super::BeginPlay();
    OriginalScale = Mesh->GetRelativeScale3D();
}

void AJellyBullet::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDeforming)
    {
        DeformTimer += DeltaTime;
        float Alpha = FMath::Clamp(DeformTimer / DeformDuration, 0.f, 1.f);

        ApplyDeformAnimation(Alpha);

        if (Alpha >= 1.f)
        {
            bIsDeforming = false;

            // 退出表面一点，避免嵌入
            SetActorLocation(GetActorLocation() + ImpactNormalWorld * 5.f);

            // 恢复物理
            Mesh->SetSimulatePhysics(true);
            Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
            Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

            Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Mesh->SetNotifyRigidBodyCollision(true);

            // 施加弹跳
            LaunchAwayFromSurface();
        }
    }
}

void AJellyBullet::OnMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                             UPrimitiveComponent* OtherComp, FVector NormalImpulse,
                             const FHitResult& Hit)
{
    if (bHasStopped || bIsDeforming)
        return;

    ImpactNormalWorld = Hit.ImpactNormal;
    if (ImpactNormalWorld.IsNearlyZero())
        ImpactNormalWorld = FVector::UpVector;
    ImpactNormalWorld.Normalize();

    RotateToSurfaceNormal();

    Mesh->SetSimulatePhysics(false);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetNotifyRigidBodyCollision(false);

    SetActorLocation(GetActorLocation() + ImpactNormalWorld * 2.f);

    // -------- 变形计算 --------
    FVector A = ImpactNormalWorld.GetAbs();

    float ScaleX = 1.f + (-A.X * SquashAmount + (1.f - A.X) * StretchAmount);
    float ScaleY = 1.f + (-A.Y * SquashAmount + (1.f - A.Y) * StretchAmount);
    float ScaleZ = 1.f + (-A.Z * SquashAmount + (1.f - A.Z) * StretchAmount);

    ScaleX = FMath::Clamp(ScaleX, 0.4f, 1.8f);
    ScaleY = FMath::Clamp(ScaleY, 0.4f, 1.8f);
    ScaleZ = FMath::Clamp(ScaleZ, 0.4f, 1.8f);

    DeformedScale = OriginalScale * FVector(ScaleX, ScaleY, ScaleZ);

    bIsDeforming = true;
    DeformTimer = 0.f;
}

void AJellyBullet::ApplyDeformAnimation(float Alpha)
{
    FVector NewScale;

    if (Alpha <= 0.5f)
    {
        float T = Alpha / 0.5f;
        NewScale = FMath::Lerp(OriginalScale, DeformedScale, FMath::SmoothStep(0.f, 1.f, T));
    }
    else
    {
        float T = (Alpha - 0.5f) / 0.5f;
        NewScale = FMath::Lerp(DeformedScale, OriginalScale, FMath::SmoothStep(0.f, 1.f, T));
    }

    Mesh->SetRelativeScale3D(NewScale);
}

void AJellyBullet::LaunchAwayFromSurface()
{
    if (CurrentBounceCount >= MaxBounceCount)
    {
        bHasStopped = true;
        return;
    }

    float ActualPower = BounceImpulseStrength * FMath::Pow(BounceDamping, CurrentBounceCount);
    CurrentBounceCount++;

    FVector PushDir = ImpactNormalWorld;
    if (PushDir.IsNearlyZero())
        PushDir = FVector::UpVector;
    PushDir.Normalize();

    // 防止 lambda 捕获悬空 this，使用弱指针
    TWeakObjectPtr<AJellyBullet> WeakThis(this);

    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle,
        [WeakThis, PushDir, ActualPower]()
        {
            if (!WeakThis.IsValid())
                return;

            AJellyBullet* Self = WeakThis.Get();

            if (Self->Mesh && Self->Mesh->IsSimulatingPhysics())
            {
                Self->Mesh->AddImpulse(PushDir * ActualPower, NAME_None, true);
            }
        },
        0.02f,
        false
    );
}

void AJellyBullet::RotateToSurfaceNormal()
{
    FVector Up = ImpactNormalWorld.GetSafeNormal();

    // 如果 Up 和世界Z轴太平行，换一个辅助轴
    FVector AuxAxis = FVector::UpVector; // (0,0,1)

    if (FMath::Abs(FVector::DotProduct(Up, AuxAxis)) > 0.95f)
    {
        AuxAxis = FVector::RightVector; // (1,0,0)
    }

    // 构造 Forward（保证与 Up 垂直）
    FVector Forward = FVector::CrossProduct(AuxAxis, Up).GetSafeNormal();

    // 再构造 Right
    FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();

    // 用三轴生成旋转矩阵
    FRotator Rot = FRotationMatrix::MakeFromXY(Forward, Right).Rotator();

    SetActorRotation(Rot);
}

