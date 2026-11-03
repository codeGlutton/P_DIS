#include "Combat/Pawn/BBMPlayerMerchantBase.h"
#include "Combat/Pawn/BBMPlayerCharacterDefinition.h"
#include "Inventory/BBMInventoryComponent.h"
#include "Inventory/BBMInventoryDefinition.h"
#include "Inventory/BBMInventory.h"
#include "Item/DISItemStack.h"
#include "Item/BBMItemEntry.h"
#include "Item/BBMItemDefinition.h"

ABBMPlayerMerchantBase::ABBMPlayerMerchantBase() :
	InventoryComponent(nullptr)
{
	// 플레이어 상인 기본 팀 설정
	SetGenericTeamId(FGenericTeamId(static_cast<uint8>(EBBMTeamType::Player)));

	// 배낭 인벤토리 래퍼 컴포넌트 서브오브젝트 생성
	InventoryComponent = CreateDefaultSubobject<UBBMInventoryComponent>(TEXT("InventoryComponent"));
}

void ABBMPlayerMerchantBase::InitializeFromDefinition()
{
	Super::InitializeFromDefinition();

	// 플레이어 정의 에셋 기반 배낭 인벤토리 자동 초기화
	InitializeInventoryFromDefinition();
}

void ABBMPlayerMerchantBase::UpdateBeatSequence()
{
	GenerateBeatSequenceFromInventory();
}

FBBMBeatSequence ABBMPlayerMerchantBase::GetCurrentBeatSequence() const
{
	return CachedBeatSequence;
}

FBBMBeatSequence ABBMPlayerMerchantBase::GenerateBeatSequenceFromInventory()
{
	CachedBeatSequence = FBBMBeatSequence();
	CachedBeatSequence.Beats.SetNum(8); // 8비트 슬롯 기본 확보

	const UBBMInventory* TargetInventory = GetInventory();
	if (TargetInventory == nullptr)
	{
		return CachedBeatSequence;
	}

	// 1. 인벤토리 Definition의 ExtractionOrder에 따라 정렬된 아이템 스택 목록 획득
	const TArray<UDISItemStack*> OrderedStacks = TargetInventory->GetItemStacksInExtractionOrder();

	int32 CurrentBeat = 0;

	// 2. 순차적으로 아이템 비트를 누적하여 8비트 타임라인 구성 (GDD 4.1)
	for (const UDISItemStack* ItemStack : OrderedStacks)
	{
		if (ItemStack == nullptr || ItemStack->IsEmpty() == true)
		{
			continue;
		}

		// 아이템이 점유하는 칸 수(Point Count)가 곧 행동에 필요한 비트 수
		const int32 ItemBeatCost = ItemStack->GetOccupiedPoints().Num();
		if (ItemBeatCost <= 0)
		{
			continue;
		}

		const int32 StartBeat = CurrentBeat;
		const int32 EndBeat = StartBeat + ItemBeatCost;

		// 8비트 초과 시 타임라인 등록 중단 (GDD 4.1: 비트 초과 Overflow로 이번 턴 미실행)
		if (EndBeat > 8)
		{
			break;
		}

		// 아이템의 비트 시퀀스 획득 (BBMItemEntry의 DynamicBeatSequence 우선, 없을 시 BBMItemDefinition의 BeatSequence)
		const FBBMBeatSequence* ItemSeq = nullptr;
		const UBBMItemEntry* BBMEntry = Cast<UBBMItemEntry>(ItemStack->GetTopEntry());
		if (BBMEntry != nullptr)
		{
			ItemSeq = &BBMEntry->GetDynamicBeatSequence();
		}
		else if (ItemStack->GetTopEntry() != nullptr)
		{
			const UBBMItemDefinition* ItemDef = Cast<UBBMItemDefinition>(ItemStack->GetTopEntry()->GetItemDefinition());
			if (ItemDef != nullptr)
			{
				ItemSeq = &ItemDef->GetBeatSequence();
			}
		}

		// 비트 액션 및 애니메이션 복사
		for (int32 BeatOffset = 0; BeatOffset < ItemBeatCost; ++BeatOffset)
		{
			const int32 TargetBeat = StartBeat + BeatOffset;
			if (ItemSeq != nullptr && ItemSeq->Beats.IsValidIndex(BeatOffset) == true)
			{
				CachedBeatSequence.Beats[TargetBeat] = ItemSeq->Beats[BeatOffset];
			}
		}

		// 비트 구간 지속 효과(EffectRanges) 시간 오프셋 시프트 및 등록
		if (ItemSeq != nullptr)
		{
			for (const FBBMBeatEffectRange& Range : ItemSeq->EffectRanges)
			{
				FBBMBeatEffectRange ShiftedRange = Range;
				ShiftedRange.StartBeat += StartBeat;
				ShiftedRange.EndBeat += StartBeat;
				ShiftedRange.EndBeat = FMath::Min(ShiftedRange.EndBeat, 8);

				if (ShiftedRange.IsValid() == true)
				{
					CachedBeatSequence.EffectRanges.Add(ShiftedRange);
				}
			}
		}

		CurrentBeat = EndBeat;
	}

	return CachedBeatSequence;
}

UBBMInventoryComponent* ABBMPlayerMerchantBase::GetInventoryComponent() const
{
	return InventoryComponent;
}

UBBMInventory* ABBMPlayerMerchantBase::GetInventory() const
{
	if (InventoryComponent != nullptr)
	{
		return InventoryComponent->GetInventory();
	}
	return nullptr;
}

const UBBMPlayerCharacterDefinition* ABBMPlayerMerchantBase::GetPlayerCharacterDefinition() const
{
	return Cast<const UBBMPlayerCharacterDefinition>(CharacterDefinition.Get());
}

void ABBMPlayerMerchantBase::InitializeInventoryFromDefinition()
{
	if (InventoryComponent == nullptr)
	{
		return;
	}

	const UBBMPlayerCharacterDefinition* PlayerDef = GetPlayerCharacterDefinition();
	if (PlayerDef != nullptr)
	{
		const UBBMInventoryDefinition* InvenDef = PlayerDef->GetInventoryDefinition();
		if (InvenDef != nullptr)
		{
			InventoryComponent->InitializeInventory(InvenDef);
		}
	}
}
