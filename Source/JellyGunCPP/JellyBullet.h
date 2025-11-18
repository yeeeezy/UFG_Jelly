#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JellyBullet.generated.h"

UCLASS()
class JELLYGUNCPP_API AJellyBullet : public AActor
{
	GENERATED_BODY()
    
public:
	AJellyBullet();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Mesh;

	// ========= 变形参数 =========
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SquashAmount = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StretchAmount = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DeformDuration = 0.25f;

	FVector OriginalScale;
	FVector DeformedScale;

	bool bIsDeforming = false;
	float DeformTimer = 0.f;

	// ========= 弹跳 =========
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BounceImpulseStrength = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BounceDamping = 0.75f;    // 每次减少25%

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxBounceCount = 2;

	int32 CurrentBounceCount = 0;

	bool bHasStopped = false;

private:
	FVector ImpactNormalWorld;

	UFUNCTION()
	void OnMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
				   UPrimitiveComponent* OtherComp, FVector NormalImpulse,
				   const FHitResult& Hit);

	void ApplyDeformAnimation(float Alpha);

	void LaunchAwayFromSurface();

	void RotateToSurfaceNormal();
};
