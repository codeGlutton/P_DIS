#include "Customizations/BBMInventoryDefinitionCustomization.h"
#include "Inventory/BBMInventoryDefinition.h"
#include "Inventory/BBMInventoryTypes.h"
#include "Inventory/BBMSlotEnhancementEffect.h"
#include "Enhancement/DISEnhancement.h"
#include "Enhancement/DISEnhancementEffect.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ScopedTransaction.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "FBBMInventoryDefinitionCustomization"

TSharedRef<IDetailCustomization> FBBMInventoryDefinitionCustomization::MakeInstance()
{
	return MakeShared<FBBMInventoryDefinitionCustomization>();
}

void FBBMInventoryDefinitionCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailLayout = &DetailBuilder;

	// 대상 UBBMInventoryDefinition 인스턴스 획득
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

	if (CustomizedObjects.Num() == 1 && CustomizedObjects[0].IsValid() == true)
	{
		TargetDefinition = Cast<UBBMInventoryDefinition>(CustomizedObjects[0].Get());
	}

	SlotEnhancementsPropertyHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UBBMInventoryDefinition, SlotEnhancements),
		UBBMInventoryDefinition::StaticClass()
	);

	// 슬롯 강화 인터랙티브 2D 매트릭스 전용 최상위 카테고리 구성
	IDetailCategoryBuilder& MatrixCategory = DetailBuilder.EditCategory(
		"BBM Slot Enhancement Matrix",
		LOCTEXT("SlotEnhancementMatrixCategory", "BBM 슬롯 강화 2D 인터랙티브 매트릭스"),
		ECategoryPriority::Important
	);

	MatrixCategory.AddCustomRow(LOCTEXT("SlotEnhancementMatrixRow", "슬롯 강화 2D 매트릭스"))
	.WholeRowContent()
	[
		SNew(SVerticalBox)
		// 안내 헤더
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 4.0f, 0.0f, 8.0f)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f)))
			.Text(LOCTEXT("MatrixGuideText", "배낭 타일을 클릭하여 해당 칸에 부여할 슬롯 강화를 직관적으로 확인하고 편집할 수 있습니다. (막힌 칸은 BlockedTileShape에 의해 비활성화)"))
		]
		// 메인 2D 그리드 패널
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(12.0f)
			[
				SAssignNew(GridContainer, SGridPanel)
			]
		]
		// 구분선
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			SNew(SSeparator)
			.Orientation(Orient_Horizontal)
		]
		// 하단 슬롯 강화 상세 편집 패널
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 8.0f, 0.0f, 4.0f)
		[
			SAssignNew(DetailContainer, SVerticalBox)
		]
	];

	// 초기 UI 렌더링
	RefreshGridWidget();
	RefreshDetailPanelWidget();
}

UBBMInventoryDefinition* FBBMInventoryDefinitionCustomization::GetTargetDefinition() const
{
	return TargetDefinition.Get();
}

void FBBMInventoryDefinitionCustomization::MarkAssetDirty()
{
	UBBMInventoryDefinition* TargetDef = GetTargetDefinition();
	if (TargetDef == nullptr)
	{
		return;
	}

	if (SlotEnhancementsPropertyHandle.IsValid() == true)
	{
		SlotEnhancementsPropertyHandle->NotifyPreChange();
		SlotEnhancementsPropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}

	TargetDef->MarkPackageDirty();
}

void FBBMInventoryDefinitionCustomization::RefreshGridWidget()
{
	if (GridContainer.IsValid() == false)
	{
		return;
	}

	GridContainer->ClearChildren();

	UBBMInventoryDefinition* TargetDef = GetTargetDefinition();
	if (TargetDef == nullptr)
	{
		return;
	}

	const int32 Width = (TargetDef->LineBreakTileCount > 0) ? TargetDef->LineBreakTileCount : 4;
	const int32 TotalCapacity = (TargetDef->DefaultCapacity > 0) ? TargetDef->DefaultCapacity : 16;
	const int32 Height = FMath::Max(1, FMath::CeilToInt((float)TotalCapacity / Width));

	const float TileSize = 48.0f;

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FIntPoint Coord(X, Y);
			const int32 SlotIndex = TargetDef->CoordToSlotIndex(Coord);
			const bool bOutOfRange = (SlotIndex < 0 || SlotIndex >= TotalCapacity);

			if (bOutOfRange == true)
			{
				continue;
			}

			const bool bIsBlocked = TargetDef->IsTileBlocked(Coord);
			const FBBMSlotEffectSpecList* SpecList = TargetDef->FindSlotEnhancements(Coord);
			const bool bHasEnhancement = (SpecList != nullptr && SpecList->Effects.Num() > 0);
			const bool bIsSelected = (SelectedCoord == Coord);

			// 타일 배경 색상 결정
			FLinearColor BgColor;
			FText TileLabel;
			FText TooltipText;

			if (bIsBlocked == true)
			{
				BgColor = FLinearColor(0.12f, 0.12f, 0.12f, 0.8f);
				TileLabel = FText::FromString(TEXT("X"));
				TooltipText = FText::Format(LOCTEXT("BlockedTileTooltip", "좌표 ({0}, {1}) - 사용 불가하게 막힌 타일 (BlockedTileShape)"), FText::AsNumber(X), FText::AsNumber(Y));
			}
			else if (bHasEnhancement == true)
			{
				BgColor = bIsSelected ? FLinearColor(0.85f, 0.45f, 0.1f, 1.0f) : FLinearColor(0.55f, 0.25f, 0.05f, 1.0f);
				TileLabel = FText::Format(LOCTEXT("EnhancedTileLabel", "+{0}"), FText::AsNumber(SpecList->Effects.Num()));
				TooltipText = FText::Format(LOCTEXT("EnhancedTileTooltip", "좌표 ({0}, {1}) - 슬롯 강화 효과 {2}개 설정됨"), FText::AsNumber(X), FText::AsNumber(Y), FText::AsNumber(SpecList->Effects.Num()));
			}
			else
			{
				BgColor = bIsSelected ? FLinearColor(0.2f, 0.45f, 0.7f, 1.0f) : FLinearColor(0.18f, 0.2f, 0.24f, 1.0f);
				TileLabel = FText::Format(LOCTEXT("NormalTileLabel", "{0},{1}"), FText::AsNumber(X), FText::AsNumber(Y));
				TooltipText = FText::Format(LOCTEXT("NormalTileTooltip", "좌표 ({0}, {1}) - 클릭하여 슬롯 강화 편집"), FText::AsNumber(X), FText::AsNumber(Y));
			}

			// 테두리 색상 (선택된 타일은 밝은 노란색 테두리)
			const FLinearColor BorderColor = bIsSelected ? FLinearColor(1.0f, 0.85f, 0.2f, 1.0f) : FLinearColor(0.08f, 0.08f, 0.08f, 1.0f);

			GridContainer->AddSlot(X, Y)
			.Padding(2.0f)
			[
				SNew(SBox)
				.WidthOverride(TileSize)
				.HeightOverride(TileSize)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(BorderColor)
					.Padding(bIsSelected ? 2.0f : 1.0f)
					[
						SNew(SButton)
						.IsEnabled(bIsBlocked == false)
						.ButtonColorAndOpacity(BgColor)
						.ToolTipText(TooltipText)
						.OnClicked(this, &FBBMInventoryDefinitionCustomization::OnTileClicked, Coord)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font(IDetailLayoutBuilder::GetDetailFontBold())
							.ColorAndOpacity(bIsBlocked ? FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f)) : FSlateColor(FLinearColor::White))
							.Text(TileLabel)
						]
					]
				]
			];
		}
	}
}

void FBBMInventoryDefinitionCustomization::RefreshDetailPanelWidget()
{
	if (DetailContainer.IsValid() == false)
	{
		return;
	}

	DetailContainer->ClearChildren();

	UBBMInventoryDefinition* TargetDef = GetTargetDefinition();
	if (TargetDef == nullptr)
	{
		return;
	}

	// 타일이 선택되지 않은 경우 안내 문구 표시
	if (SelectedCoord == FIntPoint::NoneValue)
	{
		DetailContainer->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 8.0f)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFontItalic())
			.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f)))
			.Text(LOCTEXT("NoTileSelectedPrompt", "위 매트릭스에서 타일을 클릭하면 해당 칸의 슬롯 강화를 설정할 수 있습니다."))
		];
		return;
	}

	const int32 SelectedSlotIndex = TargetDef->CoordToSlotIndex(SelectedCoord);

	// 1. 선택된 타일 헤더 및 정보
	DetailContainer->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 4.0f, 0.0f, 8.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
			.Text(FText::Format(LOCTEXT("SelectedTileHeader", "선택된 타일: 좌표 ({0}, {1}) | 슬롯 인덱스: {2}"),
				FText::AsNumber(SelectedCoord.X),
				FText::AsNumber(SelectedCoord.Y),
				FText::AsNumber(SelectedSlotIndex)))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(16.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ToolTipText(LOCTEXT("ClearTileTooltip", "이 타일에 설정된 모든 강화를 삭제합니다."))
			.OnClicked(this, &FBBMInventoryDefinitionCustomization::OnClearSlotEnhancementsClicked)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.4f, 0.4f, 1.0f)))
				.Text(LOCTEXT("ClearTileBtn", "선택 칸 모든 강화 삭제"))
			]
		]
	];

	// 2. 현재 설정된 강화 항목 목록
	const FBBMSlotEffectSpecList* SpecList = TargetDef->FindSlotEnhancements(SelectedCoord);
	if (SpecList != nullptr && SpecList->Effects.Num() > 0)
	{
		for (int32 Index = 0; Index < SpecList->Effects.Num(); ++Index)
		{
			const FDISEnhancementEffectSpec& Spec = SpecList->Effects[Index];
			const FString ClassName = (Spec.EffectClass != nullptr) ? Spec.EffectClass->GetName() : TEXT("None");

			DetailContainer->AddSlot()
			.AutoHeight()
			.Padding(4.0f, 2.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(6.0f, 4.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.Text(FText::Format(LOCTEXT("EnhanceItemRow", "[{0}] {1} (강도: {2})"),
							FText::AsNumber(Index + 1),
							FText::FromString(ClassName),
							FText::AsNumber(Spec.OverallMagnitude)))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ToolTipText(LOCTEXT("RemoveEntryTooltip", "이 강화 효과 항목을 삭제합니다."))
						.OnClicked(this, &FBBMInventoryDefinitionCustomization::OnRemoveEnhancementClicked, Index)
						[
							SNew(STextBlock)
							.Font(IDetailLayoutBuilder::GetDetailFont())
							.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f, 1.0f)))
							.Text(LOCTEXT("RemoveBtn", "삭제"))
						]
					]
				]
			];
		}
	}
	else
	{
		DetailContainer->AddSlot()
		.AutoHeight()
		.Padding(4.0f, 4.0f)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFontItalic())
			.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)))
			.Text(LOCTEXT("NoEnhancementsOnSlot", "이 타일에는 현재 부여된 슬롯 강화가 없습니다."))
		];
	}

	// 3. 신규 강화 추가 영역
	DetailContainer->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 8.0f, 0.0f, 0.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(LOCTEXT("AddMagnitudeLabel", "강도/보너스 수치:"))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 12.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(70.0f)
			[
				SNew(SSpinBox<float>)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.MinValue(1.0f)
				.MaxValue(99.0f)
				.Value(PendingOverallMagnitude)
				.OnValueChanged_Lambda([this](float NewVal)
				{
					PendingOverallMagnitude = NewVal;
				})
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
			.ToolTipText(LOCTEXT("AddEnhanceBtnTooltip", "선택된 타일에 강화를 추가합니다."))
			.OnClicked(this, &FBBMInventoryDefinitionCustomization::OnAddEnhancementClicked)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
				.Text(LOCTEXT("AddEnhanceBtn", "+ 슬롯 강화 효과 추가"))
			]
		]
	];
}

FReply FBBMInventoryDefinitionCustomization::OnTileClicked(FIntPoint Coord)
{
	SelectedCoord = Coord;
	RefreshGridWidget();
	RefreshDetailPanelWidget();
	return FReply::Handled();
}

FReply FBBMInventoryDefinitionCustomization::OnAddEnhancementClicked()
{
	UBBMInventoryDefinition* TargetDef = GetTargetDefinition();
	if (TargetDef == nullptr || SelectedCoord == FIntPoint::NoneValue)
	{
		return FReply::Handled();
	}

	// 기본 유효한 UBBMSlotEnhancementEffect 파생 클래스 자동 탐색
	if (PendingEffectClass == nullptr)
	{
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->IsChildOf(UBBMSlotEnhancementEffect::StaticClass()) == true && It->HasAnyClassFlags(CLASS_Abstract) == false)
			{
				PendingEffectClass = *It;
				break;
			}
		}
	}

	if (PendingEffectClass == nullptr)
	{
		return FReply::Handled();
	}

	FScopedTransaction Transaction(LOCTEXT("AddSlotEnhancementTx", "Add Slot Enhancement"));
	TargetDef->Modify();

	FBBMSlotEffectSpecList& SpecList = TargetDef->SlotEnhancements.FindOrAdd(SelectedCoord);
	SpecList.Effects.Add(FDISEnhancementEffectSpec(PendingEffectClass, FMath::Max(1.0f, PendingOverallMagnitude)));

	MarkAssetDirty();
	RefreshGridWidget();
	RefreshDetailPanelWidget();

	return FReply::Handled();
}

FReply FBBMInventoryDefinitionCustomization::OnRemoveEnhancementClicked(int32 EntryIndex)
{
	UBBMInventoryDefinition* TargetDef = GetTargetDefinition();
	if (TargetDef == nullptr || SelectedCoord == FIntPoint::NoneValue)
	{
		return FReply::Handled();
	}

	if (FBBMSlotEffectSpecList* SpecList = TargetDef->SlotEnhancements.Find(SelectedCoord))
	{
		if (SpecList->Effects.IsValidIndex(EntryIndex) == true)
		{
			FScopedTransaction Transaction(LOCTEXT("RemoveSlotEnhancementTx", "Remove Slot Enhancement"));
			TargetDef->Modify();

			SpecList->Effects.RemoveAt(EntryIndex);
			if (SpecList->Effects.Num() == 0)
			{
				TargetDef->SlotEnhancements.Remove(SelectedCoord);
			}

			MarkAssetDirty();
			RefreshGridWidget();
			RefreshDetailPanelWidget();
		}
	}

	return FReply::Handled();
}

FReply FBBMInventoryDefinitionCustomization::OnClearSlotEnhancementsClicked()
{
	UBBMInventoryDefinition* TargetDef = GetTargetDefinition();
	if (TargetDef == nullptr || SelectedCoord == FIntPoint::NoneValue)
	{
		return FReply::Handled();
	}

	if (TargetDef->SlotEnhancements.Contains(SelectedCoord) == true)
	{
		FScopedTransaction Transaction(LOCTEXT("ClearSlotEnhancementsTx", "Clear Slot Enhancements"));
		TargetDef->Modify();

		TargetDef->SlotEnhancements.Remove(SelectedCoord);

		MarkAssetDirty();
		RefreshGridWidget();
		RefreshDetailPanelWidget();
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

