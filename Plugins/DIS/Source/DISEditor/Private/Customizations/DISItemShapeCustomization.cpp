#include "Customizations/DISItemShapeCustomization.h"
#include "Item/DISItemTypes.h"
#include "Global/DISGlobalSettings.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FDISItemShapeCustomization"

TSharedRef<IPropertyTypeCustomization> FDISItemShapeCustomization::MakeInstance()
{
	return MakeShared<FDISItemShapeCustomization>();
}

void FDISItemShapeCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = PropertyHandle;

	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(320.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(this, &FDISItemShapeCustomization::GetSummaryText)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(12.0f, 0.0f, 4.0f, 0.0f)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ToolTipText(LOCTEXT("TrimTooltip", "상/하/좌/우 불필요한 빈 여백을 지우고 전체 타일을 좌상단으로 밀착 정렬합니다."))
			.OnClicked(this, &FDISItemShapeCustomization::OnNormalizeClicked)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 1.0f, 1.0f)))
				.Text(LOCTEXT("TrimButton", "Trim"))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ToolTipText(LOCTEXT("ResetTooltip", "기본 1x1 단일 타일 점유로 초기화합니다."))
			.OnClicked(this, &FDISItemShapeCustomization::OnResetClicked)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.45f, 0.45f, 1.0f)))
				.Text(LOCTEXT("ResetButton", "Reset"))
			]
		]
	];
}

void FDISItemShapeCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = PropertyHandle;

	ChildBuilder.AddCustomRow(LOCTEXT("ItemShapeTileGridRow", "Item Shape Grid"))
	.WholeRowContent()
	[
		SNew(SBox)
		.Padding(FMargin(0.0f, 4.0f, 0.0f, 4.0f))
		[
			SNew(SHorizontalBox)
			// [좌측] 2D 타일 그리드
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Top)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(FMargin(6.0f))
				[
					SAssignNew(GridContainer, SGridPanel)
				]
			]
			// [우측] Points 좌표 읽기 전용 목록
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(10.0f, 0.0f, 0.0f, 0.0f)
			.VAlign(VAlign_Top)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(FMargin(6.0f))
				[
					SAssignNew(PointsListContainer, SVerticalBox)
				]
			]
		]
	];

	RefreshGridWidget();
	RefreshPointsListWidget();
}

TArray<FIntPoint> FDISItemShapeCustomization::GetPointsFromHandle() const
{
	if (StructPropertyHandle.IsValid() == false)
	{
		return TArray<FIntPoint>();
	}

	void* StructData = nullptr;
	if (StructPropertyHandle->GetValueData(StructData) == FPropertyAccess::Success && StructData != nullptr)
	{
		const FDISItemShape* Shape = static_cast<const FDISItemShape*>(StructData);
		return Shape->Points;
	}

	return TArray<FIntPoint>();
}

void FDISItemShapeCustomization::SetPointsToHandle(const TArray<FIntPoint>& NewPoints)
{
	if (StructPropertyHandle.IsValid() == false)
	{
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("ModifyItemShapeTransaction", "Modify Item Shape"));
	StructPropertyHandle->NotifyPreChange();

	void* StructData = nullptr;
	if (StructPropertyHandle->GetValueData(StructData) == FPropertyAccess::Success && StructData != nullptr)
	{
		FDISItemShape* Shape = static_cast<FDISItemShape*>(StructData);
		Shape->Points = NewPoints;
	}

	StructPropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

FReply FDISItemShapeCustomization::OnTileClicked(int32 X, int32 Y)
{
	const UDISGlobalSettings* Settings = GetDefault<UDISGlobalSettings>();
	const FIntPoint MaxAllowed = (Settings != nullptr && Settings->MaxItemTileDimensions.X > 0 && Settings->MaxItemTileDimensions.Y > 0)
		? Settings->MaxItemTileDimensions
		: FIntPoint(8, 8);

	if (X >= MaxAllowed.X || Y >= MaxAllowed.Y)
	{
		return FReply::Handled();
	}

	TArray<FIntPoint> CurrentPoints = GetPointsFromHandle();
	const FIntPoint TargetCoord(X, Y);

	if (CurrentPoints.Contains(TargetCoord) == true)
	{
		CurrentPoints.Remove(TargetCoord);
	}
	else
	{
		CurrentPoints.Add(TargetCoord);
	}

	SetPointsToHandle(CurrentPoints);
	RefreshGridWidget();
	RefreshPointsListWidget();

	return FReply::Handled();
}

FReply FDISItemShapeCustomization::OnNormalizeClicked()
{
	TArray<FIntPoint> CurrentPoints = GetPointsFromHandle();
	if (CurrentPoints.IsEmpty() == true)
	{
		return FReply::Handled();
	}

	FDISItemShape Shape(CurrentPoints);
	// 상/하/좌/우 불필요한 빈 여백을 지우고 최소 좌표가 (0,0)에 오도록 밀착
	Shape.Normalize();

	SetPointsToHandle(Shape.Points);
	RefreshGridWidget();
	RefreshPointsListWidget();

	return FReply::Handled();
}

FReply FDISItemShapeCustomization::OnResetClicked()
{
	TArray<FIntPoint> NewPoints;
	NewPoints.Add(FIntPoint(0, 0));

	SetPointsToHandle(NewPoints);
	RefreshGridWidget();
	RefreshPointsListWidget();

	return FReply::Handled();
}

void FDISItemShapeCustomization::RefreshGridWidget()
{
	if (GridContainer.IsValid() == false)
	{
		return;
	}

	GridContainer->ClearChildren();

	const TArray<FIntPoint> CurrentPoints = GetPointsFromHandle();

	int32 MaxX = 0;
	int32 MaxY = 0;
	for (const FIntPoint& Pt : CurrentPoints)
	{
		MaxX = FMath::Max(MaxX, Pt.X);
		MaxY = FMath::Max(MaxY, Pt.Y);
	}

	const UDISGlobalSettings* Settings = GetDefault<UDISGlobalSettings>();
	const FIntPoint MaxAllowed = (Settings != nullptr && Settings->MaxItemTileDimensions.X > 0 && Settings->MaxItemTileDimensions.Y > 0)
		? Settings->MaxItemTileDimensions
		: FIntPoint(8, 8);

	// 최소 3x3 보장, 우측/하단 외곽 +1칸 여백 동적 확장, UDISGlobalSettings의 최대 타일 규격으로 상한 클램핑
	const int32 CanvasWidth = FMath::Clamp(FMath::Max(MaxX + 2, 3), 3, MaxAllowed.X);
	const int32 CanvasHeight = FMath::Clamp(FMath::Max(MaxY + 2, 3), 3, MaxAllowed.Y);

	for (int32 Y = 0; Y < CanvasHeight; ++Y)
	{
		for (int32 X = 0; X < CanvasWidth; ++X)
		{
			GridContainer->AddSlot(X, Y)
			.Padding(1.5f)
			[
				SNew(SBox)
				.WidthOverride(26.0f)
				.HeightOverride(26.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "Button")
					.ContentPadding(FMargin(0.0f))
					.ButtonColorAndOpacity(this, &FDISItemShapeCustomization::GetTileColor, X, Y)
					.ToolTipText(FText::Format(LOCTEXT("TileCoordTooltip", "타일 좌표: ({0}, {1})\n클릭하여 점유 상태를 토글합니다."), X, Y))
					.OnClicked(this, &FDISItemShapeCustomization::OnTileClicked, X, Y)
				]
			];
		}
	}
}

void FDISItemShapeCustomization::RefreshPointsListWidget()
{
	if (PointsListContainer.IsValid() == false)
	{
		return;
	}

	PointsListContainer->ClearChildren();

	const TArray<FIntPoint> CurrentPoints = GetPointsFromHandle();

	// 1. 헤더: "Points (N)" 라벨
	PointsListContainer->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 4.0f)
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFontBold())
		.Text(FText::Format(LOCTEXT("PointsListHeader", "Points ({0})"), CurrentPoints.Num()))
	];

	// 구분선
	PointsListContainer->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 4.0f)
	[
		SNew(SSeparator)
	];

	if (CurrentPoints.IsEmpty() == true)
	{
		PointsListContainer->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFontItalic())
			.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)))
			.Text(LOCTEXT("NoPointsText", "No occupied tiles."))
		];
		return;
	}

	// 2. 읽기 전용 좌표 목록 스크롤 박스
	TSharedRef<SScrollBox> ScrollBox = SNew(SScrollBox);

	for (int32 Index = 0; Index < CurrentPoints.Num(); ++Index)
	{
		const FIntPoint& Pt = CurrentPoints[Index];

		ScrollBox->AddSlot()
		.Padding(0.0f, 1.5f)
		[
			SNew(SHorizontalBox)
			// 인덱스 번호 라벨
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.55f, 0.55f, 1.0f)))
				.Text(FText::Format(LOCTEXT("IndexFormat", "[{0}]"), Index))
			]
			// 좌표 (X: N, Y: N) 표시
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.90f, 0.94f, 1.0f)))
				.Text(FText::Format(LOCTEXT("PointCoordFormat", "(X: {0}, Y: {1})"), Pt.X, Pt.Y))
			]
		];
	}

	PointsListContainer->AddSlot()
	.AutoHeight()
	[
		SNew(SBox)
		.MaxDesiredHeight(240.0f)
		[
			ScrollBox
		]
	];
}

FSlateColor FDISItemShapeCustomization::GetTileColor(int32 X, int32 Y) const
{
	const TArray<FIntPoint> CurrentPoints = GetPointsFromHandle();
	if (CurrentPoints.Contains(FIntPoint(X, Y)) == true)
	{
		// 점유 타일: 밝고 세련된 언리얼 에디터 셀렉션 블루
		return FSlateColor(FLinearColor(0.10f, 0.52f, 0.96f, 1.0f));
	}

	// 비점유 타일: 에디터 다크 배경과 확실하게 대비되는 밝은 슬레이트 스틸 그레이
	return FSlateColor(FLinearColor(0.28f, 0.30f, 0.35f, 1.0f));
}

FText FDISItemShapeCustomization::GetSummaryText() const
{
	const TArray<FIntPoint> CurrentPoints = GetPointsFromHandle();
	const UDISGlobalSettings* Settings = GetDefault<UDISGlobalSettings>();
	const FIntPoint MaxAllowed = (Settings != nullptr && Settings->MaxItemTileDimensions.X > 0 && Settings->MaxItemTileDimensions.Y > 0)
		? Settings->MaxItemTileDimensions
		: FIntPoint(8, 8);

	if (CurrentPoints.IsEmpty() == true)
	{
		return FText::Format(LOCTEXT("EmptyShapeSummary", "Empty (Max: {0}x{1})"), MaxAllowed.X, MaxAllowed.Y);
	}

	const FDISItemShape Shape(CurrentPoints);
	const FIntPoint Dimensions = Shape.GetBoundingDimensions();

	return FText::Format(
		LOCTEXT("ShapeSummaryFormat", "{0} tiles ({1}x{2} / Max {3}x{4})"),
		CurrentPoints.Num(),
		Dimensions.X,
		Dimensions.Y,
		MaxAllowed.X,
		MaxAllowed.Y
	);
}

#undef LOCTEXT_NAMESPACE
