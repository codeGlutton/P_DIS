/*****************************************************************//**
 * @file   BBMWorldWidgetSubsystem.h
 * @brief  BBM(비트 배낭 상인) 월드 레벨 전역 위젯 관리 서브시스템 클래스 정의
 * @details 특정 폰/컨트롤러에 귀속되지 않는 월드 소속 UI(HUD, FadeLoading 등)의 라이프사이클 및 가비지 컬렉션(GC) 보호를 총괄함
 * @date   2026-11-06
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "BBMWorldWidgetSubsystem.generated.h"

/* 독립 전방 선언 */
class UBBMUserWidgetBase;

/**
 * BBM 월드 위젯 관리 서브시스템
 * 월드의 생성 및 소멸 주기와 일치하여 레벨 전역 UI를 안전하게 보관하고 관리함
 */
UCLASS(meta = (DisplayName = "BBM World Widget Subsystem"))
class BBM_API UBBMWorldWidgetSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	/* 생성자 및 수명 주기 */
public:
	UBBMWorldWidgetSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* 위젯 등록, 조회 및 제거 인터페이스 */
public:
	/**
	 * 위젯 클래스로부터 인스턴스를 생성하고 식별 키로 서브시스템에 등록합니다
	 * @param WidgetClass 생성할 위젯 클래스 (UBBMUserWidgetBase 파생)
	 * @param WidgetKey 등록에 사용할 고유 식별 키
	 * @param ZOrderLayer 위젯의 ZOrder 레이어 (미지정 시 클래스 자체의 기본값 유지)
	 * @return 생성 및 등록된 위젯 인스턴스 포인터 (실패 시 nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	UBBMUserWidgetBase* CreateAndRegisterWidget(
		TSubclassOf<UBBMUserWidgetBase> WidgetClass,
		FName WidgetKey,
		EBBMWidgetZOrder ZOrderLayer = EBBMWidgetZOrder::UseClassDefault
	);

	/**
	 * 식별 키로 등록된 위젯 인스턴스를 조회합니다
	 * @param WidgetKey 조회할 위젯 식별 키
	 * @return 등록된 위젯 포인터 (미등록 시 nullptr)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI")
	UBBMUserWidgetBase* GetWidget(FName WidgetKey) const;

	/**
	 * 특정 타입으로 캐스팅하여 등록된 위젯을 조회하는 C++ 템플릿 헬퍼
	 * @param WidgetKey 조회할 위젯 식별 키
	 * @return 캐스팅된 위젯 포인터
	 */
	template<typename T>
	T* GetWidgetAs(FName WidgetKey) const
	{
		return Cast<T>(GetWidget(WidgetKey));
	}

	/**
	 * 특정 키로 등록된 위젯을 닫고 서브시스템에서 등록 해제합니다
	 * @param WidgetKey 제거할 위젯 식별 키
	 * @return 제거 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	bool RemoveWidget(FName WidgetKey);

	/**
	 * 현재 서브시스템에 등록된 모든 위젯을 닫고 비웁니다
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	void ClearAllWidgets();

	/**
	 * 현재 등록된 전체 위젯 개수 반환
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI")
	int32 GetRegisteredWidgetCount() const;

	/* 내부 프로퍼티 */
protected:
	/* 식별 키별 등록된 활성 월드 위젯 맵 (UPROPERTY 보관으로 GC 방지) */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UBBMUserWidgetBase>> ActiveWidgets;
};

