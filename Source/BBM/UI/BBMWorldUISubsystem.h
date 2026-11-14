/*****************************************************************//**
 * @file   BBMWorldUISubsystem.h
 * @brief  BBM(비트 배낭 상인) 월드 레벨 전역 UI(위젯 및 ViewModel) 관리 서브시스템 클래스 정의
 * @details 특정 폰/컨트롤러에 귀속되지 않는 월드 소속 위젯 및 MVVM 뷰모델의 등록, 조회, 제거 및 가비지 컬렉션(GC) 보호를 총괄함
 * @date   2026-11-12
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "BBMWorldUISubsystem.generated.h"

/* 독립 전방 선언 */
class UBBMLayerRootWidget;
class UBBMUserWidgetBase;
class UBBMViewModelBase;
class UUserWidget;
class UWidget;

/**
 * BBM 월드 UI 관리 서브시스템
 * 월드의 생성 및 소멸 주기와 일치하여 레벨 전역 위젯과 ViewModel 인스턴스를 독립적으로 보관 및 관리함
 */
UCLASS(meta = (DisplayName = "BBM World UI Subsystem"))
class BBM_API UBBMWorldUISubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	/* 생성자 및 수명 주기 */
public:
	UBBMWorldUISubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* 위젯 및 ViewModel 등록/조회/제거 인터페이스 */
public:
	/**
	 * 순수 위젯 설정(FBBMWorldWidgetConfig)을 기반으로 위젯 인스턴스를 생성하고 지정 키로 등록함
	 * @param WidgetKey 위젯을 등록할 식별 키
	 * @param Config 생성할 위젯 클래스 및 ZOrder 오버라이드 설정
	 * @return 생성 및 등록된 위젯 인스턴스 포인터 (실패 시 nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	UBBMUserWidgetBase* CreateAndRegisterWidgetByConfig(FName WidgetKey, const FBBMWorldWidgetConfig& Config);

	/**
	 * 순수 ViewModel 설정(FBBMWorldViewModelConfig)을 기반으로 ViewModel 인스턴스를 생성하고 지정 키로 등록함
	 * @param WidgetKey ViewModel을 등록할 식별 키
	 * @param Config 생성할 MVVM ViewModel 클래스 설정
	 * @return 생성 및 등록된 ViewModel 인스턴스 포인터 (실패 시 nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	UBBMViewModelBase* CreateAndRegisterViewModelByConfig(FName WidgetKey, const FBBMWorldViewModelConfig& Config);

	/**
	 * 식별 키로 등록된 위젯 인스턴스를 조회함
	 * @param WidgetKey 조회할 식별 키
	 * @return 등록된 위젯 포인터 (미등록 시 nullptr)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI")
	UBBMUserWidgetBase* GetWidget(FName WidgetKey) const;

	/**
	 * 특정 타입으로 캐스팅하여 등록된 위젯을 조회하는 C++ 템플릿 헬퍼
	 * @param WidgetKey 조회할 식별 키
	 * @return 캐스팅된 위젯 포인터
	 */
	template<typename T>
	T* GetWidgetAs(FName WidgetKey) const
	{
		return Cast<T>(GetWidget(WidgetKey));
	}

	/**
	 * 식별 키로 등록된 MVVM 뷰모델 인스턴스를 조회함
	 * @param WidgetKey 조회할 식별 키
	 * @return 등록된 뷰모델 포인터 (미등록 시 nullptr)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|MVVM")
	UBBMViewModelBase* GetViewModel(FName WidgetKey) const;

	/**
	 * 특정 타입으로 캐스팅하여 등록된 뷰모델을 조회하는 C++ 템플릿 헬퍼
	 * @param WidgetKey 조회할 식별 키
	 * @return 캐스팅된 뷰모델 포인터
	 */
	template<typename T>
	T* GetViewModelAs(FName WidgetKey) const
	{
		return Cast<T>(GetViewModel(WidgetKey));
	}

	/**
	 * 특정 키로 등록된 위젯만 뷰포트에서 분리하고 서브시스템에서 등록 해제함
	 * @param WidgetKey 제거할 위젯 식별 키
	 * @return 제거 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	bool RemoveWidget(FName WidgetKey);

	/**
	 * 특정 키로 등록된 ViewModel만 닫고 서브시스템에서 등록 해제함
	 * @param WidgetKey 제거할 ViewModel 식별 키
	 * @return 제거 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	bool RemoveViewModel(FName WidgetKey);

	/**
	 * 특정 키에 해당하는 위젯과 ViewModel 쌍을 모두 제거하고 등록 해제함
	 * @param WidgetKey 제거할 UI 식별 키
	 * @return 하나 이상 제거되었으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	bool RemoveUI(FName WidgetKey);

	/**
	 * 현재 서브시스템에 등록된 모든 위젯을 뷰포트에서 분리하고 비움
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	void ClearAllWidgets();

	/**
	 * 현재 서브시스템에 등록된 모든 ViewModel을 닫고 비움
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	void ClearAllViewModels();

	/**
	 * 현재 등록된 전체 위젯 개수 반환
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI")
	int32 GetRegisteredWidgetCount() const;

	/**
	 * 현재 등록된 전체 ViewModel 개수 반환
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|MVVM")
	int32 GetRegisteredViewModelCount() const;

	/* 레이어 루트 위젯 위임 헬퍼 인터페이스 */
public:
	/**
	 * 대상 위젯을 루트 위젯 내 지정된 ZOrder 레이어 캔버스 자식으로 추가함
	 * (이미 뷰포트 또는 레이어에 올라가 있으면 중복 추가하지 않고 false 반환)
	 * @param InWidget 추가할 위젯 인스턴스
	 * @param InLayer 배치할 대상 ZOrder 레이어
	 * @return 레이어 추가 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Layer")
	bool AddWidgetToLayer(UUserWidget* InWidget, EBBMWidgetZOrder InLayer);

	/**
	 * 이미 레이어에 올라가 있는 위젯의 슬롯 ZOrder를 해당 레이어 최상단으로 승격함
	 * @param InWidget 최상단으로 이동할 위젯 인스턴스
	 * @return 최상단 이동 성공 여부 (레이어 미소속 시 false)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Layer")
	bool BringWidgetToLayerTop(UUserWidget* InWidget);

	/**
	 * 지정 위젯이 루트 레이어 캔버스 중 하나의 직계 자식인지 확인함
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|Layer")
	bool IsWidgetInAnyLayer(const UWidget* InWidget) const;

#if WITH_EDITOR
	/**
	 * 에디터 자동화 테스트 검증을 위해 루트 레이어 위젯의 ZOrder 정규화 임계치를 임시 조정함
	 */
	void SetLayerZOrderMaxThresholdForTesting(int32 InThreshold);
#endif

	/* 내부 정리 및 루트 위젯 보장 헬퍼 */
private:
	/* 루트 레이어 위젯 생성 및 뷰포트 등록을 보장 초기화함 */
	bool EnsureLayerRootInitialized();

	/* 위젯 인스턴스의 프레젠테이션과 ViewModel 바인딩을 즉시 해제함 */
	void ReleaseWidgetInstance(UBBMUserWidgetBase* InWidget);

	/* ViewModel 인스턴스에 연결된 위젯 참조를 해제하고 논리 상태를 닫음 */
	void ReleaseViewModelInstance(UBBMViewModelBase* InViewModel);

	/* 내부 프로퍼티 */
protected:
	/* 식별 키별 등록된 활성 월드 위젯 맵 (UPROPERTY 보관으로 GC 방지) */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UBBMUserWidgetBase>> ActiveWidgets;

	/* 식별 키별 등록된 활성 MVVM 뷰모델 맵 (UPROPERTY 보관으로 GC 방지) */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UBBMViewModelBase>> ActiveViewModels;

private:
	/* 뷰포트에 단독 등록되는 UI 레이어 루트 위젯 */
	UPROPERTY(Transient)
	TObjectPtr<UBBMLayerRootWidget> LayerRootWidget;
};

