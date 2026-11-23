/*****************************************************************//**
 * @file   BBMChestActorBase.h
 * @brief  BBM(비트 배낭 상인) 보물 상자 액터 베이스 클래스 정의
 * @details 보물 만남(Treasure Encounter)에서 룸 스폰 포인트에 생성될 보물 상자 액터의 공통 기반
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBMChestActorBase.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UStaticMeshComponent;

/**
 * BBM 보물 상자 베이스 액터 클래스
 * 보물 만남 룸 진입 시 스폰되어 플레이어와의 상호작용 및 전리품(아이템/골드) 지급을 처리함
 */
UCLASS(Blueprintable, meta = (DisplayName = "BBM Chest Actor Base"))
class BBM_API ABBMChestActorBase : public AActor
{
	GENERATED_BODY()

	/* 생성자 및 라이프사이클 */
public:
	ABBMChestActorBase();

	virtual void BeginPlay() override;

	/* 컴포넌트 접근자 */
public:
	/* 스태틱 메시 컴포넌트 반환 */
	UFUNCTION(Category = "BBM|Chest", BlueprintCallable, BlueprintPure)
	UStaticMeshComponent* GetMeshComponent() const;

	/* 컴포넌트 프로퍼티 */
protected:
	/* 보물 상자 외형 스태틱 메시 컴포넌트 (루트) */
	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};

