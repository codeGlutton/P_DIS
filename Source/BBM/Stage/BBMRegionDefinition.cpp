/*****************************************************************//**
 * @file   BBMRegionDefinition.cpp
 * @brief  BBM(비트 배낭 상인) 지역(Region) 정적 정의 데이터 에셋 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Stage/BBMRegionDefinition.h"
#include "Stage/BBMEncounterDefinition.h"
#include "Item/BBMItemDefinition.h"
#include "Global/BBMGlobalLogic.h"

UBBMRegionDefinition::UBBMRegionDefinition() :
	RegionName(FText::FromString(TEXT("New Region")))
{
}

FPrimaryAssetId UBBMRegionDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(BBMPrimaryAssetTypes::Region, GetFName());
}

TArray<UBBMEncounterDefinition*> UBBMRegionDefinition::GenerateFiveEncounters(const FRandomStream& Stream) const
{
	TArray<UBBMEncounterDefinition*> Generated;
	Generated.Reserve(5);

	const TMap<EBBMEncounterType, float> TypeWeights = GetEffectiveEncounterTypeWeights();

	// 1~4회차: 1단계 인카운터 타입 가중치 룰렛 -> 2단계 해당 타입 풀에서 균일(1/N) 무작위 추출 (에셋 비로드 원칙 준수)
	for (int32 Index = 0; Index < 4; ++Index)
	{
		// 후보가 1개 이상 존재하는 유효 타입 목록 및 총 가중치 수집
		struct FEncounterTypeCandidate
		{
			EBBMEncounterType Type;
			float Weight;
		};

		TArray<FEncounterTypeCandidate> ValidTypeCandidates;
		float TotalTypeWeight = 0.0f;

		auto CheckAndAddType = [&](EBBMEncounterType InType, int32 PoolCount)
		{
			if (PoolCount > 0)
			{
				const float* W = TypeWeights.Find(InType);
				const float Weight = (W != nullptr) ? FMath::Max(0.0f, *W) : 0.0f;
				if (Weight > 0.0f)
				{
					ValidTypeCandidates.Add({ InType, Weight });
					TotalTypeWeight += Weight;
				}
			}
		};

		CheckAndAddType(EBBMEncounterType::Combat, CombatEncountersPool.Num());
		CheckAndAddType(EBBMEncounterType::Event, EventEncountersPool.Num());
		CheckAndAddType(EBBMEncounterType::Treasure, TreasureEncountersPool.Num());

		UBBMEncounterDefinition* SelectedEncounter = nullptr;

		if (ValidTypeCandidates.IsEmpty() == false && TotalTypeWeight > 0.0f)
		{
			// 1단계: 인카운터 타입 룰렛 추첨
			const float Roll = Stream.FRandRange(0.0f, TotalTypeWeight);
			float Accumulated = 0.0f;
			EBBMEncounterType SelectedType = ValidTypeCandidates.Last().Type;

			for (const FEncounterTypeCandidate& Cand : ValidTypeCandidates)
			{
				Accumulated += Cand.Weight;
				if (Roll <= Accumulated)
				{
					SelectedType = Cand.Type;
					break;
				}
			}

			// 2단계: 선정된 타입 풀에서 1/N 균일 무작위 1개 선정 (선정된 에셋만 동기 로드)
			if (SelectedType == EBBMEncounterType::Combat && CombatEncountersPool.Num() > 0)
			{
				const int32 PickIdx = Stream.RandRange(0, CombatEncountersPool.Num() - 1);
				SelectedEncounter = CombatEncountersPool[PickIdx].LoadSynchronous();
			}
			else if (SelectedType == EBBMEncounterType::Event && EventEncountersPool.Num() > 0)
			{
				const int32 PickIdx = Stream.RandRange(0, EventEncountersPool.Num() - 1);
				SelectedEncounter = EventEncountersPool[PickIdx].LoadSynchronous();
			}
			else if (SelectedType == EBBMEncounterType::Treasure && TreasureEncountersPool.Num() > 0)
			{
				const int32 PickIdx = Stream.RandRange(0, TreasureEncountersPool.Num() - 1);
				SelectedEncounter = TreasureEncountersPool[PickIdx].LoadSynchronous();
			}
		}

		// 만약 가중치 기반 추첨이 실패하거나 비어있으면 존재하는 풀에서 균일 폴백
		if (SelectedEncounter == nullptr)
		{
			if (CombatEncountersPool.Num() > 0)
			{
				SelectedEncounter = CombatEncountersPool[Stream.RandRange(0, CombatEncountersPool.Num() - 1)].LoadSynchronous();
			}
			else if (EventEncountersPool.Num() > 0)
			{
				SelectedEncounter = EventEncountersPool[Stream.RandRange(0, EventEncountersPool.Num() - 1)].LoadSynchronous();
			}
			else if (TreasureEncountersPool.Num() > 0)
			{
				SelectedEncounter = TreasureEncountersPool[Stream.RandRange(0, TreasureEncountersPool.Num() - 1)].LoadSynchronous();
			}
		}

		if (SelectedEncounter != nullptr)
		{
			Generated.Add(SelectedEncounter);
		}
	}

	// 5회차: 보스 만남 풀(BossEncountersPool)이 존재하면 1/N 균일 무작위 채택, 비어있을 시 일반 CombatEncountersPool에서 1/N 균일 선정
	UBBMEncounterDefinition* BossEncounter = nullptr;
	if (BossEncountersPool.Num() > 0)
	{
		const int32 BossIdx = Stream.RandRange(0, BossEncountersPool.Num() - 1);
		BossEncounter = BossEncountersPool[BossIdx].LoadSynchronous();
	}
	else if (CombatEncountersPool.Num() > 0)
	{
		const int32 CombatIdx = Stream.RandRange(0, CombatEncountersPool.Num() - 1);
		BossEncounter = CombatEncountersPool[CombatIdx].LoadSynchronous();
	}

	if (BossEncounter != nullptr)
	{
		Generated.Add(BossEncounter);
	}

	return Generated;
}

TArray<UBBMRegionDefinition*> UBBMRegionDefinition::GetNextRegionOptions(int32 MaxOptions, const FRandomStream& Stream) const
{
	TArray<UBBMRegionDefinition*> Result;
	if (NextPossibleRegions.Num() == 0 || MaxOptions <= 0)
	{
		return Result;
	}

	// 소프트 레퍼런스 후보 목록 복사 후 Fisher-Yates 셔플
	TArray<TSoftObjectPtr<UBBMRegionDefinition>> Candidates = NextPossibleRegions;
	for (int32 i = Candidates.Num() - 1; i > 0; --i)
	{
		const int32 j = Stream.RandRange(0, i);
		Candidates.Swap(i, j);
	}

	// 최대 MaxOptions개 추출 후 동기 로드하여 반환 (평상시 비로드 유지, 유저 분기 제시 시에만 선택 로드)
	const int32 ReturnCount = FMath::Min(MaxOptions, Candidates.Num());
	for (int32 i = 0; i < ReturnCount; ++i)
	{
		if (Candidates[i].IsNull() == false)
		{
			UBBMRegionDefinition* LoadedRegion = Candidates[i].LoadSynchronous();
			if (LoadedRegion != nullptr)
			{
				Result.Add(LoadedRegion);
			}
		}
	}

	return Result;
}

const FText& UBBMRegionDefinition::GetRegionName() const
{
	return RegionName;
}

const TSoftObjectPtr<UWorld>& UBBMRegionDefinition::GetLevelReference() const
{
	return LevelReference;
}

const TArray<TSoftObjectPtr<UBBMCombatEncounterDefinition>>& UBBMRegionDefinition::GetCombatEncountersPool() const
{
	return CombatEncountersPool;
}

const TArray<TSoftObjectPtr<UBBMEventEncounterDefinition>>& UBBMRegionDefinition::GetEventEncountersPool() const
{
	return EventEncountersPool;
}

const TArray<TSoftObjectPtr<UBBMTreasureEncounterDefinition>>& UBBMRegionDefinition::GetTreasureEncountersPool() const
{
	return TreasureEncountersPool;
}

const TArray<TSoftObjectPtr<UBBMCombatEncounterDefinition>>& UBBMRegionDefinition::GetBossEncountersPool() const
{
	return BossEncountersPool;
}

TMap<EBBMEncounterType, float> UBBMRegionDefinition::GetEffectiveEncounterTypeWeights() const
{
	if (EncounterTypeWeights.IsEmpty() == false)
	{
		return EncounterTypeWeights;
	}

	// 미설정 시 기본 3대 만남 타입 출현 가중치 (Combat 60%, Event 20%, Treasure 20%)
	TMap<EBBMEncounterType, float> DefaultWeights;
	DefaultWeights.Add(EBBMEncounterType::Combat, 60.0f);
	DefaultWeights.Add(EBBMEncounterType::Event, 20.0f);
	DefaultWeights.Add(EBBMEncounterType::Treasure, 20.0f);
	return DefaultWeights;
}

const TArray<TSoftObjectPtr<UBBMRegionDefinition>>& UBBMRegionDefinition::GetNextPossibleRegions() const
{
	return NextPossibleRegions;
}

TMap<FGameplayTag, float> UBBMRegionDefinition::GetEffectiveRarityWeights() const
{
	if (RarityWeights.IsEmpty() == false)
	{
		return RarityWeights;
	}

	// 미설정 시 기본 4대 희귀도 가중치 폴백 (Common 70%, Rare 20%, Epic 8%, Legendary 2%)
	TMap<FGameplayTag, float> DefaultWeights;
	DefaultWeights.Add(BBMItemRarityTags::Item_Rarity_Common, 70.0f);
	DefaultWeights.Add(BBMItemRarityTags::Item_Rarity_Rare, 20.0f);
	DefaultWeights.Add(BBMItemRarityTags::Item_Rarity_Epic, 8.0f);
	DefaultWeights.Add(BBMItemRarityTags::Item_Rarity_Legendary, 2.0f);
	return DefaultWeights;
}

TArray<FGameplayTag> UBBMRegionDefinition::GetEffectiveNpcItemTypePool(const UBBMGlobalLogic* GlobalLogic) const
{
	if (NpcDesiredItemTypePool.IsEmpty() == false)
	{
		return NpcDesiredItemTypePool;
	}

	if (GlobalLogic != nullptr)
	{
		return GlobalLogic->GetLeafItemTypeTags();
	}

	return TArray<FGameplayTag>();
}

float UBBMRegionDefinition::GetRarityWeight(const FGameplayTag& InRarityTag, const TMap<FGameplayTag, float>& InWeights)
{
	if (InRarityTag.IsValid() == false)
	{
		return 0.0f;
	}

	const float* Found = InWeights.Find(InRarityTag);
	return (Found != nullptr) ? FMath::Max(0.0f, *Found) : 0.0f;
}

UBBMItemDefinition* UBBMRegionDefinition::PickRandomItem(
	const TArray<TSoftObjectPtr<UBBMItemDefinition>>& InPool,
	const TMap<FGameplayTag, float>& InWeights,
	const FRandomStream& Stream) const
{
	if (InPool.IsEmpty() == true)
	{
		return nullptr;
	}

	// 1. 풀 내 아이템들을 로드하여 유효 희귀도별로 런타임 버킷팅
	TMap<FGameplayTag, TArray<UBBMItemDefinition*>> ItemsByRarity;
	for (const TSoftObjectPtr<UBBMItemDefinition>& SoftItem : InPool)
	{
		if (UBBMItemDefinition* Item = SoftItem.LoadSynchronous())
		{
			const float W = GetRarityWeight(Item->RarityTag, InWeights);
			if (W > 0.0f)
			{
				ItemsByRarity.FindOrAdd(Item->RarityTag).Add(Item);
			}
		}
	}

	if (ItemsByRarity.IsEmpty() == true)
	{
		return nullptr;
	}

	// 2. 존재하는 희귀도 태그들의 가중치 합산
	float TotalRarityWeight = 0.0f;
	for (const auto& Pair : ItemsByRarity)
	{
		TotalRarityWeight += GetRarityWeight(Pair.Key, InWeights);
	}

	if (TotalRarityWeight <= 0.0f)
	{
		return nullptr;
	}

	// 3. 1단계: 희귀도 룰렛 추첨 (Rarity Roll)
	const float Roll = Stream.FRandRange(0.0f, TotalRarityWeight);
	float Accumulated = 0.0f;
	FGameplayTag SelectedRarityTag;

	for (const auto& Pair : ItemsByRarity)
	{
		Accumulated += GetRarityWeight(Pair.Key, InWeights);
		if (Roll <= Accumulated)
		{
			SelectedRarityTag = Pair.Key;
			break;
		}
	}

	if (SelectedRarityTag.IsValid() == false)
	{
		// 부동소수점 오차 안전 폴백: 첫 번째 유효 희귀도 채택
		for (const auto& Pair : ItemsByRarity)
		{
			SelectedRarityTag = Pair.Key;
			break;
		}
	}

	// 4. 2단계: 당첨된 희귀도 아이템 목록에서 1/N 균일 무작위 추첨 (Uniform Item Roll)
	const TArray<UBBMItemDefinition*>* FoundItems = ItemsByRarity.Find(SelectedRarityTag);
	if (FoundItems != nullptr && FoundItems->IsEmpty() == false)
	{
		const int32 RandIdx = Stream.RandRange(0, FoundItems->Num() - 1);
		return (*FoundItems)[RandIdx];
	}

	return nullptr;
}

TArray<UBBMItemDefinition*> UBBMRegionDefinition::PickRandomItemsWithoutDuplicates(
	const TArray<TSoftObjectPtr<UBBMItemDefinition>>& InPool,
	const TMap<FGameplayTag, float>& InWeights,
	int32 Count,
	const FRandomStream& Stream) const
{
	TArray<UBBMItemDefinition*> Picked;
	if (InPool.IsEmpty() == true || Count <= 0)
	{
		return Picked;
	}

	// 중복 방지를 위한 임시 복사본 풀
	TArray<TSoftObjectPtr<UBBMItemDefinition>> RemainingPool = InPool;
	for (int32 i = 0; i < Count; ++i)
	{
		UBBMItemDefinition* Item = PickRandomItem(RemainingPool, InWeights, Stream);
		if (Item == nullptr)
		{
			break;
		}

		Picked.Add(Item);

		// 선택된 아이템을 풀에서 제거
		RemainingPool.RemoveAll([Item](const TSoftObjectPtr<UBBMItemDefinition>& SoftPtr)
		{
			return SoftPtr.Get() == Item || SoftPtr.ToSoftObjectPath() == FSoftObjectPath(Item);
		});

		if (RemainingPool.IsEmpty() == true)
		{
			break;
		}
	}

	return Picked;
}
