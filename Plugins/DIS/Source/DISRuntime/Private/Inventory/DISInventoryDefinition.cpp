#include "Inventory/DISInventoryDefinition.h"
#include "Inventory/DISInventorySlot.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "DISInventoryDefinition"

UDISInventoryDefinition::UDISInventoryDefinition() :
	DefaultCapacity(20),
	MaxExpandableCapacity(20),
	SlotIndexingDirection(EDISSlotIndexingDirection::Horizontal),
	LineBreakTileCount(5),
	bUseWeightSystem(false),
	DefaultMaxWeight(100.0f),
	BlockedTileShape(),
	DefaultSlotClass(nullptr),
	DefaultFilters()
{
	// 인벤토리 블록 타일은 기본적으로 아무 칸도 막히지 않은(모든 칸 사용 가능) 상태로 초기화
	BlockedTileShape.Points.Empty();
#if WITH_EDITOR
	UpdateBlockedTileShapeGridDimensions();
#endif
}

bool UDISInventoryDefinition::CanExpand() const
{
	// 최대 확장 용량이 기본 용량보다 큰 경우에만 확장 가능
	return (MaxExpandableCapacity > DefaultCapacity);
}

FIntPoint UDISInventoryDefinition::SlotIndexToCoord(int32 SlotIndex) const
{
	// 줄 바꿈 칸 수가 유효하지 않거나 유효 인덱스 범위를 벗어난 경우 기본값(NoneValue) 반환
	if (LineBreakTileCount <= 0 || SlotIndex < 0 || SlotIndex >= MaxExpandableCapacity)
	{
		return FIntPoint::NoneValue;
	}

	// 가로 우선(Row-Major): X는 줄바꿈 나머지, Y는 줄 번호
	if (SlotIndexingDirection == EDISSlotIndexingDirection::Horizontal)
	{
		return FIntPoint(SlotIndex % LineBreakTileCount, SlotIndex / LineBreakTileCount);
	}
	// 세로 우선(Column-Major): Y는 줄바꿈 나머지, X는 열 번호
	else
	{
		return FIntPoint(SlotIndex / LineBreakTileCount, SlotIndex % LineBreakTileCount);
	}
}

int32 UDISInventoryDefinition::CoordToSlotIndex(const FIntPoint& Coord) const
{
	// 음수 좌표이거나 줄 바꿈 칸 수가 유효하지 않은 경우 기본값(INDEX_NONE) 반환
	if (Coord.X < 0 || Coord.Y < 0 || LineBreakTileCount <= 0)
	{
		return INDEX_NONE;
	}

	int32 Index = INDEX_NONE;
	if (SlotIndexingDirection == EDISSlotIndexingDirection::Horizontal)
	{
		Index = (Coord.Y * LineBreakTileCount) + Coord.X;
	}
	else
	{
		Index = (Coord.X * LineBreakTileCount) + Coord.Y;
	}

	// 최대 확장 가능 용량을 초과하는 좌표인 경우 유효하지 않은 인덱스로 처리
	if (Index >= MaxExpandableCapacity)
	{
		return INDEX_NONE;
	}

	return Index;
}

bool UDISInventoryDefinition::IsTileBlocked(const FIntPoint& Coord) const
{
	// 유효하지 않은 음수 좌표인 경우 기본값(false) 반환
	if (Coord.X < 0 || Coord.Y < 0)
	{
		return false;
	}

	return BlockedTileShape.IsOccupied(Coord);
}

bool UDISInventoryDefinition::IsSlotBlocked(int32 SlotIndex) const
{
	const FIntPoint Coord = SlotIndexToCoord(SlotIndex);
	if (Coord == FIntPoint::NoneValue)
	{
		return false;
	}

	return IsTileBlocked(Coord);
}

FPrimaryAssetId UDISInventoryDefinition::GetPrimaryAssetId() const
{
	// 에셋 매니저에서 식별 가능한 PrimaryAssetType 반환
	return FPrimaryAssetId(DISPrimaryAssetTypes::Inventory, GetFName());
}

#if WITH_EDITOR
EDataValidationResult UDISInventoryDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// 최대 확장 용량이 기본 용량보다 작은지 검증
	if (MaxExpandableCapacity < DefaultCapacity)
	{
		Context.AddError(FText::Format(
			LOCTEXT("InventoryCapacityError", "인벤토리 정의 '{0}'의 최대 확장 가능 용량({1})은 기본 용량({2})보다 크거나 같아야 합니다."),
			FText::FromName(GetFName()),
			MaxExpandableCapacity,
			DefaultCapacity
		));
		Result = EDataValidationResult::Invalid;
	}

	// 기본 무게 제한이 음수인지 검증
	if (bUseWeightSystem == true && DefaultMaxWeight < 0.0f)
	{
		Context.AddError(FText::Format(
			LOCTEXT("InventoryWeightError", "인벤토리 정의 '{0}'의 기본 최대 무게({1})는 0 이상이어야 합니다."),
			FText::FromName(GetFName()),
			DefaultMaxWeight
		));
		Result = EDataValidationResult::Invalid;
	}

	// 줄 바꿈 기준 칸 수 검증
	if (LineBreakTileCount <= 0)
	{
		Context.AddError(FText::Format(
			LOCTEXT("LineBreakTileCountError", "인벤토리 정의 '{0}'의 줄 바꿈 기준 칸 수({1})는 1 이상이어야 합니다."),
			FText::FromName(GetFName()),
			LineBreakTileCount
		));
		Result = EDataValidationResult::Invalid;
	}

	// 블록된 타일 좌표가 인벤토리 최대 용량 범위를 벗어나는지 검증
	for (const FIntPoint& BlockedCoord : BlockedTileShape.Points)
	{
		const int32 MappedSlotIndex = CoordToSlotIndex(BlockedCoord);
		if (MappedSlotIndex == INDEX_NONE)
		{
			Context.AddError(FText::Format(
				LOCTEXT("BlockedTileOutOfBoundsError", "인벤토리 정의 '{0}'의 블록 타일 좌표({1}, {2})가 최대 허용 용량({3})의 유효 범위를 벗어났습니다."),
				FText::FromName(GetFName()),
				BlockedCoord.X,
				BlockedCoord.Y,
				MaxExpandableCapacity
			));
			Result = EDataValidationResult::Invalid;
		}
	}

	// 기본 슬롯 클래스 타입이 지정된 경우 UDISInventorySlot의 자손 클래스인지 검증
	if (DefaultSlotClass != nullptr)
	{
		if (DefaultSlotClass->IsChildOf(UDISInventorySlot::StaticClass()) == false)
		{
			Context.AddError(FText::Format(
				LOCTEXT("InventorySlotClassError", "인벤토리 정의 '{0}'의 슬롯 클래스 타입({1})은 UDISInventorySlot을 상속해야 합니다."),
				FText::FromName(GetFName()),
				FText::FromString(DefaultSlotClass->GetName())
			));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}

void UDISInventoryDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// 변경된 레이아웃 규격(SlotIndexingDirection, LineBreakTileCount 등)에 맞춰
	// 에디터 고정 그리드 규격을 먼저 갱신한 후 부모 프로퍼티 변경 이벤트 및 OnObjectPropertyChanged를 전파
	UpdateBlockedTileShapeGridDimensions();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UDISInventoryDefinition::PostEditUndo()
{
	// Undo/Redo 시에도 복원된 레이아웃 규격에 맞춰 에디터 고정 그리드 규격 최신화
	UpdateBlockedTileShapeGridDimensions();

	Super::PostEditUndo();
}

void UDISInventoryDefinition::UpdateBlockedTileShapeGridDimensions()
{
	// 유효한 줄바꿈 수 및 최대 용량을 가진 경우에만 고정 그리드 규격 산출
	if (LineBreakTileCount > 0 && MaxExpandableCapacity > 0)
	{
		const int32 PrimaryDim = LineBreakTileCount;
		const int32 SecondaryDim = FMath::DivideAndRoundUp(MaxExpandableCapacity, PrimaryDim);

		if (SlotIndexingDirection == EDISSlotIndexingDirection::Horizontal)
		{
			// 가로 우선: X(열) = LineBreakTileCount, Y(행) = DivideAndRoundUp
			BlockedTileShape.EditorFixedGridDimensions = FIntPoint(PrimaryDim, SecondaryDim);
		}
		else
		{
			// 세로 우선: X(열) = DivideAndRoundUp, Y(행) = LineBreakTileCount
			BlockedTileShape.EditorFixedGridDimensions = FIntPoint(SecondaryDim, PrimaryDim);
		}
	}
	else
	{
		BlockedTileShape.EditorFixedGridDimensions = FIntPoint::ZeroValue;
	}
}
#endif

void UDISInventoryDefinition::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	UpdateBlockedTileShapeGridDimensions();
#endif
}

#undef LOCTEXT_NAMESPACE
