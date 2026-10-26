#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"
#include "Inventory/DISSharedInventoryDefinition.h"
#include "Inventory/DISSharedInventory.h"
#include "Inventory/DISSharedInventorySlot.h"
#include "Inventory/DISInventoryDefinition.h"
#include "Inventory/DISInventory.h"
#include "Inventory/DISInventorySlot.h"
#include "Inventory/DISInventoryFilter.h"
#include "Item/DISItemDefinition.h"
#include "Item/DISItemEntry.h"
#include "Item/DISItemStack.h"
#include "Global/DISGlobalSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * UDISSharedInventory 1D 고빈도 슬롯 조작 스트레스 및 성능 벤치마크 테스트
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISSharedInventoryStressTest,
	"DIS.SharedInventory.Stress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISSharedInventoryStressTest::RunTest(const FString& Parameters)
{
	constexpr int32 IterationCount = 100000; // 10만 회 고빈도 조작
	constexpr int32 SlotCapacity = 8;        // 8칸 퀵슬롯

	UDISSharedInventoryDefinition* Def = NewObject<UDISSharedInventoryDefinition>();
	Def->DefaultCapacity = SlotCapacity;

	UDISSharedInventory* Inventory = NewObject<UDISSharedInventory>();
	Inventory->InitializeInventory(Def);

	// 테스트용 아이템 스택 풀 생성 (16종)
	constexpr int32 PoolSize = 16;
	TArray<TObjectPtr<UDISItemStack>> StackPool;
	StackPool.Reserve(PoolSize);

	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		UDISItemDefinition* ItemDef = NewObject<UDISItemDefinition>();
		UDISItemStack* Stack = NewObject<UDISItemStack>();
		UDISItemEntry* Entry = NewObject<UDISItemEntry>();
		Entry->InitializeEntry(ItemDef);
		Stack->PushEntry(Entry);
		StackPool.Add(Stack);
	}

	// 성능 계측 시작
	const double StartTimeSeconds = FPlatformTime::Seconds();

	int32 SuccessfulOperations = 0;

	for (int32 Iteration = 0; Iteration < IterationCount; ++Iteration)
	{
		const int32 ActionType = Iteration % 4;
		const int32 TargetSlotA = Iteration % SlotCapacity;
		const int32 TargetSlotB = (Iteration + 3) % SlotCapacity;
		UDISItemStack* SelectedStack = StackPool[Iteration % PoolSize];

		switch (ActionType)
		{
		case 0:
			// 슬롯 등록 및 이전 슬롯 자동 해제(중복 방지) 스트레스
			if (Inventory->SetSlotItemStack(TargetSlotA, SelectedStack) == true)
			{
				++SuccessfulOperations;
			}
			break;

		case 1:
			// 슬롯 간 고속 스왑 스트레스
			if (Inventory->SwapSlots(TargetSlotA, TargetSlotB) == true)
			{
				++SuccessfulOperations;
			}
			break;

		case 2:
			// 스택 위치 고속 역방향 검색
			if (Inventory->FindSlotWithItemStack(SelectedStack) != INDEX_NONE)
			{
				++SuccessfulOperations;
			}
			break;

		case 3:
			// 슬롯 비우기
			Inventory->ClearSlot(TargetSlotA);
			++SuccessfulOperations;
			break;

		default:
			break;
		}
	}

	const double ElapsedSeconds = FPlatformTime::Seconds() - StartTimeSeconds;
	const double OpsPerSecond = static_cast<double>(IterationCount) / FMath::Max(ElapsedSeconds, 0.000001);
	const double AverageLatencyMicroseconds = (ElapsedSeconds * 1000000.0) / static_cast<double>(IterationCount);

	AddInfo(FString::Printf(TEXT("[UDISSharedInventory Stress Benchmark] Total Iterations: %d | Time: %.4f s (%.2f ms) | Ops/Sec: %.2f ops/s | Avg Latency: %.4f us"),
		IterationCount, ElapsedSeconds, ElapsedSeconds * 1000.0, OpsPerSecond, AverageLatencyMicroseconds));

	TestTrue(TEXT("스트레스 테스트 반복 루프가 정상 완료되어야 함"), SuccessfulOperations > 0);
	TestTrue(TEXT("10만 회 1D 슬롯 조작이 1.0초 이내에 완료되어야 함 (최소 100,000 ops/s)"), ElapsedSeconds < 1.0);

	return true;
}

/**
 * UDISInventory 2D 대형 그리드 공간 배치/회전/충돌 판정 스트레스 및 성능 벤치마크 테스트
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventorySpatialStressTest,
	"DIS.Inventory.SpatialStress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventorySpatialStressTest::RunTest(const FString& Parameters)
{
	constexpr int32 IterationCount = 10000; // 1만 회 2D 공간 조작
	constexpr int32 GridWidth = 10;
	constexpr int32 GridHeight = 10;
	constexpr int32 TotalSlots = GridWidth * GridHeight; // 100칸 대형 그리드

	UDISInventoryDefinition* Def = NewObject<UDISInventoryDefinition>();
	Def->DefaultCapacity = TotalSlots;
	Def->LineBreakTileCount = GridWidth;
	Def->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;

	UDISInventory* Inventory = NewObject<UDISInventory>();
	Inventory->InitializeInventory(Def);

	// 다양한 형태의 다중 타일 아이템 생성 (1x1, 2x1, 1x3, 2x2, L자)
	TArray<TObjectPtr<UDISItemStack>> ItemPool;
	const TArray<TArray<FIntPoint>> Shapes = {
		{ FIntPoint(0, 0) },                                          // 1x1
		{ FIntPoint(0, 0), FIntPoint(1, 0) },                         // 2x1
		{ FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(2, 0) },        // 3x1
		{ FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(1, 1) }, // 2x2
		{ FIntPoint(0, 0), FIntPoint(0, 1), FIntPoint(1, 1) }         // L-shape
	};

	for (int32 ShapeIdx = 0; ShapeIdx < Shapes.Num(); ++ShapeIdx)
	{
		UDISItemDefinition* ItemDef = NewObject<UDISItemDefinition>();
		ItemDef->bCanRotate = true;
		ItemDef->Shape.Points = Shapes[ShapeIdx];

		UDISItemStack* Stack = NewObject<UDISItemStack>();
		UDISItemEntry* Entry = NewObject<UDISItemEntry>();
		Entry->InitializeEntry(ItemDef);
		Stack->PushEntry(Entry);
		ItemPool.Add(Stack);
	}

	const double StartTimeSeconds = FPlatformTime::Seconds();

	int32 TotalAttempted = 0;
	int32 PlacedCount = 0;
	int32 RotatedCount = 0;

	for (int32 Iteration = 0; Iteration < IterationCount; ++Iteration)
	{
		const int32 TargetSlot = Iteration % TotalSlots;
		UDISItemStack* TargetStack = ItemPool[Iteration % ItemPool.Num()];

		++TotalAttempted;

		// 1. 배치 시도 (충돌 검증 포함)
		if (Inventory->CanAcceptItemStackAtSlot(TargetStack, TargetSlot) == true)
		{
			const FDISItemPlacementResult Result = Inventory->SetSlotItemStack(TargetSlot, TargetStack);
			if (Result.bSuccess == true)
			{
				++PlacedCount;

				// 2. 배치 성공 시 시계방향 및 반시계방향 회전 스트레스 테스트
				if (Inventory->RotateItemStack(TargetStack, true) == true)
				{
					++RotatedCount;
					Inventory->RotateItemStack(TargetStack, false);
				}
			}
		}
		else
		{
			// 공간이 꽉 차거나 충돌 시 주기적으로 슬롯 비우기
			Inventory->ClearSlot(TargetSlot);
		}
	}

	const double ElapsedSeconds = FPlatformTime::Seconds() - StartTimeSeconds;
	const double OpsPerSecond = static_cast<double>(IterationCount) / FMath::Max(ElapsedSeconds, 0.000001);
	const double AverageLatencyMicroseconds = (ElapsedSeconds * 1000000.0) / static_cast<double>(IterationCount);

	AddInfo(FString::Printf(TEXT("[UDISInventory 2D Spatial Stress Benchmark] Iterations: %d | Time: %.4f s (%.2f ms) | Ops/Sec: %.2f ops/s | Avg Latency: %.4f us | Placed: %d, Rotated: %d"),
		IterationCount, ElapsedSeconds, ElapsedSeconds * 1000.0, OpsPerSecond, AverageLatencyMicroseconds, PlacedCount, RotatedCount));

	TestTrue(TEXT("2D 공간 스트레스 테스트 루프 완료"), TotalAttempted == IterationCount);
	TestTrue(TEXT("1만 회 2D 공간 판정/배치/회전이 2.0초 이내에 완료되어야 함 (최소 5,000 ops/s)"), ElapsedSeconds < 2.0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
