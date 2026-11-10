/*****************************************************************//**
 * @file   SAssetLinterBatchFixDialog.h
 * @brief  Asset Linter 일괄 수정 확인 팝업 Slate 위젯 선언
 * @details 선택한 에셋들 중 린트 위반 항목들의 변경 전/후 이름과 위치를 확인하고 적용하는 모달 다이얼로그
 * @date   2026-11-11
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "AssetLinterTypes.h"

class SWindow;

/**
 * @class SAssetLinterBatchFixDialog
 * @brief 에디터 현재 테마 스타일을 반영하여 수정 대상 에셋 목록을 테이블 뷰로 확인하고 일괄 적용하는 모달 다이얼로그
 */
class ASSETLINTER_API SAssetLinterBatchFixDialog : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAssetLinterBatchFixDialog) {}
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(TArray<TSharedPtr<FAssetLintFixItem>>*, FixItems)
	SLATE_END_ARGS()

	/* 생성자 및 초기화 */
public:
	void Construct(const FArguments& InArgs);

	/* 정적 모달 실행 인터페이스 */
public:
	/**
	 * 다이얼로그를 모달 창으로 생성하여 표시하고 사용자의 확인 여부를 반환합니다.
	 * @param InFixItems 수정 대상 항목 목록
	 * @return 사용자가 '일괄 수정 적용'을 눌렀으면 true, '취소' 또는 창을 닫았으면 false
	 */
	static bool OpenDialog(TArray<TSharedPtr<FAssetLintFixItem>>& InFixItems);

	/* 내부 UI 생성 및 이벤트 핸들러 */
private:
	TSharedRef<ITableRow> OnGenerateRowForList(TSharedPtr<FAssetLintFixItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

	FReply OnApplyClicked();
	FReply OnCancelClicked();

	void OnToggleSelectAll(ECheckBoxState NewState);
	ECheckBoxState GetSelectAllCheckState() const;

	/* 멤버 변수 */
private:
	TWeakPtr<SWindow> ParentWindowPtr;
	TArray<TSharedPtr<FAssetLintFixItem>>* FixItemsPtr = nullptr;
	TSharedPtr<SListView<TSharedPtr<FAssetLintFixItem>>> ListViewWidget;
	bool bUserConfirmed = false;
};

