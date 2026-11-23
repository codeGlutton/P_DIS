#include "Customizations/BBMInventoryDefinitionCustomization.h"
#include "Inventory/BBMInventoryDefinition.h"
#include "Inventory/BBMInventoryTypes.h"
#include "Enhancement/DISEnhancementEffect.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FBBMInventoryDefinitionCustomization"

TSharedRef<IDetailCustomization> FBBMInventoryDefinitionCustomization::MakeInstance()
{
	return MakeShared<FBBMInventoryDefinitionCustomization>();
}

FBBMInventoryDefinitionCustomization::~FBBMInventoryDefinitionCustomization()
{
	// 등록된 엔진 오브젝트 프로퍼티 변경 델리게이트 안전하게 해제
	if (OnPropertyChangedHandle.IsValid() == true)
	{
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OnPropertyChangedHandle);
		OnPropertyChangedHandle.Reset();
	}
}

void FBBMInventoryDefinitionCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// 대상 UBBMInventoryDefinition 인스턴스 획득
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

	if (CustomizedObjects.Num() == 1 && CustomizedObjects[0].IsValid() == true)
	{
		TargetDefinition = Cast<UBBMInventoryDefinition>(CustomizedObjects[0].Get());
	}

	// 에셋 프로퍼티 실시간 변경 감지를 위한 델리게이트 등록
	if (OnPropertyChangedHandle.IsValid() == false)
	{
		OnPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(
			this, &FBBMInventoryDefinitionCustomization::OnObjectPropertyChanged);
	}

	// 배낭 프리뷰 전용 최상위 카테고리 구성 (시각화 전용)
	IDetailCategoryBuilder& PreviewCategory = DetailBuilder.EditCategory(
		"BBM Backpack Preview",
		LOCTEXT("BackpackPreviewCategory", "BBM 배낭 2D 시각화 프리뷰"),
		ECategoryPriority::Important
	);

	PreviewCategory.AddCustomRow(LOCTEXT("BackpackPreviewRow", "배낭 2D 프리뷰"))
	.WholeRowContent()
	[
		SNew(SVerticalBox)
		// 안내 문구 및 요약 정보
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 4.0f, 0.0f, 6.0f)
		[
			SAssignNew(SummaryTextBlock, STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
			.ColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f)))
		]
		// 범례(Legend) 표시
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			// 일반 타일 범례
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(12.0f)
					.HeightOverride(12.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(FLinearColor(0.18f, 0.22f, 0.28f, 1.0f))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Text(LOCTEXT("LegendNormal", "일반 타일"))
				]
			]
			// 막힌 타일 범례
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(12.0f)
					.HeightOverride(12.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(FLinearColor(0.12f, 0.12f, 0.12f, 0.9f))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Text(LOCTEXT("LegendBlocked", "막힌 타일 (X)"))
				]
			]
			// 슬롯 강화 범례
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(12.0f)
					.HeightOverride(12.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(FLinearColor(0.75f, 0.40f, 0.08f, 1.0f))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Text(LOCTEXT("LegendEnhanced", "슬롯 강화 (+N)"))
				]
			]
		]
		// 메인 2D 그리드 패널 (읽기 전용 시각화)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(10.0f)
			[
				SAssignNew(GridContainer, SGridPanel)
			]
		]
	];

	// 초기 UI 렌더링
	RefreshGridWidget();
}

UBBMInventoryDefinition* FBBMInventoryDefinitionCustomization::GetTargetDefinition() const
{
	return TargetDefinition.Get();
}

void FBBMInventoryDefinitionCustomization::OnObjectPropertyChanged(UObject* InObject, FPropertyChangedEvent& InPropertyChangedEvent)
{
	if (TargetDefinition.IsValid() == false || InObject == nullptr)
	{
		return;
	}

	// 현재 커스터마이징 중인 UBBMInventoryDefinition 인스턴스의 프로퍼티 변경 시 그리드 실시간 최신화
	if (InObject == TargetDefinition.Get())
	{
		RefreshGridWidget();
	}
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
	const int32 Height = FMath::Max(1, FMath::DivideAndRoundUp(TotalCapacity, Width));

	int32 BlockedCount = 0;
	int32 EnhancedCount = 0;

	// 사전 통계 집계
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FIntPoint Coord(X, Y);
			const int32 SlotIndex = TargetDef->CoordToSlotIndex(Coord);
			if (SlotIndex < 0 || SlotIndex >= TotalCapacity)
			{
				continue;
			}

			if (TargetDef->IsTileBlocked(Coord) == true)
			{
				++BlockedCount;
			}
			else if (const FBBMSlotEffectSpecList* SpecList = TargetDef->FindSlotEnhancements(Coord))
			{
				if (SpecList->Effects.Num() > 0)
				{
					++EnhancedCount;
				}
			}
		}
	}

	// 상단 요약 텍스트 갱신
	if (SummaryTextBlock.IsValid() == true)
	{
		SummaryTextBlock->SetText(FText::Format(
			LOCTEXT("BackpackSummaryFormat", "배낭 그리드: {0} x {1} (총 {2}칸) | 사용 가능: {3}칸 | 막힌 타일: {4}칸 | 강화 슬롯: {5}칸"),
			FText::AsNumber(Width),
			FText::AsNumber(Height),
			FText::AsNumber(TotalCapacity),
			FText::AsNumber(TotalCapacity - BlockedCount),
			FText::AsNumber(BlockedCount),
			FText::AsNumber(EnhancedCount)
		));
	}

	constexpr float TileSize = 34.0f;

	for (int32 Row = 0; Row < Height; ++Row)
	{
		for (int32 Col = 0; Col < Width; ++Col)
		{
			const FIntPoint Coord(Col, Row);
			const int32 SlotIndex = TargetDef->CoordToSlotIndex(Coord);
			const bool bOutOfRange = (SlotIndex < 0 || SlotIndex >= TotalCapacity);

			if (bOutOfRange == true)
			{
				// 유효 슬롯 범위를 벗어난 칸은 흐릿한 비활성 칸으로 렌더링
				GridContainer->AddSlot(Col, Row)
				.Padding(1.5f)
				[
					SNew(SBox)
					.WidthOverride(TileSize)
					.HeightOverride(TileSize)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 0.4f))
					]
				];
				continue;
			}

			const bool bIsBlocked = TargetDef->IsTileBlocked(Coord);
			const FBBMSlotEffectSpecList* SpecList = TargetDef->FindSlotEnhancements(Coord);
			const bool bHasEnhancement = (SpecList != nullptr && SpecList->Effects.Num() > 0);

			FLinearColor BgColor;
			FLinearColor BorderColor = FLinearColor(0.06f, 0.06f, 0.06f, 1.0f);
			FText TileLabel;
			FText TooltipText;

			if (bIsBlocked == true)
			{
				// 막힌 타일
				BgColor = FLinearColor(0.12f, 0.12f, 0.12f, 0.9f);
				TileLabel = FText::FromString(TEXT("X"));
				TooltipText = FText::Format(
					LOCTEXT("BlockedTileTooltip", "좌표 ({0}, {1}) | 슬롯 #{2}\n사용 불가하게 막힌 타일 (BlockedTileShape)"),
					FText::AsNumber(Col), FText::AsNumber(Row), FText::AsNumber(SlotIndex));
			}
			else if (bHasEnhancement == true)
			{
				// 강화 타일
				BgColor = FLinearColor(0.70f, 0.35f, 0.06f, 1.0f);
				BorderColor = FLinearColor(0.95f, 0.55f, 0.15f, 1.0f);
				TileLabel = FText::Format(LOCTEXT("EnhancedTileLabel", "+{0}"), FText::AsNumber(SpecList->Effects.Num()));

				FString DetailsString = FString::Printf(TEXT("좌표 (%d, %d) | 슬롯 #%d\n슬롯 강화 효과 (%d개):"), Col, Row, SlotIndex, SpecList->Effects.Num());
				for (int32 Idx = 0; Idx < SpecList->Effects.Num(); ++Idx)
				{
					const FDISEnhancementEffectSpec& Spec = SpecList->Effects[Idx];
					const FString ClassName = (Spec.EffectClass != nullptr) ? Spec.EffectClass->GetName() : TEXT("None");
					DetailsString += FString::Printf(TEXT("\n  • [%d] %s (강도: %.1f)"), Idx + 1, *ClassName, Spec.OverallMagnitude);
				}
				TooltipText = FText::FromString(DetailsString);
			}
			else
			{
				// 일반 사용 가능 타일
				BgColor = FLinearColor(0.16f, 0.20f, 0.25f, 1.0f);
				TileLabel = FText::Format(LOCTEXT("NormalTileLabel", "{0},{1}"), FText::AsNumber(Col), FText::AsNumber(Row));
				TooltipText = FText::Format(
					LOCTEXT("NormalTileTooltip", "좌표 ({0}, {1}) | 슬롯 #{2}\n사용 가능한 기본 타일"),
					FText::AsNumber(Col), FText::AsNumber(Row), FText::AsNumber(SlotIndex));
			}

			// 순수 시각적 타일 렌더링 (버튼 없이 SBorder + STextBlock 구성)
			GridContainer->AddSlot(Col, Row)
			.Padding(1.5f)
			[
				SNew(SBox)
				.WidthOverride(TileSize)
				.HeightOverride(TileSize)
				.ToolTipText(TooltipText)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(BorderColor)
					.Padding(1.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(BgColor)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font(IDetailLayoutBuilder::GetDetailFont())
							.ColorAndOpacity(bIsBlocked ? FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f, 1.0f)) : FSlateColor(FLinearColor::White))
							.Text(TileLabel)
						]
					]
				]
			];
		}
	}
}

#undef LOCTEXT_NAMESPACE
