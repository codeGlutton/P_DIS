/*****************************************************************//**
 * @file   DISEnhanceableInterface.h
 * @brief  DIS 강화 시스템이 적용 가능한 대상 객체 인터페이스 정의
 * @date   2026-09-29
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DISEnhanceableInterface.generated.h"

class UDISEnhancementContainer;
class UDISEnhancement;

UINTERFACE(MinimalAPI, BlueprintType)
class UDISEnhanceableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 강화를 소유/적용 가능한 객체가 구현하는 인터페이스
 * 컴포넌트 접근 패턴(예: IAbilitySystemInterface)과 유사하게 강화 전담 컨테이너 객체의 포인터를 반환함
 */
class DISRUNTIME_API IDISEnhanceableInterface
{
	GENERATED_BODY()

	/* 인터페이스 접근 메서드 */
public:
	/* 강화를 전담 관리하는 컨테이너 객체 포인터 반환 */
	virtual UDISEnhancementContainer* GetEnhancementContainer() const = 0;

	/* 특정 강화 스탯의 기본 최대 강화 레벨 반환 (기본 구현: UDISEnhancement CDO의 DefaultMaxLevel 반환) */
	virtual int32 GetBaseMaxEnhancementLevel(TSubclassOf<UDISEnhancement> InEnhancementClass) const;
};
