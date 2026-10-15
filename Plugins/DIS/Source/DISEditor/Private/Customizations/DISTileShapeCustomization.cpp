#include "Customizations/DISTileShapeCustomization.h"
#include "Item/DISItemTypes.h"
#include "Inventory/DISInventoryDefinition.h"
#include "Global/DISGlobalSettings.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IPropertyUtilities.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FDISTileShapeCustomization"

TSharedRef<IPropertyTypeCustomization> FDISTileShapeCustomization::MakeInstance()
{
	return MakeShared<FDISTileShapeCustomization>();
}

FDISTileShapeCustomization::~FDISTileShapeCustomization()
{
	// 등록된 엔진 오브젝트 프로퍼티 변경 델리게이트 해제
	if (OnPropertyChangedHandle.IsValid() == true)
	{
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OnPropertyChangedHandle);
		OnPropertyChangedHandle.Reset();
	}
}

void FDISTileShapeCustomization::CustomizeHeader(
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
			.Text(this, &FDISTileShapeCustomization::GetSummaryText)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.Visibility(this, &FDISTileShapeCustomization::GetTrimButtonVisibility)
			.Padding(FMargin(12.0f, 0.0f, 4.0f, 0.0f))
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ToolTipText(LOCTEXT("TrimTooltip", "상/하/좌/우 불필요한 빈 여백을 지우고 전체 타일을 좌상단으로 밀착 정렬합니다."))
				.OnClicked(this, &FDISTileShapeCustomization::OnNormalizeClicked)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 1.0f, 1.0f)))
					.Text(LOCTEXT("TrimButton", "Trim"))
				]
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ToolTipText(LOCTEXT("ResetTooltip", "모든 타일 점유를 비우고 초기화합니다."))
			.OnClicked(this, &FDISTileShapeCustomization::OnResetClicked)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.4f, 0.4f, 1.0f)))
				.Text(LOCTEXT("ResetButton", "Reset"))
			]
		]
	];
}

void FDISTileShapeCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = PropertyHandle;
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	// 외부 UObject(예: UDISInventoryDefinition)의 프로퍼티 변경 시 그리드 실시간 최신화를 위한 델리게이트 등록
	if (OnPropertyChangedHandle.IsValid() == false)
	{
		OnPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(
			this, &FDISTileShapeCustomization::OnObjectPropertyChanged);
	}

	ChildBuilder.AddCustomRow(LOCTEXT("TileGridSearchString", "Tile Matrix Grid Shape"))
	.WholeRowContent()
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.0f)
		[
			SNew(SHorizontalBox)
			// 좌측: 2D 타일 매트릭스 그리드 컨테이너
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(GridContainer, SGridPanel)
			]
			// 구분선
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(16.0f, 0.0f)
			[
				SNew(SSeparator)
				.Orientation(Orient_Vertical)
			]
			// 우측: Points 읽기 전용 좌표 목록 및 조작 안내
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFontBold())
					.Text(LOCTEXT("PointsListHeader", "Occupied Coordinates (Points)"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(PointsListContainer, SVerticalBox)
				]
			]
		]
	];

	RefreshGridWidget();
	RefreshPointsListWidget();
}

TArray<FIntPoint> FDISTileShapeCustomization::GetPointsFromHandle() const
{
	if (StructPropertyHandle.IsValid() == false)
	{
		return TArray<FIntPoint>();
	}

	void* StructData = nullptr;
	if (StructPropertyHandle->GetValueData(StructData) == FPropertyAccess::Success && StructData != nullptr)
	{
		const FDISTileShape* Shape = static_cast<const FDISTileShape*>(StructData);
		return Shape->Points;
	}

	return TArray<FIntPoint>();
}

void FDISTileShapeCustomization::SetPointsToHandle(const TArray<FIntPoint>& NewPoints)
{
	if (StructPropertyHandle.IsValid() == false)
	{
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("ModifyTileShapeTransaction", "Modify Tile Shape"));
	StructPropertyHandle->NotifyPreChange();

	void* StructData = nullptr;
	if (StructPropertyHandle->GetValueData(StructData) == FPropertyAccess::Success && StructData != nullptr)
	{
		FDISTileShape* Shape = static_cast<FDISTileShape*>(StructData);
		Shape->Points = NewPoints;
	}

	StructPropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

FReply FDISTileShapeCustomization::OnTileClicked(int32 X, int32 Y)
{
	const FIntPoint MaxAllowed = GetMaxAllowedDimensions();

	if (X < 0 || X >= MaxAllowed.X || Y < 0 || Y >= MaxAllowed.Y)
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

FReply FDISTileShapeCustomization::OnNormalizeClicked()
{
	TArray<FIntPoint> CurrentPoints = GetPointsFromHandle();
	if (CurrentPoints.IsEmpty() == true)
	{
		return FReply::Handled();
	}

	FDISTileShape Shape(CurrentPoints);
	Shape.Normalize();

	SetPointsToHandle(Shape.Points);
	RefreshGridWidget();
	RefreshPointsListWidget();
	return FReply::Handled();
}

FReply FDISTileShapeCustomization::OnResetClicked()
{
	// 모든 점유 타일을 비우고 빈 상태로 초기화 (0,0 미포함)
	TArray<FIntPoint> EmptyPoints;

	SetPointsToHandle(EmptyPoints);
	RefreshGridWidget();
	RefreshPointsListWidget();
	return FReply::Handled();
}

void FDISTileShapeCustomization::OnObjectPropertyChanged(UObject* InObject, FPropertyChangedEvent& InPropertyChangedEvent)
{
	if (StructPropertyHandle.IsValid() == false || InObject == nullptr)
	{
		return;
	}

	// 현재 커스터마이제이션 대상 구조체를 소유한 Outer 객체에서 발생한 프로퍼티 변경인지 검증
	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);
	if (OuterObjects.Contains(InObject) == true)
	{
		// 슬레이트 타일 그리드 및 점유 목록 위젯을 즉시 리프레시
		RefreshGridWidget();
		RefreshPointsListWidget();
	}
}

void FDISTileShapeCustomization::RefreshGridWidget()
{
	if (GridContainer.IsValid() == false)
	{
		return;
	}

	GridContainer->ClearChildren();

	int32 CanvasWidth = 0;
	int32 CanvasHeight = 0;

	if (HasFixedGridDimensions() == true)
	{
		// 고정 그리드 규격 모드 (인벤토리 블록 타일 등: 고정된 크기 전체를 표시)
		const FIntPoint FixedDim = GetFixedGridDimensions();
		CanvasWidth = FixedDim.X;
		CanvasHeight = FixedDim.Y;
	}
	else
	{
		// 동적 그리드 모드 (아이템 형상 등: EditorFixedGridDimensions이 Zero인 경우)
		// 클릭한 모양의 최대 좌표를 기반으로 최소 3x3 보장 및 우측/하단 +1칸 여백 동적 확장
		const TArray<FIntPoint> CurrentPoints = GetPointsFromHandle();

		int32 MaxX = 0;
		int32 MaxY = 0;
		for (const FIntPoint& Pt : CurrentPoints)
		{
			MaxX = FMath::Max(MaxX, Pt.X);
			MaxY = FMath::Max(MaxY, Pt.Y);
		}

		const FIntPoint MaxAllowed = GetMaxAllowedDimensions();

		CanvasWidth = FMath::Clamp(FMath::Max(MaxX + 2, 3), 3, MaxAllowed.X);
		CanvasHeight = FMath::Clamp(FMath::Max(MaxY + 2, 3), 3, MaxAllowed.Y);
	}

	constexpr float TileSize = 24.0f;
	constexpr float TileMargin = 1.0f;

	for (int32 Row = 0; Row < CanvasHeight; ++Row)
	{
		for (int32 Col = 0; Col < CanvasWidth; ++Col)
		{
			const int32 X = Col;
			const int32 Y = Row;

			GridContainer->AddSlot(Col, Row)
			.Padding(TileMargin)
			[
				SNew(SBox)
				.WidthOverride(TileSize)
				.HeightOverride(TileSize)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "Button")
					.ContentPadding(0.0f)
					.ToolTipText(FText::Format(LOCTEXT("TileCoordTooltip", "Coord ({0}, {1}) 클릭하여 점유 토글"), X, Y))
					.ButtonColorAndOpacity(this, &FDISTileShapeCustomization::GetTileColor, X, Y)
					.OnClicked(this, &FDISTileShapeCustomization::OnTileClicked, X, Y)
				]
			];
		}
	}
}

void FDISTileShapeCustomization::RefreshPointsListWidget()
{
	if (PointsListContainer.IsValid() == false)
	{
		return;
	}

	PointsListContainer->ClearChildren();

	const TArray<FIntPoint> CurrentPoints = GetPointsFromHandle();
	if (CurrentPoints.IsEmpty() == true)
	{
		PointsListContainer->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFontItalic())
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text(LOCTEXT("NoPointsDesc", "No occupied tiles."))
		];
		return;
	}

	TSharedPtr<SScrollBox> ScrollBox = SNew(SScrollBox);

	for (int32 Index = 0; Index < CurrentPoints.Num(); ++Index)
	{
		const FIntPoint& Pt = CurrentPoints[Index];

		ScrollBox->AddSlot()
		.Padding(0.0f, 1.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text(FText::Format(LOCTEXT("IndexFormat", "[{0}]"), Index))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.8f, 0.4f, 1.0f)))
				.Text(FText::Format(LOCTEXT("PointCoordFormat", "({0}, {1})"), Pt.X, Pt.Y))
			]
		];
	}

	PointsListContainer->AddSlot()
	.AutoHeight()
	[
		SNew(SBox)
		.MaxDesiredHeight(240.0f)
		[
			ScrollBox.ToSharedRef()
		]
	];
}

FSlateColor FDISTileShapeCustomization::GetTileColor(int32 X, int32 Y) const
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

FText FDISTileShapeCustomization::GetSummaryText() const
{
	const TArray<FIntPoint> CurrentPoints = GetPointsFromHandle();
	const FIntPoint MaxAllowed = GetMaxAllowedDimensions();

	if (CurrentPoints.IsEmpty() == true)
	{
		return FText::Format(LOCTEXT("EmptyShapeSummary", "Empty (Max: {0}x{1})"), MaxAllowed.X, MaxAllowed.Y);
	}

	const FDISTileShape Shape(CurrentPoints);
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

EVisibility FDISTileShapeCustomization::GetTrimButtonVisibility() const
{
	// EditorFixedGridDimensions가 Zero가 아니라면(고정 그리드 모드) Trim 버튼 미표기
	return HasFixedGridDimensions() ? EVisibility::Collapsed : EVisibility::Visible;
}

bool FDISTileShapeCustomization::HasFixedGridDimensions() const
{
	const FIntPoint FixedDim = GetFixedGridDimensions();
	return (FixedDim.X > 0 && FixedDim.Y > 0);
}

FIntPoint FDISTileShapeCustomization::GetFixedGridDimensions() const
{
	// 1순위: FDISTileShape 내부의 EditorFixedGridDimensions 확인
	if (StructPropertyHandle.IsValid() == true)
	{
		void* StructData = nullptr;
		if (StructPropertyHandle->GetValueData(StructData) == FPropertyAccess::Success && StructData != nullptr)
		{
			const FDISTileShape* Shape = static_cast<const FDISTileShape*>(StructData);
#if WITH_EDITORONLY_DATA
			if (Shape->EditorFixedGridDimensions.X > 0 && Shape->EditorFixedGridDimensions.Y > 0)
			{
				return Shape->EditorFixedGridDimensions;
			}
#endif
		}
	}

	// 2순위: Outer UObject가 UDISInventoryDefinition인 경우 해당 규격(LineBreakTileCount, SlotIndexingDirection 등)으로부터 직접 산출
	if (StructPropertyHandle.IsValid() == true)
	{
		TArray<UObject*> OuterObjects;
		StructPropertyHandle->GetOuterObjects(OuterObjects);
		for (UObject* Outer : OuterObjects)
		{
			if (const UDISInventoryDefinition* InvenDef = Cast<UDISInventoryDefinition>(Outer))
			{
				if (InvenDef->LineBreakTileCount > 0 && InvenDef->MaxExpandableCapacity > 0)
				{
					const int32 PrimaryDim = InvenDef->LineBreakTileCount;
					const int32 SecondaryDim = FMath::DivideAndRoundUp(InvenDef->MaxExpandableCapacity, PrimaryDim);

					if (InvenDef->SlotIndexingDirection == EDISSlotIndexingDirection::Horizontal)
					{
						// 가로 우선: 한 행의 열 수 = PrimaryDim (X), 총 행 수 = SecondaryDim (Y)
						return FIntPoint(PrimaryDim, SecondaryDim);
					}
					else
					{
						// 세로 우선: 한 열의 행 수 = PrimaryDim (Y), 총 열 수 = SecondaryDim (X)
						return FIntPoint(SecondaryDim, PrimaryDim);
					}
				}
			}
		}
	}

	return FIntPoint::ZeroValue;
}

FIntPoint FDISTileShapeCustomization::GetMaxAllowedDimensions() const
{
	// 1순위: FDISTileShape 내부의 EditorFixedGridDimensions 확인
	const FIntPoint FixedDim = GetFixedGridDimensions();
	if (FixedDim.X > 0 && FixedDim.Y > 0)
	{
		return FixedDim;
	}

	// 2순위: 프로퍼티의 Outer UObject들을 순회하여 UDISInventoryDefinition인지 검사
	if (StructPropertyHandle.IsValid() == true)
	{
		TArray<UObject*> OuterObjects;
		StructPropertyHandle->GetOuterObjects(OuterObjects);
		for (UObject* Outer : OuterObjects)
		{
			if (const UDISInventoryDefinition* InvenDef = Cast<UDISInventoryDefinition>(Outer))
			{
				if (InvenDef->LineBreakTileCount > 0 && InvenDef->MaxExpandableCapacity > 0)
				{
					const int32 PrimaryDim = InvenDef->LineBreakTileCount;
					const int32 SecondaryDim = FMath::DivideAndRoundUp(InvenDef->MaxExpandableCapacity, PrimaryDim);

					if (InvenDef->SlotIndexingDirection == EDISSlotIndexingDirection::Horizontal)
					{
						// 가로 우선: 한 행의 열 수 = PrimaryDim (X), 총 행 수 = SecondaryDim (Y)
						return FIntPoint(PrimaryDim, SecondaryDim);
					}
					else
					{
						// 세로 우선: 한 열의 행 수 = PrimaryDim (Y), 총 열 수 = SecondaryDim (X)
						return FIntPoint(SecondaryDim, PrimaryDim);
					}
				}
			}
		}
	}

	// 3순위: 기본 글로벌 세팅
	const UDISGlobalSettings* Settings = GetDefault<UDISGlobalSettings>();
	if (Settings != nullptr && Settings->MaxItemTileDimensions.X > 0 && Settings->MaxItemTileDimensions.Y > 0)
	{
		return Settings->MaxItemTileDimensions;
	}

	return FIntPoint(8, 8);
}

#undef LOCTEXT_NAMESPACE
