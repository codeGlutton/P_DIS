/*****************************************************************//**
 * @file   BBM1DTileMapActor.h
 * @brief  BBM(비트 배낭 상인) 1D 타일 맵 액터 정의
 * @details 대치 중앙점을 기준으로 아군 3칸, 적군 3칸의 직육면체 타일 인스턴스를 소유하고 전투원 배치 트랜스폼을 제공하는 액터
 * @date   2026-11-07
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/BBMCombatTypes.h"
#include "BBM1DTileMapActor.generated.h"

/* 독립 전방 선언 */
class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;

/**
 * 1D 라인 전투용 6칸(아군 3칸, 적군 3칸) 타일 맵 액터
 * 직육면체 타일 인스턴스 렌더링 및 각 슬롯별 배치 트랜스폼 관리
 */
UCLASS(Blueprintable, meta = (DisplayName = "BBM 1D Tile Map Actor"))
class BBM_API ABBM1DTileMapActor : public AActor
{
	GENERATED_BODY()

public:
	ABBM1DTileMapActor();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/* 타일 및 슬롯 인터페이스 */
public:
	/**
	 * 타일 인스턴스 및 슬롯 트랜스폼을 재구축
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|Combat|TileMap")
	void RebuildTiles();

	/**
	 * 특정 레인 및 슬롯에 해당하는 전투원 배치 트랜스폼 반환 (타일 상단 표면 위치 및 진영 방향 회전)
	 * @param Lane 아군(Ally) 또는 적군(Enemy) 레인
	 * @param SlotIndex 슬롯 인덱스 (0: 최전방, 1: 중열, 2: 후열)
	 * @return 전투원 스폰/배치용 트랜스폼
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|Combat|TileMap")
	FTransform GetSlotTransform(EBBMCombatLane Lane, int32 SlotIndex) const;

	/**
	 * 특정 레인 및 슬롯에 해당하는 바닥 타일 인스턴스 메시 트랜스폼 반환
	 * @param Lane 아군 또는 적군 레인
	 * @param SlotIndex 슬롯 인덱스
	 * @return 바닥 타일 메시의 월드 트랜스폼
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|Combat|TileMap")
	FTransform GetTileTransform(EBBMCombatLane Lane, int32 SlotIndex) const;

	/**
	 * 지정된 레인의 최대 슬롯 수 반환
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|Combat|TileMap")
	int32 GetSlotCount(EBBMCombatLane Lane) const { return SlotsPerLane; }

	/* 타일 ISM 컴포넌트 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|Combat|TileMap")
	UInstancedStaticMeshComponent* GetTileISMComponent() const { return TileISMComponent; }

	/* 타일 설정 프로퍼티 */
public:
	/* 직육면체 타일의 크기 (가로 X, 세로 Y, 두께 Z, 센티미터 단위) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BBM|Combat|TileMap")
	FVector TileSize;

	/* 중앙선(대치 중심점)에서 0번 최전방 슬롯 타일 중심까지의 거리 (기본값: 180.0f) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BBM|Combat|TileMap")
	float CenterOffset;

	/* 슬롯 중심 간 간격 (기본값: 120.0f) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BBM|Combat|TileMap")
	float SlotSpacing;

	/* 레인당 슬롯 수 (아군 3칸, 적군 3칸의 기준값: 3) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BBM|Combat|TileMap", meta = (ClampMin = "1", ClampMax = "8"))
	int32 SlotsPerLane;

	/* 타일 렌더링에 사용할 스태틱 메시 (미지정 시 엔진 기본 Cube 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BBM|Combat|TileMap")
	TObjectPtr<UStaticMesh> TileMesh;

	/* 컴포넌트 */
protected:
	/* 루트 씬 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BBM|Combat|TileMap")
	TObjectPtr<USceneComponent> SceneRoot;

	/* 직육면체 타일 인스턴스 렌더링 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BBM|Combat|TileMap")
	TObjectPtr<UInstancedStaticMeshComponent> TileISMComponent;

	/* 내부 캐시된 트랜스폼 */
private:
	UPROPERTY(Transient)
	TArray<FTransform> CachedAllyTileTransforms;

	UPROPERTY(Transient)
	TArray<FTransform> CachedEnemyTileTransforms;

	UPROPERTY(Transient)
	TArray<FTransform> CachedAllySlotTransforms;

	UPROPERTY(Transient)
	TArray<FTransform> CachedEnemySlotTransforms;
};

