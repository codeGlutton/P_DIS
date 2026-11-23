/*****************************************************************//**
 * @file   BBMEventNpcBase.cpp
 * @brief  BBM(비트 배낭 상인) 용사 만남 방(Event Encounter) 등장 NPC 베이스 폰 클래스 구현
 * @date   2026-11-04
 *********************************************************************/

#include "Stage/BBMEventNpcBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#if WITH_EDITORONLY_DATA
#include "Components/ArrowComponent.h"
#endif

ABBMEventNpcBase::ABBMEventNpcBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 루트 캡슐 컴포넌트 생성 및 기본 크기 설정
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(34.0f, 88.0f);
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->bDynamicObstacle = true;
	RootComponent = CapsuleComponent;

	// 2. 외형 스켈레탈 메시 컴포넌트 생성 및 캡슐 하단 정렬
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
	Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Mesh->SetGenerateOverlapEvents(false);

#if WITH_EDITORONLY_DATA
	// 3. 에디터 전용 방향 화살표 컴포넌트 생성
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetupAttachment(RootComponent);
	ArrowComponent->ArrowColor = FColor(150, 200, 255);
	ArrowComponent->ArrowSize = 1.0f;
	ArrowComponent->bIsScreenSizeScaled = true;
	ArrowComponent->SetVisibility(true);
	ArrowComponent->bHiddenInGame = true;
#endif
}

UCapsuleComponent* ABBMEventNpcBase::GetCapsuleComponent() const
{
	return CapsuleComponent;
}

USkeletalMeshComponent* ABBMEventNpcBase::GetMesh() const
{
	return Mesh;
}

