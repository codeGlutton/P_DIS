/*****************************************************************//**
 * @file   BBMEventNpcBase.h
 * @brief  BBM(비트 배낭 상인) 용사 만남 방(Event Encounter) 등장 NPC 베이스 폰 클래스 정의
 * @details 가벼운 연출 및 상호작용을 위해 캡슐 충돌체, 스켈레탈 메시 및 에디터 화살표만 포함하는 경량 APawn 베이스
 * @date   2026-11-04
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BBMEventNpcBase.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UCapsuleComponent;
class USkeletalMeshComponent;
class UArrowComponent;

/**
 * BBM 용사 만남 방(Event Encounter) 등장 NPC 베이스 폰 클래스
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "BBM Event NPC Base"))
class BBM_API ABBMEventNpcBase : public APawn
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMEventNpcBase();

	/* 컴포넌트 접근자 */
public:
	/* 캡슐 충돌 컴포넌트 반환 */
	UFUNCTION(Category = "BBM|NPC", BlueprintCallable, BlueprintPure)
	UCapsuleComponent* GetCapsuleComponent() const;

	/* 외형 스켈레탈 메시 컴포넌트 반환 */
	UFUNCTION(Category = "BBM|NPC", BlueprintCallable, BlueprintPure)
	USkeletalMeshComponent* GetMesh() const;

	/* 컴포넌트 프로퍼티 */
protected:
	/* 루트 캡슐 충돌 컴포넌트 */
	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	/* 외형 스켈레탈 메시 컴포넌트 */
	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;

#if WITH_EDITORONLY_DATA
	/* 에디터 전방 방향 지시 화살표 컴포넌트 */
	UPROPERTY(Category = "Components", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif
};

