/*****************************************************************//**
 * @file   BBMSettingsTabEntryWidget.h
 * @brief  BBM(비트 배낭 상인) 환경설정 화면 전용 탭 엔트리 위젯 C++ 베이스 클래스 정의
 * @details MainButtonName으로 내부 UButton을 조회·캐싱하고, SetTabSelected(bool)를 통한 입력 제한(HitTestInvisible)·FButtonStyle 전체 치환 및 클릭 시 OnRequestSelect 발송을 담당함
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/BBMTabEntryWidget.h"
#include "Styling/SlateTypes.h"
#include "BBMSettingsTabEntryWidget.generated.h"

/* 독립 전방 선언 */
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBBMOnTabEntrySelectRequestedDynamicSignature);

/**
 * BBM 환경설정 탭 엔트리용 UMG C++ 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Settings Tab Entry Widget"))
class BBM_API UBBMSettingsTabEntryWidget : public UBBMTabEntryWidget
{
	GENERATED_BODY()

	/* 수명 주기 */
protected:
	virtual void NativeOnInitialized() override;
	virtual void BeginDestroy() override;

	/* 탭 선택 상태 제어 및 조회 */
public:
	/**
	 * 탭 선택 상태를 반영함 (버튼 누락 또는 동일 상태 유지 시 즉시 반환)
	 * @param bInIsSelected 선택됨 여부 (true: HitTestInvisible + SelectedButtonStyle 적용, false: Visible + 기본 스타일 복원)
	 */
	UFUNCTION(Category = "BBM|UI|Tab", BlueprintCallable)
	void SetTabSelected(bool bInIsSelected);

	/**
	 * 현재 내부 선택 상태 여부를 반환함
	 */
	UFUNCTION(Category = "BBM|UI|Tab", BlueprintPure)
	bool IsTabSelected() const { return bIsSelected; }

	/* 내부 버튼 클릭 및 비주얼 적용 헬퍼 */
private:
	UFUNCTION()
	void HandleMainButtonClicked();

	void ApplySelectionState();

	/* C++ 및 블루프린트 선택 요청 이벤트 */
public:
	/* 비선택 상태에서 메인 버튼 클릭 시 발송되는 C++ 선택 요청 대리자 */
	FSimpleMulticastDelegate OnRequestSelectNative;

	/* 비선택 상태에서 메인 버튼 클릭 시 발송되는 블루프린트 선택 요청 대리자 */
	UPROPERTY(Category = "BBM|UI|Tab|Event", BlueprintAssignable)
	FBBMOnTabEntrySelectRequestedDynamicSignature OnRequestSelect;

	/* 위젯 이름 및 선택 스타일 설정 */
protected:
	/* WBP 파생 위젯에서 필수로 지정할 메인 버튼 위젯 이름 */
	UPROPERTY(Category = "BBM|UI|Tab", EditAnywhere, BlueprintReadWrite)
	FName MainButtonName = NAME_None;

	/* 선택 상태(bIsSelected == true)일 때 메인 버튼에 전체 적용할 FButtonStyle */
	UPROPERTY(Category = "BBM|UI|Tab|Style", EditAnywhere, BlueprintReadWrite)
	FButtonStyle SelectedButtonStyle;

	/* 내부 캐시 및 상태 */
private:
	/* MainButtonName으로 조회·캐싱된 메인 버튼 포인터 */
	UPROPERTY(Transient)
	TObjectPtr<UButton> CachedMainButton = nullptr;

	/* NativeOnInitialized 시점에 백업해 둔 메인 버튼의 초기 FButtonStyle */
	FButtonStyle CachedDefaultButtonStyle;

	/* 현재 탭 엔트리의 내부 선택 상태 */
	bool bIsSelected = false;
};