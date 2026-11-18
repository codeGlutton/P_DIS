/*****************************************************************//**
 * @file   RichComboBoxKeyCustomizationExtender.h
 * @brief  UMG 디자이너의 Viewmodel 카테고리에서 URichComboBoxKey 확장 UI를 제공하는 커스터마이제이션 정의
 * @details PanelWidget의 ViewModel Extension UX를 모방하여 [Add/Remove Viewmodel Extension] 토글 버튼 및
 *          Content/Item 위젯 클래스와 각 Entry ViewModel 드롭다운·클래스 피커 UI를 제공함.
 * @date   2026-11-18
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Customizations/IBlueprintWidgetCustomizationExtender.h"
#include "Layout/Visibility.h"
#include "UObject/WeakObjectPtr.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class FReply;
class FWidgetBlueprintEditor;
class IDetailCategoryBuilder;
class IDetailLayoutBuilder;
class IPropertyHandle;
class IPropertyUtilities;
class SWidget;
class UMVVMBlueprintView;
class URichComboBoxKey;
class UWidget;
class UWidgetBlueprint;
struct FSlateBrush;

/**
 * URichComboBoxKey의 Content/Item 슬롯 구분 열거형
 */
enum class ERichComboBoxExtensionSlot : uint8
{
	Content,
	Item
};

/**
 * URichComboBoxKey 전용 UMG 디테일 Viewmodel 확장 커스터마이저
 */
class FRichComboBoxKeyCustomizationExtender : public IBlueprintWidgetCustomizationExtender
{
	/* 생성 및 IBlueprintWidgetCustomizationExtender 인터페이스 */
public:
	static TSharedPtr<FRichComboBoxKeyCustomizationExtender> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& InDetailLayout, const TArrayView<UWidget*> InWidgets, const TSharedRef<FWidgetBlueprintEditor>& InWidgetBlueprintEditor) override;

	/* 내부 UI 구축 및 상호작용 헬퍼 */
private:
	/* Content 또는 Item 슬롯의 위젯 클래스 · 엔트리 ViewModel 드롭다운 행을 추가함 (VM Class는 C++ 호환용으로 유지하되 디테일 패널에서는 숨김) */
	void AddEntrySlotRows(
		IDetailCategoryBuilder& InMVVMCategory,
		TWeakObjectPtr<URichComboBoxKey> InComboBox,
		TWeakPtr<IPropertyUtilities> InPropertyUtilities,
		ERichComboBoxExtensionSlot InSlot,
		const TSharedPtr<IPropertyHandle>& InWidgetClassHandle,
		const TSharedPtr<IPropertyHandle>& InVMNameHandle,
		const TSharedPtr<IPropertyHandle>& InVMClassHandle
	);

	/* Viewmodel Extension 토글 버튼 클릭 처리 */
	FReply HandleToggleExtensionClicked(
		TWeakObjectPtr<URichComboBoxKey> InComboBox,
		TWeakPtr<IPropertyUtilities> InPropertyUtilities,
		TSharedPtr<IPropertyHandle> InUseExtensionHandle,
		TSharedPtr<IPropertyHandle> InContentClassHandle,
		TSharedPtr<IPropertyHandle> InContentVMNameHandle,
		TSharedPtr<IPropertyHandle> InContentVMClassHandle,
		TSharedPtr<IPropertyHandle> InItemClassHandle,
		TSharedPtr<IPropertyHandle> InItemVMNameHandle,
		TSharedPtr<IPropertyHandle> InItemVMClassHandle
	);

	/* 위젯 클래스 변경 시 유효하지 않은 기존 ViewModelName을 정리하고 패널을 갱신함 */
	void HandleWidgetClassChanged(
		TWeakObjectPtr<URichComboBoxKey> InComboBox,
		TWeakPtr<IPropertyUtilities> InPropertyUtilities,
		TSharedPtr<IPropertyHandle> InWidgetClassHandle,
		TSharedPtr<IPropertyHandle> InVMNameHandle,
		TSharedPtr<IPropertyHandle> InVMClassHandle,
		ERichComboBoxExtensionSlot InSlot
	);

	/* 선택된 Entry ViewModel 드롭다운 메뉴 생성 */
	TSharedRef<SWidget> BuildEntryViewModelsMenu(
		TWeakObjectPtr<URichComboBoxKey> InComboBox,
		TWeakPtr<IPropertyUtilities> InPropertyUtilities,
		TSharedPtr<IPropertyHandle> InVMNameHandle,
		TSharedPtr<IPropertyHandle> InVMClassHandle,
		ERichComboBoxExtensionSlot InSlot
	);

	/* 드롭다운에서 Entry ViewModel 선택 적용 */
	void ApplyEntryViewModelSelection(
		TWeakObjectPtr<URichComboBoxKey> InComboBox,
		TWeakPtr<IPropertyUtilities> InPropertyUtilities,
		TSharedPtr<IPropertyHandle> InVMNameHandle,
		TSharedPtr<IPropertyHandle> InVMClassHandle,
		ERichComboBoxExtensionSlot InSlot,
		FName InViewModelName,
		UClass* InViewModelClass
	);

	/* 슬롯별 ViewModel 이름 및 기대 클래스를 프로퍼티 핸들과 대상 위젯에 일관 반영함 */
	static void WriteSlotViewModelProperties(
		URichComboBoxKey* InComboBox,
		const TSharedPtr<IPropertyHandle>& InVMNameHandle,
		const TSharedPtr<IPropertyHandle>& InVMClassHandle,
		ERichComboBoxExtensionSlot InSlot,
		FName InViewModelName,
		UClass* InViewModelClass
	);

	/* 버튼 및 드롭다운 표시 텍스트·아이콘 조회 */
	FText GetExtensionButtonText(TWeakObjectPtr<URichComboBoxKey> InComboBox) const;
	const FSlateBrush* GetExtensionButtonIcon(TWeakObjectPtr<URichComboBoxKey> InComboBox) const;
	FText GetSelectedEntryViewModelText(TWeakObjectPtr<URichComboBoxKey> InComboBox, ERichComboBoxExtensionSlot InSlot) const;

	/* 대상 슬롯의 위젯 클래스와 WBP MVVM View 조회 헬퍼 */
	static TSubclassOf<UUserWidget> GetSlotWidgetClass(const URichComboBoxKey* InComboBox, ERichComboBoxExtensionSlot InSlot);
	static FName GetSlotViewModelName(const URichComboBoxKey* InComboBox, ERichComboBoxExtensionSlot InSlot);
	static const UMVVMBlueprintView* FindBlueprintViewForWidgetClass(const UClass* InWidgetClass);
};
