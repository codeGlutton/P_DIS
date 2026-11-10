/*****************************************************************//**
 * @file   BBMBeatSequenceCustomization.cpp
 * @brief  FBBMBeatSequence 구조체의 디테일 패널 3단 타임라인 시각화 커스터마이제이션 구현
 * @date   2026-11-01
 *********************************************************************/

#include "Customizations/BBMBeatSequenceCustomization.h"
#include "Item/BBMBeatTypes.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SToolTip.h"

#define LOCTEXT_NAMESPACE "FBBMBeatSequenceCustomization"

TSharedRef<IPropertyTypeCustomization> FBBMBeatSequenceCustomization::MakeInstance()
{
	return MakeShared<FBBMBeatSequenceCustomization>();
}

void FBBMBeatSequenceCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> InPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = InPropertyHandle;
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	HeaderRow
	.NameContent()
	[
		InPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(260.0f)
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(this, &FBBMBeatSequenceCustomization::GetSummaryText)
	];
}

void FBBMBeatSequenceCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> InPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = InPropertyHandle;
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	// 프로퍼티 변경 감지 시 상단 타임라인 위젯 부분 리프레시 바인딩 (디테일 뷰 전체 재생성 방지)
	TWeakPtr<FBBMBeatSequenceCustomization> WeakThis = StaticCastSharedRef<FBBMBeatSequenceCustomization>(AsShared());

	auto RefreshCallback = [WeakThis]()
	{
		if (TSharedPtr<FBBMBeatSequenceCustomization> PinnedThis = WeakThis.Pin())
		{
			PinnedThis->RefreshTimelineWidget();
		}
	};

	InPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda(RefreshCallback));
	InPropertyHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateLambda(RefreshCallback));

	// 1. 상단: 3단 비트 타임라인 시각화 커스텀 UI
	ChildBuilder.AddCustomRow(LOCTEXT("TimelineVisualizerFilter", "비트 타임라인 시각화"))
	.WholeRowContent()
	[
		SAssignNew(TimelineBox, SBox)
		.Padding(FMargin(0.0f, 4.0f, 0.0f, 8.0f))
		[
			BuildTimelineWidget()
		]
	];

	// 2. 하단: Beats 및 EffectRanges 기본 멤버 설정창 노출
	uint32 NumChildren = 0;
	InPropertyHandle->GetNumChildren(NumChildren);

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = InPropertyHandle->GetChildHandle(ChildIndex);
		if (ChildHandle.IsValid() == true)
		{
			ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
		}
	}
}

void FBBMBeatSequenceCustomization::RefreshTimelineWidget()
{
	if (TimelineBox.IsValid() == true)
	{
		TimelineBox->SetContent(BuildTimelineWidget());
	}
}

TSharedRef<SWidget> FBBMBeatSequenceCustomization::BuildTimelineWidget()
{
	const FBBMBeatSequence* Sequence = GetBeatSequence();

	TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

	// 1. Track 1: EffectRanges 구간 표기 트랙 (채움 솔리드 바)
	Container->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 4.0f)
	[
		BuildEffectRangesTrack(Sequence)
	];

	// 2. Track 2: AnimLayerRanges 구간 표기 트랙 (아웃라인 보더 바)
	Container->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 4.0f)
	[
		BuildAnimLayerRangesTrack(Sequence)
	];

	// 3. 비트 번호 인디케이터 (1 ~ 8)
	TSharedRef<SHorizontalBox> BeatNumbersBox = SNew(SHorizontalBox);
	for (int32 BeatIndex = 0; BeatIndex < 8; ++BeatIndex)
	{
		const bool bIsPairBorder = (BeatIndex % 2 == 1);
		const bool bIsAssignedBeat = (Sequence != nullptr && Sequence->Beats.IsValidIndex(BeatIndex));
		const FLinearColor BgColor = bIsAssignedBeat ? FLinearColor(0.08f, 0.08f, 0.08f, 1.0f) : FLinearColor(0.03f, 0.03f, 0.03f, 0.6f);
		const FLinearColor TextColor = bIsAssignedBeat ? FLinearColor(0.75f, 0.75f, 0.75f, 1.0f) : FLinearColor(0.35f, 0.35f, 0.35f, 1.0f);
		const FText BeatText = bIsAssignedBeat ?
			FText::Format(LOCTEXT("BeatNumberFormat", "비트 {0}"), FText::AsNumber(BeatIndex + 1)) :
			FText::Format(LOCTEXT("BeatNumberUnusedFormat", "비트 {0} (-)"), FText::AsNumber(BeatIndex + 1));

		BeatNumbersBox->AddSlot()
		.FillWidth(1.0f)
		.Padding(FMargin(1.0f, 0.0f, bIsPairBorder ? 4.0f : 1.0f, 0.0f))
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(BgColor)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 2.0f))
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
				.ColorAndOpacity(FSlateColor(TextColor))
				.Text(BeatText)
			]
		];
	}

	Container->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 4.0f)
	[
		BeatNumbersBox
	];

	// 4. Track 3: 8비트 로직 Actions 가로 바
	Container->AddSlot()
	.AutoHeight()
	.Padding(2.0f, 2.0f, 0.0f, 1.0f)
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f)))
		.Text(LOCTEXT("ActionsTrackTitle", "[ 로직 액션 (Logic Actions) ]"))
	];

	Container->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 4.0f)
	[
		BuildActionsTrack(Sequence)
	];

	// 5. Track 4: 8비트 Animations 가로 바
	Container->AddSlot()
	.AutoHeight()
	.Padding(2.0f, 2.0f, 0.0f, 1.0f)
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f)))
		.Text(LOCTEXT("AnimationsTrackTitle", "[ 애니메이션 연출 (Animations: 4-Slot Windows) ]"))
	];

	Container->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 6.0f)
	[
		BuildAnimationsTrack(Sequence)
	];

	// 5. 범례(Legend) 위젯
	Container->AddSlot()
	.AutoHeight()
	[
		BuildLegendWidget()
	];

	return SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 1.0f))
		.Padding(FMargin(6.0f))
		[
			Container
		];
}

TSharedRef<SWidget> FBBMBeatSequenceCustomization::BuildEffectRangesTrack(const FBBMBeatSequence* Sequence)
{
	TSharedRef<SVerticalBox> RangesContainer = SNew(SVerticalBox);

	// 타이틀 라벨
	RangesContainer->AddSlot()
	.AutoHeight()
	.Padding(2.0f, 0.0f, 0.0f, 2.0f)
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f)))
		.Text(LOCTEXT("EffectRangesTrackTitle", "[ 효과 지속 구간 (Effect Ranges) ]"))
	];

	if (Sequence == nullptr || Sequence->EffectRanges.IsEmpty() == true)
	{
		RangesContainer->AddSlot()
		.AutoHeight()
		.Padding(FMargin(2.0f, 2.0f))
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f)))
			.Text(LOCTEXT("NoEffectRangesText", "(설정된 비트 구간 효과 없음)"))
		];
		return RangesContainer;
	}

	// Range별 대표 색상 팔레트
	const FLinearColor RangeColors[] = {
		FLinearColor(0.9f, 0.7f, 0.2f, 1.0f), // 골드
		FLinearColor(0.7f, 0.3f, 0.9f, 1.0f), // 퍼플
		FLinearColor(0.2f, 0.8f, 0.8f, 1.0f), // 시안
		FLinearColor(0.9f, 0.4f, 0.3f, 1.0f)  // 코랄
	};

	for (int32 RangeIndex = 0; RangeIndex < Sequence->EffectRanges.Num(); ++RangeIndex)
	{
		const FBBMBeatEffectRange& Range = Sequence->EffectRanges[RangeIndex];
		const int32 Start = FMath::Clamp(Range.StartBeat, 0, 8);
		const int32 End = FMath::Clamp(Range.EndBeat, 0, 8);

		TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox);

		// Start 이전 공백
		if (Start > 0)
		{
			RowBox->AddSlot()
			.FillWidth(static_cast<float>(Start))
			[
				SNullWidget::NullWidget
			];
		}

		// [Start, End) 활성 구간 브래킷 바
		if (End > Start)
		{
			const FLinearColor CurrentColor = RangeColors[RangeIndex % UE_ARRAY_COUNT(RangeColors)];
			const FString PrimaryEffectName = (Range.GrantedEffects.IsEmpty() == false && Range.GrantedEffects[0].EffectClass.IsNull() == false) ?
				Range.GrantedEffects[0].EffectClass.GetAssetName() : FString::Printf(TEXT("EffectRange %d"), RangeIndex + 1);
			const FString RangeLabel = FString::Printf(TEXT("◀ %s [%d ~ %d) ▶"), *PrimaryEffectName, Start + 1, End);

			// 툴팁 텍스트 구성
			FString ToolTipString = FString::Printf(TEXT("비트 구간: %d ~ %d (총 %d비트 유지)\n부여 GameplayEffect: %d개"),
				Start + 1, End, End - Start, Range.GrantedEffects.Num());
			for (const FBBMItemGameplayEffectConfig& EffectConfig : Range.GrantedEffects)
			{
				ToolTipString += FString::Printf(TEXT("\n - %s (SetByCaller: %d개)"),
					*EffectConfig.EffectClass.GetAssetName(), EffectConfig.SetByCallerMagnitudes.Num());
			}

			RowBox->AddSlot()
			.FillWidth(static_cast<float>(End - Start))
			.Padding(FMargin(1.0f, 1.0f))
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(CurrentColor * 0.45f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.ToolTipText(FText::FromString(ToolTipString))
				.Padding(FMargin(4.0f, 2.0f))
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFontBold())
					.ColorAndOpacity(FSlateColor(CurrentColor))
					.Text(FText::FromString(RangeLabel))
				]
			];
		}

		// End 이후 공백
		if (End < 8)
		{
			RowBox->AddSlot()
			.FillWidth(static_cast<float>(8 - End))
			[
				SNullWidget::NullWidget
			];
		}

		RangesContainer->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 1.0f)
		[
			RowBox
		];
	}

	return RangesContainer;
}

TSharedRef<SWidget> FBBMBeatSequenceCustomization::BuildAnimLayerRangesTrack(const FBBMBeatSequence* Sequence)
{
	TSharedRef<SVerticalBox> RangesContainer = SNew(SVerticalBox);

	// 타이틀 라벨
	RangesContainer->AddSlot()
	.AutoHeight()
	.Padding(2.0f, 2.0f, 0.0f, 2.0f)
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f)))
		.Text(LOCTEXT("AnimLayerRangesTrackTitle", "[ 애님 레이어 지속 구간 (Anim Layer Ranges) ]"))
	];

	if (Sequence == nullptr || Sequence->AnimLayerRanges.IsEmpty() == true)
	{
		RangesContainer->AddSlot()
		.AutoHeight()
		.Padding(FMargin(2.0f, 2.0f))
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f)))
			.Text(LOCTEXT("NoAnimLayerRangesText", "(설정된 비트 구간 애님 레이어 없음)"))
		];
		return RangesContainer;
	}

	// AnimLayer Range 전용 쿨톤 팔레트 (EffectRange의 웜/원색 톤과 대비)
	const FLinearColor LayerRangeColors[] = {
		FLinearColor(0.12f, 0.85f, 0.70f, 1.0f), // 틸 / 민트
		FLinearColor(0.85f, 0.35f, 0.85f, 1.0f), // 마젠타 / 오키드
		FLinearColor(0.35f, 0.65f, 0.95f, 1.0f), // 세룰리안 블루
		FLinearColor(0.95f, 0.55f, 0.20f, 1.0f)  // 앰버 오렌지
	};

	for (int32 RangeIndex = 0; RangeIndex < Sequence->AnimLayerRanges.Num(); ++RangeIndex)
	{
		const FBBMBeatAnimLayerRange& Range = Sequence->AnimLayerRanges[RangeIndex];
		const int32 Start = FMath::Clamp(Range.StartBeat, 0, 8);
		const int32 End = FMath::Clamp(Range.EndBeat, 0, 8);

		TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox);

		// Start 이전 공백
		if (Start > 0)
		{
			RowBox->AddSlot()
			.FillWidth(static_cast<float>(Start))
			[
				SNullWidget::NullWidget
			];
		}

		// [Start, End) 활성 구간 아웃라인 바
		if (End > Start)
		{
			const FLinearColor CurrentColor = LayerRangeColors[RangeIndex % UE_ARRAY_COUNT(LayerRangeColors)];

			// 식별 태그 문자열
			const FString TagName = Range.AnimLayerTag.IsValid() ? Range.AnimLayerTag.ToString() : TEXT("None");

			// EffectRange(◀ ▶)와 구분되는 다이아몬드(◆ ◆) 기호 사용
			const FString RangeLabel = FString::Printf(TEXT("◆ %s [%d ~ %d) ◆"), *TagName, Start + 1, End);

			// 상세 툴팁 텍스트 구성
			const FString ToolTipString = FString::Printf(TEXT("[애님 레이어 지속 구간] 비트: %d ~ %d (총 %d비트 유지)\n링크 AnimLayer 태그: %s"),
				Start + 1, End, End - Start, *TagName);

			// 외곽 테두리(Outlined Double Border) 스타일 적용: EffectRange(단색 면 채움)와 시각적으로 확연히 구분됨
			RowBox->AddSlot()
			.FillWidth(static_cast<float>(End - Start))
			.Padding(FMargin(1.0f, 1.0f))
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(CurrentColor * 0.9f) // 밝고 뚜렷한 외곽 보더 라인
				.Padding(FMargin(1.5f)) // 외곽선 두께
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(CurrentColor * 0.20f) // 어두운 내부 투명 배경
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.ToolTipText(FText::FromString(ToolTipString))
					.Padding(FMargin(4.0f, 2.0f))
					[
						SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFontBold())
						.ColorAndOpacity(FSlateColor(CurrentColor))
						.Text(FText::FromString(RangeLabel))
					]
				]
			];
		}

		// End 이후 공백
		if (End < 8)
		{
			RowBox->AddSlot()
			.FillWidth(static_cast<float>(8 - End))
			[
				SNullWidget::NullWidget
			];
		}

		RangesContainer->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 1.0f)
		[
			RowBox
		];
	}

	return RangesContainer;
}

TSharedRef<SWidget> FBBMBeatSequenceCustomization::BuildActionsTrack(const FBBMBeatSequence* Sequence)
{
	TSharedRef<SHorizontalBox> ActionsBox = SNew(SHorizontalBox);

	for (int32 BeatIndex = 0; BeatIndex < 8; ++BeatIndex)
	{
		const bool bIsPairBorder = (BeatIndex % 2 == 1);
		const bool bHasBeat = (Sequence != nullptr && Sequence->Beats.IsValidIndex(BeatIndex));
		const FBBMBeatDefinition* BeatDef = bHasBeat ? &Sequence->Beats[BeatIndex] : nullptr;
		const bool bHasActions = (BeatDef != nullptr && BeatDef->Actions.IsEmpty() == false);

		if (bHasActions == true)
		{
			// 실제 전투 로직 실행 순서(EBBMActionCategory: 선이동 -> 선버프 -> 방어 -> 데미지 -> 후버프 -> 후이동)로 Stable Sort
			struct FSortedActionItem
			{
				int32 OriginalIndex;
				const FBBMBeatAction* ActionPtr;
				EBBMActionCategory Category;
			};

			TArray<FSortedActionItem> SortedActions;
			SortedActions.Reserve(BeatDef->Actions.Num());

			for (int32 ActionIndex = 0; ActionIndex < BeatDef->Actions.Num(); ++ActionIndex)
			{
				const FBBMBeatAction* ActionPtr = BeatDef->Actions[ActionIndex].GetPtr<FBBMBeatAction>();
				const EBBMActionCategory Category = ActionPtr ? ActionPtr->ActionCategory : EBBMActionCategory::Damage;
				SortedActions.Add({ ActionIndex, ActionPtr, Category });
			}

			// 카테고리 순 오름차순 정렬 (동일 카테고리는 OriginalIndex 순서 유지)
			SortedActions.StableSort([](const FSortedActionItem& A, const FSortedActionItem& B)
			{
				return static_cast<uint8>(A.Category) < static_cast<uint8>(B.Category);
			});

			// 해당 비트 내 정렬된 Action들을 세부 분할하여 렌더링
			TSharedRef<SHorizontalBox> BeatSubActionsBox = SNew(SHorizontalBox);
			FString ToolTipString = FString::Printf(TEXT("[비트 %d 로직 실행 순서: %d개 액션]"), BeatIndex + 1, SortedActions.Num());

			for (int32 DisplayIndex = 0; DisplayIndex < SortedActions.Num(); ++DisplayIndex)
			{
				const FSortedActionItem& Item = SortedActions[DisplayIndex];
				const EBBMActionCategory Category = Item.Category;

				FLinearColor CategoryColor = FLinearColor(0.85f, 0.25f, 0.25f); // Damage 기본 레드
				FString CategoryAbbrev = TEXT("DMG");

				switch (Category)
				{
				case EBBMActionCategory::PreMovement:
				case EBBMActionCategory::PostMovement:
					CategoryColor = FLinearColor(0.2f, 0.75f, 0.35f); // 그린
					CategoryAbbrev = TEXT("MOV");
					break;
				case EBBMActionCategory::Defense:
					CategoryColor = FLinearColor(0.25f, 0.5f, 0.9f); // 블루
					CategoryAbbrev = TEXT("DEF");
					break;
				case EBBMActionCategory::PreBuffDebuff:
				case EBBMActionCategory::PostBuffDebuff:
					CategoryColor = FLinearColor(0.9f, 0.65f, 0.15f); // 앰버
					CategoryAbbrev = TEXT("BUF");
					break;
				default:
					break;
				}

				ToolTipString += FString::Printf(TEXT("\n %d단계. %s (원래 등록 인덱스: #%d)"),
					DisplayIndex + 1, *UEnum::GetValueAsString(Category), Item.OriginalIndex + 1);

				// 세부 액션 타입별 추가 수치 정보 표기
				if (const FBBMCombatAction* CombatAction = static_cast<const FBBMCombatAction*>(Item.ActionPtr))
				{
					ToolTipString += FString::Printf(TEXT(" [계수: %.1f, 기본값: %.1f]"), CombatAction->Coefficient, CombatAction->BaseValue);
				}
				else if (const FBBMMovementAction* MoveAction = static_cast<const FBBMMovementAction*>(Item.ActionPtr))
				{
					ToolTipString += FString::Printf(TEXT(" [이동: %d칸]"), MoveAction->StepCount);
				}
				else if (const FBBMArmorAction* ArmorAction = static_cast<const FBBMArmorAction*>(Item.ActionPtr))
				{
					ToolTipString += FString::Printf(TEXT(" [%s방어도: 계수 %.1f, 기본값: %.1f]"),
						ArmorAction->bIsTurnArmor ? TEXT("턴") : TEXT("지속"),
						ArmorAction->Coefficient, ArmorAction->BaseValue);
				}

				BeatSubActionsBox->AddSlot()
				.FillWidth(1.0f)
				.Padding(FMargin(0.5f, 0.0f))
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(CategoryColor)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFontBold())
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
						.Text(FText::FromString(CategoryAbbrev))
					]
				];
			}

			ActionsBox->AddSlot()
			.FillWidth(1.0f)
			.Padding(FMargin(1.0f, 0.0f, bIsPairBorder ? 4.0f : 1.0f, 0.0f))
			[
				SNew(SBox)
				.HeightOverride(24.0f)
				.ToolTipText(FText::FromString(ToolTipString))
				[
					BeatSubActionsBox
				]
			];
		}
		else
		{
			// 액션 없는 빈 비트 슬롯
			ActionsBox->AddSlot()
			.FillWidth(1.0f)
			.Padding(FMargin(1.0f, 0.0f, bIsPairBorder ? 4.0f : 1.0f, 0.0f))
			[
				SNew(SBox)
				.HeightOverride(24.0f)
				.ToolTipText(FText::Format(LOCTEXT("EmptyBeatToolTip", "비트 {0}: 등록된 로직 액션 없음"), FText::AsNumber(BeatIndex + 1)))
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0.06f, 0.06f, 0.06f, 1.0f))
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.ColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.3f, 0.3f, 1.0f)))
						.Text(LOCTEXT("EmptyActionSlotText", "-"))
					]
				]
			];
		}
	}

	return ActionsBox;
}

TSharedRef<SWidget> FBBMBeatSequenceCustomization::BuildAnimationsTrack(const FBBMBeatSequence* Sequence)
{
	TSharedRef<SHorizontalBox> AnimationsBox = SNew(SHorizontalBox);

	for (int32 BeatIndex = 0; BeatIndex < 8; ++BeatIndex)
	{
		const bool bIsPairBorder = (BeatIndex % 2 == 1);
		const bool bHasBeat = (Sequence != nullptr && Sequence->Beats.IsValidIndex(BeatIndex));
		const FBBMBeatDefinition* BeatDef = bHasBeat ? &Sequence->Beats[BeatIndex] : nullptr;

		// 한 비트 내 4대 윈도우(0~25% 선이동, 25~50% 선방어, 50~75% 타격, 75~100% 후퇴) 4분할
		TSharedRef<SHorizontalBox> WindowsBox = SNew(SHorizontalBox);
		FString ToolTipString = FString::Printf(TEXT("[비트 %d 애니메이션 4대 윈도우]"), BeatIndex + 1);

		int32 ActiveWindowCount = 0;

		for (int32 WindowIndex = 0; WindowIndex < 4; ++WindowIndex)
		{
			const EBBMVisualWindow CurrentWindow = static_cast<EBBMVisualWindow>(WindowIndex);
			bool bWindowActive = false;
			FGameplayTag ActiveStateTag;

			if (BeatDef != nullptr)
			{
				for (const FBBMBeatAnimationConfig& AnimConfig : BeatDef->Animations)
				{
					if (AnimConfig.VisualWindow == CurrentWindow && AnimConfig.IsValid() == true)
					{
						bWindowActive = true;
						ActiveStateTag = AnimConfig.StateTag;
						ActiveWindowCount++;
						ToolTipString += FString::Printf(TEXT("\n W%d (%s): %s"),
							WindowIndex + 1, *UEnum::GetValueAsString(CurrentWindow), *ActiveStateTag.ToString());
						break;
					}
				}
			}

			// 활성화 시 밝은 시안/스카이블루, 비활성화 시 어두운 슬레이트 회색
			const FLinearColor WindowColor = bWindowActive ?
				FLinearColor(0.12f, 0.72f, 0.88f, 1.0f) :
				FLinearColor(0.06f, 0.06f, 0.06f, 1.0f);

			const FString WindowLabel = FString::Printf(TEXT("W%d"), WindowIndex + 1);

			WindowsBox->AddSlot()
			.FillWidth(1.0f)
			.Padding(FMargin(0.5f, 0.0f))
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(WindowColor)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ColorAndOpacity(bWindowActive ? FSlateColor(FLinearColor::Black) : FSlateColor(FLinearColor(0.25f, 0.25f, 0.25f, 1.0f)))
					.Text(FText::FromString(WindowLabel))
				]
			];
		}

		if (ActiveWindowCount == 0)
		{
			ToolTipString += TEXT("\n (등록된 애니메이션 연출 없음)");
		}

		AnimationsBox->AddSlot()
		.FillWidth(1.0f)
		.Padding(FMargin(1.0f, 0.0f, bIsPairBorder ? 4.0f : 1.0f, 0.0f))
		[
			SNew(SBox)
			.HeightOverride(18.0f)
			.ToolTipText(FText::FromString(ToolTipString))
			[
				WindowsBox
			]
		];
	}

	return AnimationsBox;
}

TSharedRef<SWidget> FBBMBeatSequenceCustomization::BuildLegendWidget()
{
	return SNew(SVerticalBox)
		// 1행: 상단 범위(Range) 트랙 2종 범례 (Solid vs Outline)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 3.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.7f, 0.2f)))
				.Text(LOCTEXT("LegendEffectRange", "◀▶ Effect Range (Solid)"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.12f, 0.85f, 0.70f)))
				.Text(LOCTEXT("LegendAnimLayerRange", "◆◆ Anim Layer Range (Outline)"))
			]
		]
		// 2행: 비트별 로직 액션 및 4-Slot 애니메이션 연출 범례
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.25f, 0.25f)))
				.Text(LOCTEXT("LegendDamage", "■ Damage"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.5f, 0.9f)))
				.Text(LOCTEXT("LegendDefense", "■ Defense"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.75f, 0.35f)))
				.Text(LOCTEXT("LegendMovement", "■ Movement"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.65f, 0.15f)))
				.Text(LOCTEXT("LegendBuff", "■ Buff/Debuff"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.15f, 0.7f, 0.85f)))
				.Text(LOCTEXT("LegendAnimation", "■ Animation (4-Slot)"))
			]
		];
}

const FBBMBeatSequence* FBBMBeatSequenceCustomization::GetBeatSequence() const
{
	if (StructPropertyHandle.IsValid() == false)
	{
		return nullptr;
	}

	TArray<void*> RawData;
	StructPropertyHandle->AccessRawData(RawData);

	if (RawData.Num() > 0 && RawData[0] != nullptr)
	{
		return reinterpret_cast<const FBBMBeatSequence*>(RawData[0]);
	}

	return nullptr;
}

FText FBBMBeatSequenceCustomization::GetSummaryText() const
{
	const FBBMBeatSequence* Sequence = GetBeatSequence();
	if (Sequence == nullptr)
	{
		return LOCTEXT("NullSequenceSummary", "시퀀스 데이터 없음");
	}

	const int32 BeatCount = Sequence->Beats.Num();
	const int32 RangeCount = Sequence->EffectRanges.Num();
	const int32 AnimLayerCount = Sequence->AnimLayerRanges.Num();
	const bool bValid = Sequence->IsValid();

	FText StatusText;
	if (bValid == false)
	{
		StatusText = LOCTEXT("InvalidBeatText", "규격 초과(>8)");
	}
	else if (BeatCount == 8)
	{
		StatusText = LOCTEXT("FullEightBeatsText", "완전 8비트");
	}
	else
	{
		StatusText = FText::Format(LOCTEXT("PartialBeatsText", "{0}비트 가변"), FText::AsNumber(BeatCount));
	}

	return FText::Format(
		LOCTEXT("BeatSequenceSummaryFormat", "{0}개 비트 ({1}) | {2}개 EffectRange | {3}개 AnimLayerRange"),
		FText::AsNumber(BeatCount),
		StatusText,
		FText::AsNumber(RangeCount),
		FText::AsNumber(AnimLayerCount)
	);
}

#undef LOCTEXT_NAMESPACE

