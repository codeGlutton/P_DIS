/*****************************************************************//**
 * @file   BBMStageTypes.cpp
 * @brief  BBM(비트 배낭 상인) 지역 및 만남 관련 구조체 생성자 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Stage/BBMStageTypes.h"
#include "Stage/BBMEncounterDefinition.h"
#include "Stage/BBMEventNpcBase.h"
#include "Item/BBMItemDefinition.h"
#include "Item/DISItemDefinition.h"
#include "Combat/Pawn/BBMAICombatantDefinition.h"
#include "Engine/AssetManager.h"

namespace BBMStageAssetUtils
{
	static void AddPrimaryAssetIdFromObjectPath(const FSoftObjectPath& InPath, const FPrimaryAssetType& InFallbackType, TArray<FPrimaryAssetId>& OutPrimaryAssetIds)
	{
		if (InPath.IsNull() == true)
		{
			return;
		}

		if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
		{
			const FPrimaryAssetId AssetId = AssetManager->GetPrimaryAssetIdForPath(InPath);
			if (AssetId.IsValid() == true)
			{
				OutPrimaryAssetIds.AddUnique(AssetId);
				return;
			}
		}

		const FName AssetName = InPath.GetAssetFName();
		if (AssetName != NAME_None && InFallbackType.IsValid() == true)
		{
			OutPrimaryAssetIds.AddUnique(FPrimaryAssetId(InFallbackType, AssetName));
		}
	}

	template<typename T>
	static void AddPrimaryAssetIdFromSoftPtr(const TSoftObjectPtr<T>& InSoftPtr, const FPrimaryAssetType& InFallbackType, TArray<FPrimaryAssetId>& OutPrimaryAssetIds)
	{
		if (const UPrimaryDataAsset* LoadedPda = Cast<UPrimaryDataAsset>(InSoftPtr.Get()))
		{
			const FPrimaryAssetId AssetId = LoadedPda->GetPrimaryAssetId();
			if (AssetId.IsValid() == true)
			{
				OutPrimaryAssetIds.AddUnique(AssetId);
				return;
			}
		}

		AddPrimaryAssetIdFromObjectPath(InSoftPtr.ToSoftObjectPath(), InFallbackType, OutPrimaryAssetIds);
	}
}

FBBMEncounterSpawnWeight::FBBMEncounterSpawnWeight() :
	EncounterDefinition(nullptr),
	SelectionWeight(1.0f)
{
}

FBBMEncounterSpawnWeight::FBBMEncounterSpawnWeight(TSoftObjectPtr<UBBMEncounterDefinition> InDef, float InWeight) :
	EncounterDefinition(InDef),
	SelectionWeight(InWeight)
{
}

FBBMEncounterSpawnWeight::FBBMEncounterSpawnWeight(const UBBMEncounterDefinition* InDef, float InWeight) :
	EncounterDefinition(const_cast<UBBMEncounterDefinition*>(InDef)),
	SelectionWeight(InWeight)
{
}

FBBMRuntimeNpcData::FBBMRuntimeNpcData() :
	NpcClass(nullptr),
	OfferedItem(nullptr),
	DesiredItemTypeTag(FGameplayTag::EmptyTag)
{
}

FBBMRuntimeEncounterData::FBBMRuntimeEncounterData() :
	EncounterDefinition(nullptr)
{
}

EBBMEncounterType FBBMRuntimeEncounterData::GetEncounterType() const
{
	if (const UBBMEncounterDefinition* LoadedDef = EncounterDefinition.LoadSynchronous())
	{
		return LoadedDef->GetEncounterType();
	}

	return EBBMEncounterType::Combat;
}

void FBBMRuntimeEncounterData::CollectPrimaryAssetIds(TArray<FPrimaryAssetId>& OutPrimaryAssetIds) const
{
	BBMStageAssetUtils::AddPrimaryAssetIdFromSoftPtr(EncounterDefinition, FPrimaryAssetType(BBMPrimaryAssetTypes::Encounter), OutPrimaryAssetIds);
}

FBBMRuntimeTreasureEncounterData::FBBMRuntimeTreasureEncounterData() :
	FBBMRuntimeEncounterData(),
	SelectedRewardItem(nullptr)
{
}

void FBBMRuntimeTreasureEncounterData::CollectPrimaryAssetIds(TArray<FPrimaryAssetId>& OutPrimaryAssetIds) const
{
	FBBMRuntimeEncounterData::CollectPrimaryAssetIds(OutPrimaryAssetIds);

	for (const TSoftObjectPtr<UBBMItemDefinition>& ItemPtr : TreasureOptions)
	{
		BBMStageAssetUtils::AddPrimaryAssetIdFromSoftPtr(ItemPtr, FPrimaryAssetType(DISPrimaryAssetTypes::Item), OutPrimaryAssetIds);
	}

	if (SelectedRewardItem.IsNull() == false)
	{
		BBMStageAssetUtils::AddPrimaryAssetIdFromSoftPtr(SelectedRewardItem, FPrimaryAssetType(DISPrimaryAssetTypes::Item), OutPrimaryAssetIds);
	}
}

FBBMRuntimeCombatEncounterData::FBBMRuntimeCombatEncounterData() :
	FBBMRuntimeEncounterData()
{
}

void FBBMRuntimeCombatEncounterData::CollectPrimaryAssetIds(TArray<FPrimaryAssetId>& OutPrimaryAssetIds) const
{
	FBBMRuntimeEncounterData::CollectPrimaryAssetIds(OutPrimaryAssetIds);

	for (const TSoftObjectPtr<UBBMItemDefinition>& ItemPtr : CombatRewardItems)
	{
		BBMStageAssetUtils::AddPrimaryAssetIdFromSoftPtr(ItemPtr, FPrimaryAssetType(DISPrimaryAssetTypes::Item), OutPrimaryAssetIds);
	}

	if (const UBBMCombatEncounterDefinition* CombatDef = Cast<UBBMCombatEncounterDefinition>(EncounterDefinition.Get()))
	{
		for (const TSoftObjectPtr<UBBMAICombatantDefinition>& MonsterPtr : CombatDef->GetMonsterList())
		{
			BBMStageAssetUtils::AddPrimaryAssetIdFromSoftPtr(MonsterPtr, FPrimaryAssetType(BBMPrimaryAssetTypes::AICombatant), OutPrimaryAssetIds);
		}
	}
}

FBBMRuntimeEventEncounterData::FBBMRuntimeEventEncounterData() :
	FBBMRuntimeEncounterData()
{
}

void FBBMRuntimeEventEncounterData::CollectPrimaryAssetIds(TArray<FPrimaryAssetId>& OutPrimaryAssetIds) const
{
	FBBMRuntimeEncounterData::CollectPrimaryAssetIds(OutPrimaryAssetIds);

	for (const FBBMRuntimeNpcData& NpcData : EventNpcs)
	{
		BBMStageAssetUtils::AddPrimaryAssetIdFromSoftPtr(NpcData.OfferedItem, FPrimaryAssetType(DISPrimaryAssetTypes::Item), OutPrimaryAssetIds);
	}
}

