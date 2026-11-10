#include "UI/SAssetLinterBatchFixDialog.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Styling/AppStyle.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "SAssetLinterBatchFixDialog"

/**
 * @class SAssetLintFixRow
 * @brief 일괄 수정 대상 목록의 각 행(Row)을 렌더링하는 다중 컬럼 테이블 행 위젯
 */
class SAssetLintFixRow : public SMultiColumnTableRow<TSharedPtr<FAssetLintFixItem>>
{
public:
	SLATE_BEGIN_ARGS(SAssetLintFixRow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, TSharedPtr<FAssetLintFixItem> InItem)
	{
		Item = InItem;
		SMultiColumnTableRow<TSharedPtr<FAssetLintFixItem>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (Item.IsValid() == false)
		{
			return SNullWidget::NullWidget;
		}

		if (ColumnName == TEXT("Select"))
		{
			return SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]()
					{
						return (Item.IsValid() && Item->bIsSelected) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						if (Item.IsValid())
						{
							Item->bIsSelected = (NewState == ECheckBoxState::Checked);
						}
					})
				];
		}
		else if (ColumnName == TEXT("Name"))
		{
			if (Item->bNeedsRename == true)
			{
				return SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item->CurrentName))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.4f, 0.3f)))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("➔")))
						.Font(FAppStyle::GetFontStyle("NormalFontBold"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item->ProposedName))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.85f, 0.4f)))
						.Font(FAppStyle::GetFontStyle("NormalFontBold"))
					];
			}
			else
			{
				return SNew(SBox)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item->CurrentName))
					];
			}
		}
		else if (ColumnName == TEXT("Path"))
		{
			if (Item->bNeedsMove == true)
			{
				return SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item->CurrentPath))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.6f, 0.2f)))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("➔")))
						.Font(FAppStyle::GetFontStyle("NormalFontBold"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item->ProposedPath))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.85f, 0.4f)))
						.Font(FAppStyle::GetFontStyle("NormalFontBold"))
					];
			}
			else
			{
				return SNew(SBox)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item->CurrentPath))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
					];
			}
		}
		else if (ColumnName == TEXT("Reason"))
		{
			return SNew(SBox)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(Item->ViolationReason)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f)))
				];
		}

		return SNullWidget::NullWidget;
	}

private:
	TSharedPtr<FAssetLintFixItem> Item;
};

void SAssetLinterBatchFixDialog::Construct(const FArguments& InArgs)
{
	ParentWindowPtr = InArgs._ParentWindow;
	FixItemsPtr = InArgs._FixItems;
	bUserConfirmed = false;

	const int32 TotalCount = FixItemsPtr ? FixItemsPtr->Num() : 0;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		.Padding(16.0f)
		[
			SNew(SVerticalBox)

			// 1. 헤더 안내 바
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(20.0f)
					.HeightOverride(20.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("Icons.Warning"))
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::Format(
						LOCTEXT("HeaderSummary", "선택한 에셋 중 총 {0}개의 에셋이 린트 규칙(네이밍 또는 위치)을 위반하여 보정이 필요합니다.\n적용할 항목을 선택한 후 [일괄 수정 적용]을 누르십시오."),
						FText::AsNumber(TotalCount)
					))
					.Font(FAppStyle::GetFontStyle("NormalFont"))
				]
			]

			// 2. 에셋 목록 테이블 뷰
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(0.0f)
				[
					SAssignNew(ListViewWidget, SListView<TSharedPtr<FAssetLintFixItem>>)
					.ListItemsSource(FixItemsPtr)
					.OnGenerateRow(this, &SAssetLinterBatchFixDialog::OnGenerateRowForList)
					.HeaderRow
					(
						SNew(SHeaderRow)

						// 전체 선택/해제 체크박스 컬럼
						+ SHeaderRow::Column(TEXT("Select"))
						.DefaultLabel(FText::GetEmpty())
						.FixedWidth(36.0f)
						.HAlignHeader(HAlign_Center)
						.HeaderContent()
						[
							SNew(SCheckBox)
							.IsChecked(this, &SAssetLinterBatchFixDialog::GetSelectAllCheckState)
							.OnCheckStateChanged(this, &SAssetLinterBatchFixDialog::OnToggleSelectAll)
						]

						// 에셋 명칭 컬럼
						+ SHeaderRow::Column(TEXT("Name"))
						.DefaultLabel(LOCTEXT("Col_Name", "에셋 명칭 (현재 ➔ 변경 제안)"))
						.FillWidth(0.35f)

						// 에셋 위치 컬럼
						+ SHeaderRow::Column(TEXT("Path"))
						.DefaultLabel(LOCTEXT("Col_Path", "패키지 경로 (현재 ➔ 이동 제안)"))
						.FillWidth(0.35f)

						// 위반 사유 컬럼
						+ SHeaderRow::Column(TEXT("Reason"))
						.DefaultLabel(LOCTEXT("Col_Reason", "위반 사유"))
						.FillWidth(0.3f)
					)
				]
			]

			// 3. 하단 액션 버튼 바
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpacer)
				]

				// 일괄 수정 적용 버튼 (Primary)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::GetWidgetStyle<FButtonStyle>("PrimaryButton"))
					.OnClicked(this, &SAssetLinterBatchFixDialog::OnApplyClicked)
					.ContentPadding(FMargin(16.0f, 6.0f))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ApplyBtn", "일괄 수정 적용"))
						.Font(FAppStyle::GetFontStyle("NormalFontBold"))
					]
				]

				// 취소 버튼
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::GetWidgetStyle<FButtonStyle>("Button"))
					.OnClicked(this, &SAssetLinterBatchFixDialog::OnCancelClicked)
					.ContentPadding(FMargin(16.0f, 6.0f))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CancelBtn", "취소"))
					]
				]
			]
		]
	];
}

bool SAssetLinterBatchFixDialog::OpenDialog(TArray<TSharedPtr<FAssetLintFixItem>>& InFixItems)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("DialogTitle", "Asset Linter - 일괄 수정 확인"))
		.ClientSize(FVector2D(1000.0f, 600.0f))
		.SupportsMinimize(false)
		.SupportsMaximize(true);

	TSharedRef<SAssetLinterBatchFixDialog> DialogWidget = SNew(SAssetLinterBatchFixDialog)
		.ParentWindow(Window)
		.FixItems(&InFixItems);

	Window->SetContent(DialogWidget);

	GEditor->EditorAddModalWindow(Window);

	return DialogWidget->bUserConfirmed;
}

TSharedRef<ITableRow> SAssetLinterBatchFixDialog::OnGenerateRowForList(TSharedPtr<FAssetLintFixItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SAssetLintFixRow, OwnerTable, Item);
}

FReply SAssetLinterBatchFixDialog::OnApplyClicked()
{
	bUserConfirmed = true;

	if (TSharedPtr<SWindow> ParentWindow = ParentWindowPtr.Pin())
	{
		ParentWindow->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply SAssetLinterBatchFixDialog::OnCancelClicked()
{
	bUserConfirmed = false;

	if (TSharedPtr<SWindow> ParentWindow = ParentWindowPtr.Pin())
	{
		ParentWindow->RequestDestroyWindow();
	}

	return FReply::Handled();
}

void SAssetLinterBatchFixDialog::OnToggleSelectAll(ECheckBoxState NewState)
{
	if (FixItemsPtr == nullptr)
	{
		return;
	}

	const bool bSelect = (NewState == ECheckBoxState::Checked);
	for (TSharedPtr<FAssetLintFixItem>& Item : *FixItemsPtr)
	{
		if (Item.IsValid())
		{
			Item->bIsSelected = bSelect;
		}
	}

	if (ListViewWidget.IsValid())
	{
		ListViewWidget->RequestListRefresh();
	}
}

ECheckBoxState SAssetLinterBatchFixDialog::GetSelectAllCheckState() const
{
	if (FixItemsPtr == nullptr || FixItemsPtr->Num() == 0)
	{
		return ECheckBoxState::Unchecked;
	}

	int32 SelectedCount = 0;
	for (const TSharedPtr<FAssetLintFixItem>& Item : *FixItemsPtr)
	{
		if (Item.IsValid() && Item->bIsSelected)
		{
			SelectedCount++;
		}
	}

	if (SelectedCount == FixItemsPtr->Num())
	{
		return ECheckBoxState::Checked;
	}
	else if (SelectedCount > 0)
	{
		return ECheckBoxState::Undetermined;
	}

	return ECheckBoxState::Unchecked;
}

#undef LOCTEXT_NAMESPACE

