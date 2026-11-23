/*****************************************************************//**
 * @file   BBMChestActorBase.cpp
 * @brief  BBM(비트 배낭 상인) 보물 상자 액터 베이스 클래스 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Stage/BBMChestActorBase.h"
#include "Components/StaticMeshComponent.h"

ABBMChestActorBase::ABBMChestActorBase()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
}

void ABBMChestActorBase::BeginPlay()
{
	Super::BeginPlay();
}

UStaticMeshComponent* ABBMChestActorBase::GetMeshComponent() const
{
	return MeshComponent;
}

