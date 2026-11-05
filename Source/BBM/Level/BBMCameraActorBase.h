/*****************************************************************//**
 * @file   BBMCameraActorBase.h
 * @brief  BBM(비트 배낭 상인) 메인 뷰 카메라 액터 베이스 클래스 정의
 * @details 스프링암 없이 UCameraComponent를 루트로 장착한 가벼운 뷰타깃 전용 액터
 * @date   2026-11-04
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBMCameraActorBase.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UCameraComponent;

/**
 * BBM 메인 뷰 카메라 액터 베이스 클래스
 * 레벨 내 룸/만남별 WorldSettings의 메인 카메라 포인트에 배치되거나 뷰타깃 전환에 활용됨
 */
UCLASS(Blueprintable, meta = (DisplayName = "BBM Camera Actor Base"))
class BBM_API ABBMCameraActorBase : public AActor
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMCameraActorBase();

	/* 컴포넌트 접근자 */
public:
	/* 카메라 컴포넌트 반환 */
	UFUNCTION(Category = "BBM|Camera", BlueprintCallable, BlueprintPure)
	UCameraComponent* GetCameraComponent() const;

	/* 컴포넌트 프로퍼티 */
protected:
	/* 루트 카메라 컴포넌트 */
	UPROPERTY(Category = "Camera", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;
};

