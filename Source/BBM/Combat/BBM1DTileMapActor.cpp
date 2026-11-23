/*****************************************************************//**
 * @file   BBM1DTileMapActor.cpp
 * @brief  BBM(비트 배낭 상인) 1D 타일 맵 액터 구현
 * @date   2026-11-07
 *********************************************************************/

#include "Combat/BBM1DTileMapActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABBM1DTileMapActor::ABBM1DTileMapActor()
	: TileSize(FVector(100.0f, 100.0f, 10.0f))
	, CenterOffset(180.0f)
	, SlotSpacing(120.0f)
	, SlotsPerLane(3)
	, TileMesh(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	TileISMComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TileISMComponent"));
	TileISMComponent->SetupAttachment(RootComponent);
	TileISMComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TileISMComponent->SetCollisionResponseToAllChannels(ECR_Block);

	// 기본 큐브 스태틱 메시 로드
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultCubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultCubeFinder.Succeeded())
	{
		TileMesh = DefaultCubeFinder.Object;
		TileISMComponent->SetStaticMesh(TileMesh);
	}
}

void ABBM1DTileMapActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildTiles();
}

void ABBM1DTileMapActor::BeginPlay()
{
	Super::BeginPlay();
	RebuildTiles();
}

void ABBM1DTileMapActor::RebuildTiles()
{
	if (TileISMComponent == nullptr)
	{
		return;
	}

	// 스태틱 메시 동기화
	if (TileMesh != nullptr && TileISMComponent->GetStaticMesh() != TileMesh)
	{
		TileISMComponent->SetStaticMesh(TileMesh);
	}

	TileISMComponent->ClearInstances();
	CachedAllyTileTransforms.Reset();
	CachedEnemyTileTransforms.Reset();
	CachedAllySlotTransforms.Reset();
	CachedEnemySlotTransforms.Reset();

	const FVector LineForward = GetActorForwardVector();
	const FVector CenterLocation = GetActorLocation();
	const FRotator CenterRotation = GetActorRotation();

	const FRotator FacingEnemyRotation = CenterRotation;
	const FRotator FacingAllyRotation = (CenterRotation + FRotator(0.0f, 180.0f, 0.0f)).GetNormalized();

	// 언리얼 기본 Cube(100x100x100) 기준 스케일 계산
	const FVector TileScale = FVector(
		FMath::Max(0.01f, TileSize.X / 100.0f),
		FMath::Max(0.01f, TileSize.Y / 100.0f),
		FMath::Max(0.01f, TileSize.Z / 100.0f)
	);

	// 1. 아군 레인 타일 및 슬롯 트랜스폼 구축 (중심에서 -LineForward 방향)
	for (int32 SlotIdx = 0; SlotIdx < SlotsPerLane; ++SlotIdx)
	{
		const float Dist = CenterOffset + (SlotIdx * SlotSpacing);
		const FVector TileLocation = CenterLocation - (LineForward * Dist);
		const FTransform TileWorldTransform(CenterRotation, TileLocation, TileScale);

		TileISMComponent->AddInstance(TileWorldTransform, true);
		CachedAllyTileTransforms.Add(TileWorldTransform);

		// 전투원은 타일 윗면 중앙에 위치하며 적 진영을 바라봄
		const FVector SlotLocation = TileLocation + FVector(0.0f, 0.0f, TileSize.Z * 0.5f);
		CachedAllySlotTransforms.Add(FTransform(FacingEnemyRotation, SlotLocation));
	}

	// 2. 적군 레인 타일 및 슬롯 트랜스폼 구축 (중심에서 +LineForward 방향)
	for (int32 SlotIdx = 0; SlotIdx < SlotsPerLane; ++SlotIdx)
	{
		const float Dist = CenterOffset + (SlotIdx * SlotSpacing);
		const FVector TileLocation = CenterLocation + (LineForward * Dist);
		const FTransform TileWorldTransform(CenterRotation, TileLocation, TileScale);

		TileISMComponent->AddInstance(TileWorldTransform, true);
		CachedEnemyTileTransforms.Add(TileWorldTransform);

		// 적군은 타일 윗면 중앙에 위치하며 아군(플레이어) 진영을 바라봄
		const FVector SlotLocation = TileLocation + FVector(0.0f, 0.0f, TileSize.Z * 0.5f);
		CachedEnemySlotTransforms.Add(FTransform(FacingAllyRotation, SlotLocation));
	}
}

FTransform ABBM1DTileMapActor::GetSlotTransform(EBBMCombatLane Lane, int32 SlotIndex) const
{
	const TArray<FTransform>& TargetArray = (Lane == EBBMCombatLane::AllyLane)
		? CachedAllySlotTransforms
		: CachedEnemySlotTransforms;

	if (TargetArray.Num() == 0)
	{
		// 캐시가 비어있을 경우 안전 재구축
		const_cast<ABBM1DTileMapActor*>(this)->RebuildTiles();
	}

	const TArray<FTransform>& RecheckedArray = (Lane == EBBMCombatLane::AllyLane)
		? CachedAllySlotTransforms
		: CachedEnemySlotTransforms;

	if (RecheckedArray.IsValidIndex(SlotIndex))
	{
		return RecheckedArray[SlotIndex];
	}

	if (RecheckedArray.Num() > 0)
	{
		const int32 ClampedIndex = FMath::Clamp(SlotIndex, 0, RecheckedArray.Num() - 1);
		return RecheckedArray[ClampedIndex];
	}

	return GetActorTransform();
}

FTransform ABBM1DTileMapActor::GetTileTransform(EBBMCombatLane Lane, int32 SlotIndex) const
{
	const TArray<FTransform>& TargetArray = (Lane == EBBMCombatLane::AllyLane)
		? CachedAllyTileTransforms
		: CachedEnemyTileTransforms;

	if (TargetArray.Num() == 0)
	{
		const_cast<ABBM1DTileMapActor*>(this)->RebuildTiles();
	}

	const TArray<FTransform>& RecheckedArray = (Lane == EBBMCombatLane::AllyLane)
		? CachedAllyTileTransforms
		: CachedEnemyTileTransforms;

	if (RecheckedArray.IsValidIndex(SlotIndex))
	{
		return RecheckedArray[SlotIndex];
	}

	if (RecheckedArray.Num() > 0)
	{
		const int32 ClampedIndex = FMath::Clamp(SlotIndex, 0, RecheckedArray.Num() - 1);
		return RecheckedArray[ClampedIndex];
	}

	return GetActorTransform();
}

