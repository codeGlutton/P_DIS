/*****************************************************************//**
 * @file   BBMCombatantBase.cpp
 * @brief  BBM(비트 배낭 상인) 전투원 캐릭터 베이스 폰 구현
 * @date   2026-10-31
 *********************************************************************/

#include "Combat/BBMCombatantBase.h"
#include "Combat/BBMAttributeSet.h"
#include "Combat/BBMCharacterDefinition.h"
#include "Combat/BBMAbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

ABBMCombatantBase::ABBMCombatantBase() :
	AbilitySystemComponent(nullptr),
	AttributeSet(nullptr),
	CharacterDefinition(nullptr),
	TeamId(FGenericTeamId(static_cast<uint8>(EBBMTeamType::Player))),
	CharacterLevel(1),
	LaneIndex(0)
{
	PrimaryActorTick.bCanEverTick = false;

	// 언리얼 GAS 서브오브젝트 생성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UBBMAttributeSet>(TEXT("AttributeSet"));
}

void ABBMCombatantBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (AbilitySystemComponent != nullptr)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (CharacterDefinition != nullptr)
	{
		SetGenericTeamId(FGenericTeamId(static_cast<uint8>(CharacterDefinition->GetDefaultTeam())));
	}
}

UAbilitySystemComponent* ABBMCombatantBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABBMCombatantBase::SetGenericTeamId(const FGenericTeamId& InTeamId)
{
	TeamId = InTeamId;
}

FGenericTeamId ABBMCombatantBase::GetGenericTeamId() const
{
	return TeamId;
}

ETeamAttitude::Type ABBMCombatantBase::GetTeamAttitudeTowards(const AActor& Other) const
{
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);
	if (OtherTeamAgent != nullptr)
	{
		return FGenericTeamId::GetAttitude(GetGenericTeamId(), OtherTeamAgent->GetGenericTeamId());
	}
	return ETeamAttitude::Neutral;
}

const UBBMCharacterDefinition* ABBMCombatantBase::GetCharacterDefinition() const
{
	return CharacterDefinition;
}

void ABBMCombatantBase::SetCharacterDefinition(const UBBMCharacterDefinition* InDef)
{
	CharacterDefinition = InDef;
	if (InDef != nullptr)
	{
		SetTeamType(InDef->GetDefaultTeam());
	}
}

UBBMAttributeSet* ABBMCombatantBase::GetAttributeSet() const
{
	return AttributeSet;
}

void ABBMCombatantBase::InitializeStats()
{
	InitializeStatsForLevel(CharacterLevel);
}

void ABBMCombatantBase::InitializeStatsForLevel(int32 InLevel)
{
	if (AbilitySystemComponent == nullptr)
	{
		return;
	}

	CharacterLevel = FMath::Max(1, InLevel);

	// CharacterDefinition의 고정 TableKeyName을 우선 사용하며, 미지정 시 기본 "Player" 그룹 사용
	const FName TableKey = (CharacterDefinition != nullptr && CharacterDefinition->GetTableKeyName().IsNone() == false)
		? CharacterDefinition->GetTableKeyName()
		: FName(TEXT("Player"));

	UAbilitySystemGlobals& Globals = UAbilitySystemGlobals::Get();
	UBBMAbilitySystemGlobals* BBMGlobals = Cast<UBBMAbilitySystemGlobals>(&Globals);

	if (BBMGlobals != nullptr && BBMGlobals->HasAttributeSetInitter() == false)
	{
		return;
	}

	FAttributeSetInitter* Initter = Globals.GetAttributeSetInitter();
	if (Initter != nullptr)
	{
		// 스탯 갱신 시 이전 MaxHealth에 의해 새 Health가 잘리는 현상을 방지하기 위해 MaxHealth를 임시 실수 최댓값으로 리셋
		if (AttributeSet != nullptr)
		{
			AttributeSet->InitMaxHealth(TNumericLimits<float>::Max());
		}

		Initter->InitAttributeSetDefaults(AbilitySystemComponent, TableKey, CharacterLevel, true);

		if (AttributeSet != nullptr)
		{
			AbilitySystemComponent->SetNumericAttributeBase(
				UBBMAttributeSet::GetHealthAttribute(),
				FMath::Clamp(AttributeSet->GetHealth(), 0.0f, AttributeSet->GetMaxHealth())
			);
		}
	}
}

float ABBMCombatantBase::GetAttributeValue(FGameplayAttribute Attribute) const
{
	if (AbilitySystemComponent != nullptr && AbilitySystemComponent->HasAttributeSetForAttribute(Attribute))
	{
		return AbilitySystemComponent->GetNumericAttribute(Attribute);
	}
	return 0.0f;
}

float ABBMCombatantBase::GetAttributeBaseValue(FGameplayAttribute Attribute) const
{
	if (AbilitySystemComponent != nullptr && AbilitySystemComponent->HasAttributeSetForAttribute(Attribute))
	{
		return AbilitySystemComponent->GetNumericAttributeBase(Attribute);
	}
	return 0.0f;
}

bool ABBMCombatantBase::IsDead() const
{
	return GetAttributeValue(UBBMAttributeSet::GetHealthAttribute()) <= 0.0f;
}

void ABBMCombatantBase::SetTeamType(EBBMTeamType InTeamType)
{
	SetGenericTeamId(FGenericTeamId(static_cast<uint8>(InTeamType)));
}

EBBMTeamType ABBMCombatantBase::GetTeamType() const
{
	return static_cast<EBBMTeamType>(TeamId.GetId());
}

bool ABBMCombatantBase::IsFriendlyTowards(const AActor* Other) const
{
	if (Other == nullptr)
	{
		return false;
	}
	return GetTeamAttitudeTowards(*Other) == ETeamAttitude::Friendly;
}

bool ABBMCombatantBase::IsHostileTowards(const AActor* Other) const
{
	if (Other == nullptr)
	{
		return false;
	}
	return GetTeamAttitudeTowards(*Other) == ETeamAttitude::Hostile;
}

int32 ABBMCombatantBase::GetLaneIndex() const
{
	return LaneIndex;
}

void ABBMCombatantBase::SetLaneIndex(int32 InLaneIndex)
{
	LaneIndex = FMath::Max(0, InLaneIndex);
}
