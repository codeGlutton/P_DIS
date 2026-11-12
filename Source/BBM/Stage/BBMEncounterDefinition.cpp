/*****************************************************************//**
 * @file   BBMEncounterDefinition.cpp
 * @brief  BBM(비트 배낭 상인) 만남(Encounter) 정적 정의 데이터 에셋 클래스 계층 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Stage/BBMEncounterDefinition.h"
#include "Combat/Pawn/BBMAICombatantDefinition.h"
#include "Stage/BBMChestActorBase.h"
#include "Stage/BBMEventNpcBase.h"

#define LOCTEXT_NAMESPACE "UBBMEncounterDefinition"

/* UBBMEncounterDefinition 베이스 클래스 구현 */
UBBMEncounterDefinition::UBBMEncounterDefinition() :
	EncounterType(EBBMEncounterType::Combat),
	SpawnSettingKey(NAME_None),
	DisplayName(LOCTEXT("DefaultDisplayName", "기본 만남"))
{
}

FPrimaryAssetId UBBMEncounterDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(BBMPrimaryAssetTypes::Encounter, GetFName());
}

EBBMEncounterType UBBMEncounterDefinition::GetEncounterType() const
{
	return EncounterType;
}

const FName& UBBMEncounterDefinition::GetSpawnSettingKey() const
{
	return SpawnSettingKey;
}

const FText& UBBMEncounterDefinition::GetDisplayName() const
{
	return DisplayName;
}

/* UBBMCombatEncounterDefinition 전투 파생 클래스 구현 */
UBBMCombatEncounterDefinition::UBBMCombatEncounterDefinition()
{
	EncounterType = EBBMEncounterType::Combat;
	DisplayName = LOCTEXT("CombatDisplayName", "전투 만남");
}

const TArray<TSoftObjectPtr<UBBMAICombatantDefinition>>& UBBMCombatEncounterDefinition::GetMonsterList() const
{
	return MonsterList;
}

/* UBBMTreasureEncounterDefinition 보물 파생 클래스 구현 */
UBBMTreasureEncounterDefinition::UBBMTreasureEncounterDefinition() :
	ChestClass(nullptr)
{
	EncounterType = EBBMEncounterType::Treasure;
	DisplayName = LOCTEXT("TreasureDisplayName", "보물 만남");
}

const TSoftClassPtr<ABBMChestActorBase>& UBBMTreasureEncounterDefinition::GetChestClass() const
{
	return ChestClass;
}

/* UBBMEventEncounterDefinition 사람/모험가 조우 파생 클래스 구현 */
UBBMEventEncounterDefinition::UBBMEventEncounterDefinition()
{
	EncounterType = EBBMEncounterType::Event;
	DisplayName = LOCTEXT("EventDisplayName", "이벤트 만남");
}

const TArray<TSoftClassPtr<ABBMEventNpcBase>>& UBBMEventEncounterDefinition::GetNpcClasses() const
{
	return NpcClasses;
}

#undef LOCTEXT_NAMESPACE
