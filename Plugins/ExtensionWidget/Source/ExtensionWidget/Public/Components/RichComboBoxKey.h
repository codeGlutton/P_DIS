/*****************************************************************//**
 * @file   RichComboBoxKey.h
 * @brief  UComboBoxKey 완전 복제 및 ViewModel Extension·SelectedKey TwoWay 통합 위젯 정의
 * @details UWidget을 직접 상속해 SComboBox<FName>을 래핑하며, 기존 수동 옵션·대리자 API를 유지하는 동시에
 *          bUseViewModelExtension 활성화 시 OnGenerate* 대리자를 무시하고 ViewModelItems 기반 엔트리 위젯 자동 생성·주입을 수행함.
 *          또한 SelectedOption 선 대입 후 FieldNotify 발송으로 양방향 바인딩 순환 오류를 방지함.
 * @date   2026-11-19
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateTypes.h"
#include "Types/RichComboBoxTypes.h"
#include "UObject/ObjectMacros.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SWidget.h"
#include "Components/Widget.h"
#include "RichComboBoxKey.generated.h"

/* 독립 전방 선언 */
class FRichComboBoxKeyCustomizationExtender;
class SBox;
class UUserWidget;

/* SelectedOption 변경 통지용 네이티브/동적 멀티캐스트 대리자 선언 (선택 키와 항목 인덱스 함께 전달) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FRichComboBoxOnSelectedOptionChangedNativeSignature, FName /*SelectedOption*/, int32 /*ItemIndex*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRichComboBoxOnSelectedOptionChangedDynamicSignature, FName, SelectedOption, int32, ItemIndex);

/**
 * SComboBox<FName> 직접 래핑 기반 ViewModel Extension 및 안전한 SelectedOption 양방향 바인딩 콤보박스 위젯
 */
UCLASS(meta = (DisplayName = "Rich ComboBox (Key)"))
class EXTENSIONWIDGET_API URichComboBoxKey : public UWidget
{
	GENERATED_BODY()

	friend class FRichComboBoxKeyCustomizationExtender;

	/* 델리게이트 타입 */
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSelectionChangedEvent, FName, SelectedItem, ESelectInfo::Type, SelectionType);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOpeningEvent);
	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(UWidget*, FGenerateWidgetEvent, FName, Item);

	/* 생성자 */
public:
	URichComboBoxKey();

	/* MVVM 바인딩 프로퍼티 접근자 */
public:
	/** 현재 바인딩된 키-ViewModel 아이템 배열 반환 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintPure)
	const TArray<FRichComboBoxKeyViewModelItem>& GetViewModelItems() const { return ViewModelItems; }

	/** 키-ViewModel 아이템 배열 일괄 갱신 (선택 키 보존 및 Content 위젯 즉시 재생성 포함) */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintCallable)
	void SetViewModelItems(const TArray<FRichComboBoxKeyViewModelItem>& InItems);

	/** 내부 캐싱 맵에서 특정 키에 매칭되는 ViewModel 조회 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintPure)
	UObject* FindViewModelByKey(FName InKey) const;

	/** 내부 캐싱 맵에서 특정 키의 항목 인덱스 조회 (미존재 시 INDEX_NONE) */
	UFUNCTION(Category = "Rich ComboBox|Selection", BlueprintPure)
	int32 FindItemIndexByKey(FName InKey) const;

	/* UComboBoxKey 호환 수동 옵션·스타일 API */
public:
	/** 옵션 목록에 키를 추가함 */
	UFUNCTION(Category = "ComboBox", BlueprintCallable)
	void AddOption(FName Option);

	/** 옵션 목록에서 키를 제거함 */
	UFUNCTION(Category = "ComboBox", BlueprintCallable)
	bool RemoveOption(FName Option);

	/** 모든 옵션을 제거하고 선택 상태를 초기화함 */
	UFUNCTION(Category = "ComboBox", BlueprintCallable)
	void ClearOptions();

	/** 현재 선택 상태를 초기화함 */
	UFUNCTION(Category = "ComboBox", BlueprintCallable)
	void ClearSelection();

	/** 현재 선택된 옵션을 설정함 */
	UFUNCTION(Category = "ComboBox", BlueprintCallable)
	void SetSelectedOption(FName Option);

	/** 현재 선택된 옵션을 반환함 */
	UFUNCTION(Category = "ComboBox", BlueprintCallable)
	FName GetSelectedOption() const;

	/** 콤보박스 드롭다운 메뉴 열림 여부를 반환함 */
	UFUNCTION(Category = "ComboBox", BlueprintCallable, meta = (ReturnDisplayName = "bOpen"))
	bool IsOpen() const;

	/** 콘텐츠 패딩 설정 */
	void SetContentPadding(FMargin InPadding);

	/** 콘텐츠 패딩 반환 */
	FMargin GetContentPadding() const;

	/** 게임패드 내비게이션 모드 활성화 여부 반환 */
	bool IsEnableGamepadNavigationMode() const;

	/** 게임패드 내비게이션 모드 활성화 여부 설정 */
	void SetEnableGamepadNavigationMode(bool InEnableGamepadNavigationMode);

	/** 드롭다운 화살표 표시 여부 반환 */
	bool IsHasDownArrow() const;

	/** 드롭다운 화살표 표시 여부 설정 */
	void SetHasDownArrow(bool InHasDownArrow);

	/** 드롭다운 목록 최대 높이 반환 */
	float GetMaxListHeight() const;

	/** 드롭다운 목록 최대 높이 설정 */
	void SetMaxListHeight(float InMaxHeight);

	/** 콤보박스 스타일 반환 */
	const FComboBoxStyle& GetWidgetStyle() const;

	/** 콤보박스 스타일 설정 */
	void SetWidgetStyle(const FComboBoxStyle& InWidgetStyle);

	/** 항목 행 스타일 반환 */
	const FTableRowStyle& GetItemStyle() const;

	/** 항목 행 스타일 설정 */
	void SetItemStyle(const FTableRowStyle& InItemStyle);

	/** 스크롤바 스타일 반환 */
	const FScrollBarStyle& GetScrollBarStyle() const;

	/** 포커스 가능 여부 반환 */
	bool IsFocusable() const;

	/** 전경색 반환 */
	FSlateColor GetForegroundColor() const;

	/* ViewModel Extension 설정 접근자 */
public:
	/** ViewModel Extension 자동 위젯 생성·대입 기능 활성화 여부 조회 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintPure)
	bool GetUseViewModelExtension() const { return bUseViewModelExtension; }

	/** ViewModel Extension 활성화 여부 설정 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintCallable)
	void SetUseViewModelExtension(bool bInUseViewModelExtension);

	/** Content 위젯 클래스 반환 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintPure)
	TSubclassOf<UUserWidget> GetContentWidgetClass() const { return ContentWidgetClass; }

	/** Content 위젯 클래스 설정 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintCallable)
	void SetContentWidgetClass(TSubclassOf<UUserWidget> InContentWidgetClass);

	/** Content 위젯 타겟 ViewModel 이름 반환 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintPure)
	FName GetContentViewModelName() const { return ContentViewModelName; }

	/** Content 위젯 타겟 ViewModel 이름 설정 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintCallable)
	void SetContentViewModelName(FName InContentViewModelName);

	/** Content 위젯 기대 ViewModel 클래스 반환 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintPure)
	TSubclassOf<UObject> GetContentViewModelClass() const { return ContentViewModelClass; }

	/** Content 위젯 기대 ViewModel 클래스 설정 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintCallable)
	void SetContentViewModelClass(TSubclassOf<UObject> InContentViewModelClass);

	/** Item 위젯 클래스 반환 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintPure)
	TSubclassOf<UUserWidget> GetItemWidgetClass() const { return ItemWidgetClass; }

	/** Item 위젯 클래스 설정 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintCallable)
	void SetItemWidgetClass(TSubclassOf<UUserWidget> InItemWidgetClass);

	/** Item 위젯 타겟 ViewModel 이름 반환 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintPure)
	FName GetItemViewModelName() const { return ItemViewModelName; }

	/** Item 위젯 타겟 ViewModel 이름 설정 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintCallable)
	void SetItemViewModelName(FName InItemViewModelName);

	/** Item 위젯 기대 ViewModel 클래스 반환 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintPure)
	TSubclassOf<UObject> GetItemViewModelClass() const { return ItemViewModelClass; }

	/** Item 위젯 기대 ViewModel 클래스 설정 */
	UFUNCTION(Category = "Rich ComboBox|ViewModel", BlueprintCallable)
	void SetItemViewModelClass(TSubclassOf<UObject> InItemViewModelClass);

	/* UWidget / UVisual 인터페이스 */
public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** 생성자 전용 초기화 헬퍼 (Slate 위젯 생성 전 호출) */
	void InitScrollBarStyle(const FScrollBarStyle& InScrollBarStyle);
	void InitIsFocusable(bool InIsFocusable);
	void InitForegroundColor(FSlateColor InForegroundColor);

	/* 내부 Slate 콜백 및 위젯 생성 헬퍼 */
private:
	/** 선택된 Content 박스의 위젯을 갱신함 (bUseViewModelExtension 시 VM 자동 주입 분기) */
	void GenerateContent();

	/** Slate가 드롭다운 항목 위젯을 요청할 때 호출됨 */
	TSharedRef<SWidget> HandleGenerateItemWidget(FName Item);

	/** Slate 선택 변경 시 호출됨 */
	void HandleSelectionChanged(FName Item, ESelectInfo::Type SelectionType);

	/** Slate 콤보박스 열림 시 호출됨 */
	void HandleOpening();

	/** 지정 위젯 클래스를 생성하고 키에 해당하는 ViewModel을 UMVVMView에 대입함 */
	UUserWidget* GenerateEntryWidgetWithViewModel(
		FName InKey,
		TSubclassOf<UUserWidget> InWidgetClass,
		FName InViewModelName,
		TSubclassOf<UObject> InExpectedViewModelClass
	);

	/* C++ / 블루프린트 이벤트 및 대리자 */
public:
	/** SelectedOption 변경 시 선택 키와 항목 인덱스를 전달하는 네이티브 대리자 */
	FRichComboBoxOnSelectedOptionChangedNativeSignature OnSelectedOptionChangedNative;

	/** SelectedOption 변경 시 선택 키와 항목 인덱스를 전달하는 블루프린트 대리자 */
	UPROPERTY(Category = "Rich ComboBox|Events", BlueprintAssignable)
	FRichComboBoxOnSelectedOptionChangedDynamicSignature OnSelectedOptionChanged;

	/** Content 영역 위젯 수동 생성 이벤트 (bUseViewModelExtension 활성화 시 비활성화됨) */
	UPROPERTY(Category = "Events", EditAnywhere, meta = (IsBindableEvent = "True", EditCondition = "!bUseViewModelExtension"))
	FGenerateWidgetEvent OnGenerateContentWidget;

	/** 드롭다운 목록 항목 위젯 수동 생성 이벤트 (bUseViewModelExtension 활성화 시 비활성화됨) */
	UPROPERTY(Category = "Events", EditAnywhere, meta = (IsBindableEvent = "True", EditCondition = "!bUseViewModelExtension"))
	FGenerateWidgetEvent OnGenerateItemWidget;

	/** 새 항목 선택 시 호출되는 콤보박스 이벤트 */
	UPROPERTY(Category = "Events", BlueprintAssignable)
	FOnSelectionChangedEvent OnSelectionChanged;

	/** 콤보박스가 열릴 때 호출되는 이벤트 */
	UPROPERTY(Category = "Events", BlueprintAssignable)
	FOnOpeningEvent OnOpening;

	/* UComboBoxKey 스타일 프로퍼티 */
protected:
	/** 콤보박스 버튼/메뉴 스타일 */
	UPROPERTY(Category = "Style", EditAnywhere, BlueprintReadWrite, Getter, Setter, meta = (DisplayName = "Style"))
	FComboBoxStyle WidgetStyle;

	/** 목록 항목 행 스타일 */
	UPROPERTY(Category = "Style", EditAnywhere, BlueprintReadWrite, Getter, Setter)
	FTableRowStyle ItemStyle;

	/** 목록 스크롤바 스타일 */
	UPROPERTY(Category = "Style", EditAnywhere, BlueprintReadOnly, Getter)
	FScrollBarStyle ScrollBarStyle;

	/** 전경색 */
	UPROPERTY(Category = "Style", EditAnywhere, BlueprintReadOnly, Getter, meta = (DesignerRebuild))
	FSlateColor ForegroundColor;

	/** 콘텐츠 영역 여백 */
	UPROPERTY(Category = "Style", EditAnywhere, BlueprintReadWrite, Getter, Setter)
	FMargin ContentPadding;

	/** 드롭다운 목록 최대 높이 */
	UPROPERTY(Category = "Style", EditAnywhere, BlueprintReadWrite, Getter, Setter, AdvancedDisplay)
	float MaxListHeight = 450.0f;

	/** 드롭다운 화살표 아이콘 표시 여부 */
	UPROPERTY(Category = "Style", EditAnywhere, BlueprintReadWrite, Getter = "IsHasDownArrow", Setter = "SetHasDownArrow", AdvancedDisplay)
	bool bHasDownArrow = true;

	/** 게임패드 방향키 내비게이션 전용 활성화 여부 */
	UPROPERTY(Category = "Style", EditAnywhere, BlueprintReadWrite, Getter = "IsEnableGamepadNavigationMode", Setter = "SetEnableGamepadNavigationMode", AdvancedDisplay)
	bool bEnableGamepadNavigationMode = true;

	/** 키보드 포커스 수신 가능 여부 */
	UPROPERTY(Category = "Style", EditAnywhere, BlueprintReadOnly, Getter = "IsFocusable")
	bool bIsFocusable = true;

	/* MVVM 바인딩 및 옵션 프로퍼티 */
private:
	/** SComboBox<FName>::OptionsSource에 바인딩되는 옵션 키 목록 */
	UPROPERTY(Category = "Content", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TArray<FName> Options;

	/** 현재 선택된 옵션 키 */
	UPROPERTY(Category = "Content", BlueprintReadWrite, FieldNotify, Getter = GetSelectedOption, Setter = SetSelectedOption, meta = (AllowPrivateAccess = "true"))
	FName SelectedOption = NAME_None;

	/** 키-ViewModel 엔트리 배열 (MVVM OneWayToDestination 바인딩 대상) */
	UPROPERTY(Category = "Rich ComboBox|ViewModel", BlueprintReadWrite, FieldNotify, Getter = GetViewModelItems, Setter = SetViewModelItems, meta = (AllowPrivateAccess = "true"))
	TArray<FRichComboBoxKeyViewModelItem> ViewModelItems;

	/* ViewModel Extension 모방 설정 */
private:
	/** Content/Item 위젯 자동 생성 및 ViewModel 자동 주입 기능 사용 여부 */
	UPROPERTY(Category = "Viewmodel", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool bUseViewModelExtension = false;

	/** 선택된 Content 영역에 생성할 엔트리 위젯 클래스 */
	UPROPERTY(Category = "Viewmodel", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> ContentWidgetClass;

	/** Content 위젯 내 바인딩할 ViewModel 이름 (미지정 시 클래스 매칭 폴백) */
	UPROPERTY(Category = "Viewmodel", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FName ContentViewModelName = NAME_None;

	/** Content 위젯에 기대하는 ViewModel 클래스 (에디터 검증 및 런타임 타입 체크용) */
	UPROPERTY(Category = "Viewmodel", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", AllowedClasses = "/Script/FieldNotification.NotifyFieldValueChanged"))
	TSubclassOf<UObject> ContentViewModelClass;

	/** 드롭다운 목록 항목에 생성할 엔트리 위젯 클래스 */
	UPROPERTY(Category = "Viewmodel", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> ItemWidgetClass;

	/** Item 위젯 내 바인딩할 ViewModel 이름 (미지정 시 클래스 매칭 폴백) */
	UPROPERTY(Category = "Viewmodel", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FName ItemViewModelName = NAME_None;

	/** Item 위젯에 기대하는 ViewModel 클래스 (에디터 검증 및 런타임 타입 체크용) */
	UPROPERTY(Category = "Viewmodel", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", AllowedClasses = "/Script/FieldNotification.NotifyFieldValueChanged"))
	TSubclassOf<UObject> ItemViewModelClass;

	/* 내부 Slate 위젯 포인터·캐시·재진입 가드 */
private:
	/** 관리 중인 Slate 콤보박스 위젯 */
	TSharedPtr<SComboBox<FName>> MyComboBox;

	/** 선택된 콘텐츠 위젯을 수용하는 Slate 박스 */
	TSharedPtr<SBox> ComboBoxContent;

	/** FName 키로 ViewModelItems/옵션 인덱스를 O(1) 조회하기 위한 내부 매핑 테이블 */
	TMap<FName, int32> CachedKeyIndices;
};
