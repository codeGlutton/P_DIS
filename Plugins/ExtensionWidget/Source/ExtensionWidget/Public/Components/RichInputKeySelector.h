/*****************************************************************//**
 * @file   RichInputKeySelector.h
 * @brief  UInputKeySelector 완전 복제 및 FieldNotify 대입-알림 순서 수정 위젯 정의
 * @details UWidget을 직접 상속해 SInputKeySelector를 래핑하며, SetSelectedKey 시 SelectedKey 선 대입 및
 *          Slate 동기화 중 OnKeySelected 에코 차단을 거쳐 마지막에 FieldNotify를 발송해 TwoWay 순환 오류를 방지함.
 * @date   2026-11-19
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Commands/InputChord.h"
#include "InputCoreTypes.h"
#include "Layout/Margin.h"
#include "Layout/Visibility.h"
#include "Styling/SlateTypes.h"
#include "UObject/ObjectMacros.h"
#include "Widgets/SWidget.h"
#include "Components/Widget.h"
#include "RichInputKeySelector.generated.h"

/* 독립 전방 선언 */
class SInputKeySelector;

/**
 * FieldNotify 선 대입·후 알림 규약을 준수해 안전한 TwoWay 바인딩을 지원하는 키 선택 위젯
 */
UCLASS(meta = (DisplayName = "Rich Input Key Selector"))
class EXTENSIONWIDGET_API URichInputKeySelector : public UWidget
{
	GENERATED_BODY()

	/* 델리게이트 타입 */
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKeySelected, FInputChord, SelectedKey);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnIsSelectingKeyChanged);

	/* 생성자 */
public:
	URichInputKeySelector(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/* 선택 키 및 동작 설정 API */
public:
	/** 현재 선택된 입력 키 조합을 반환함 */
	FInputChord GetSelectedKey() const;

	/** 현재 선택된 입력 키 조합을 선 대입 후 Slate 반영 및 FieldNotify 알림으로 갱신함 */
	UFUNCTION(Category = "Widget", BlueprintCallable)
	void SetSelectedKey(const FInputChord& InSelectedKey);

	/** 키 선택 대기 중 표시할 텍스트를 설정함 */
	UFUNCTION(Category = "Widget", BlueprintCallable)
	void SetKeySelectionText(FText InKeySelectionText);

	/** 키 선택 대기 중 표시할 텍스트를 반환함 */
	const FText& GetKeySelectionText() const;

	/** 할당된 키가 없을 때 표시할 텍스트를 설정함 */
	UFUNCTION(Category = "Widget", BlueprintCallable)
	void SetNoKeySpecifiedText(FText InNoKeySpecifiedText);

	/** 할당된 키가 없을 때 표시할 텍스트를 반환함 */
	const FText& GetNoKeySpecifiedText() const;

	/** 모디파이어 키(Shift·Ctrl·Alt 등) 조합 허용 여부를 설정함 */
	UFUNCTION(Category = "Widget", BlueprintCallable)
	void SetAllowModifierKeys(bool bInAllowModifierKeys);

	/** 모디파이어 키 조합 허용 여부를 반환함 */
	bool AllowModifierKeys() const;

	/** 게임패드 키 입력 허용 여부를 설정함 */
	UFUNCTION(Category = "Widget", BlueprintCallable)
	void SetAllowGamepadKeys(bool bInAllowGamepadKeys);

	/** 게임패드 키 입력 허용 여부를 반환함 */
	bool AllowGamepadKeys() const;

	/** 현재 키 입력 대기 상태인지 반환함 */
	UFUNCTION(Category = "Widget", BlueprintCallable)
	bool GetIsSelectingKey() const;

	/** 내부 텍스트 블록의 가시성을 설정함 */
	UFUNCTION(Category = "Widget", BlueprintCallable)
	void SetTextBlockVisibility(const ESlateVisibility InVisibility);

	/** 키 선택 버튼 스타일을 설정함 */
	void SetButtonStyle(const FButtonStyle& InButtonStyle);

	/** 키 선택 버튼 스타일을 반환함 */
	const FButtonStyle& GetButtonStyle() const;

	/** 내부 텍스트 스타일을 설정함 */
	void SetTextStyle(const FTextBlockStyle& InTextStyle);

	/** 내부 텍스트 스타일을 반환함 */
	const FTextBlockStyle& GetTextStyle() const;

	/** 내부 텍스트 여백을 설정함 */
	void SetMargin(const FMargin& InMargin);

	/** 내부 텍스트 여백을 반환함 */
	const FMargin& GetMargin() const;

	/** 키 선택 취소용 Escape 키 목록을 설정함 */
	UFUNCTION(Category = "Widget", BlueprintCallable)
	void SetEscapeKeys(const TArray<FKey>& InKeys);

	/* UObject / UWidget / UVisual 인터페이스 */
public:
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
	virtual void SynchronizeProperties() override;

protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	/* 내부 Slate 콜백 */
private:
	/** Slate 키 선택 완료 콜백 (프로그램적 SetSelectedKey 중에는 에코 억제) */
	virtual void HandleKeySelected(const FInputChord& InSelectedKey);

	/** Slate 키 입력 대기 상태 전환 콜백 */
	void HandleIsSelectingKeyChanged();

	/* 이벤트 델리게이트 */
public:
	/** 사용자가 새로운 키를 선택했을 때 호출됨 */
	UPROPERTY(Category = "Widget Event", BlueprintAssignable)
	FOnKeySelected OnKeySelected;

	/** 키 선택 대기 상태가 시작되거나 종료될 때 호출됨 */
	UPROPERTY(Category = "Widget Event", BlueprintAssignable)
	FOnIsSelectingKeyChanged OnIsSelectingKeyChanged;

	/* 위젯 프로퍼티 */
protected:
	/** 런타임 키 선택 버튼 스타일 */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite, Getter = "GetButtonStyle", Setter = "SetButtonStyle", meta = (DisplayName = "Style"))
	FButtonStyle WidgetStyle;

	/** 키 선택 버튼 내부 텍스트 스타일 */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite, Getter, Setter, meta = (DisplayName = "Text Style"))
	FTextBlockStyle TextStyle;

	/** 현재 선택된 입력 키 조합 (FieldNotify TwoWay 바인딩 대상) */
	UPROPERTY(Category = "Key Selection", BlueprintReadWrite, FieldNotify, Getter, Setter, BlueprintSetter = "SetSelectedKey")
	FInputChord SelectedKey;

	/** 선택 키 텍스트 주변 여백 */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite, Getter, Setter)
	FMargin Margin;

	/** 키 선택 대기 중 표시 텍스트 */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetKeySelectionText")
	FText KeySelectionText;

	/** 선택된 키가 없을 때 표시 텍스트 */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetNoKeySpecifiedText")
	FText NoKeySpecifiedText;

	/** 모디파이어 키(Shift·Ctrl·Alt 등) 조합 허용 여부 */
	UPROPERTY(Category = "Key Selection", EditAnywhere, BlueprintReadWrite, Getter = "AllowModifierKeys", Setter = "SetAllowModifierKeys", BlueprintSetter = "SetAllowModifierKeys")
	bool bAllowModifierKeys = true;

	/** 게임패드 키 입력 허용 여부 */
	UPROPERTY(Category = "Key Selection", EditAnywhere, BlueprintReadWrite, Getter = "AllowGamepadKeys", Setter = "SetAllowGamepadKeys", BlueprintSetter = "SetAllowGamepadKeys")
	bool bAllowGamepadKeys = false;

	/** 키 선택을 취소할 키 목록 */
	UPROPERTY(Category = "Key Selection", EditAnywhere, BlueprintReadOnly)
	TArray<FKey> EscapeKeys;

	/* 내부 Slate 포인터 */
private:
	/** 관리 중인 Slate 키 선택 위젯 인스턴스 */
	TSharedPtr<SInputKeySelector> MyInputKeySelector;
};
