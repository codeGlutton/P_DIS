/*****************************************************************//**
 * @file   BBMCameraActorBase.cpp
 * @brief  BBM(비트 배낭 상인) 메인 뷰 카메라 액터 베이스 클래스 구현
 * @date   2026-11-04
 *********************************************************************/

#include "Level/BBMCameraActorBase.h"
#include "Camera/CameraComponent.h"

ABBMCameraActorBase::ABBMCameraActorBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 스프링암 없이 UCameraComponent를 루트 컴포넌트로 생성
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	SetRootComponent(CameraComponent);
}

UCameraComponent* ABBMCameraActorBase::GetCameraComponent() const
{
	return CameraComponent;
}

