/*****************************************************************//**
 * @file   DISGlobalSubsystem.h
 * @brief  DIS 플러그인 전역 런타임 호스팅 서브시스템
 * @date   2026-09-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Global/DISGlobalLogic.h"
#include "DISGlobalSubsystem.generated.h"

/**
 * DIS 플러그인의 전역 런타임 수명주기를 관리하는 게임 인스턴스 서브시스템
 * UGameInstance 수명주기 동안 UDISGlobalLogic 인스턴스를 단일 관리하며,
 * 어디서든 안전하게 전역 로직에 접근할 수 있는 멤버 및 정적 헬퍼를 제공함
 */
UCLASS(DisplayName = "DIS Global Subsystem")
class DISRUNTIME_API UDISGlobalSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UDISGlobalSubsystem();

	/* 서브시스템 라이프사이클 */
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* 전역 로직 접근자 (멤버 함수) */
public:
	/* 관리 중인 UDISGlobalLogic 인스턴스 반환 */
	UFUNCTION(Category = "DIS|Global", BlueprintCallable, BlueprintPure)
	UDISGlobalLogic* GetLogic() const;

	/* 템플릿 기반 파생 UDISGlobalLogic 인스턴스 반환 */
	template <typename T = UDISGlobalLogic>
	T* GetLogic() const
	{
		return Cast<T>(GetLogic());
	}

	/* 전역 로직 접근자 (정적 헬퍼 함수) */
public:
	/**
	 * 월드 컨텍스트 오브젝트를 기반으로 UDISGlobalLogic 인스턴스를 조회하는 정적 헬퍼
	 * @param WorldContextObject 월드 검색에 활용될 UObject
	 * @return 발견된 UDISGlobalLogic 인스턴스 (실패 시 nullptr)
	 */
	UFUNCTION(Category = "DIS|Global", BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject", DisplayName = "Get DIS Global Logic"))
	static UDISGlobalLogic* GetDISGlobalLogic(const UObject* WorldContextObject);

	/**
	 * 월드 컨텍스트 기반 C++ 정적 헬퍼 오버로드
	 */
	static UDISGlobalLogic* GetLogic(const UObject* WorldContextObject)
	{
		return GetDISGlobalLogic(WorldContextObject);
	}

	/**
	 * 월드 컨텍스트 기반 템플릿 정적 헬퍼
	 * @param WorldContextObject 월드 검색에 활용될 UObject
	 * @return 캐스팅된 파생 UDISGlobalLogic 인스턴스 (실패 시 nullptr)
	 */
	template <typename T = UDISGlobalLogic>
	static T* GetLogic(const UObject* WorldContextObject)
	{
		return Cast<T>(GetDISGlobalLogic(WorldContextObject));
	}

	/* 관리 중인 로직 서브오브젝트 인스턴스 */
private:
	UPROPERTY(Transient)
	TObjectPtr<UDISGlobalLogic> LogicInstance;
};
