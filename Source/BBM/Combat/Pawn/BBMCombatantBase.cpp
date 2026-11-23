#include "Combat/Pawn/BBMCombatantBase.h"
#include "Combat/GAS/BBMAttributeSet.h"
#include "Combat/Pawn/BBMCharacterDefinition.h"
#include "Combat/GAS/BBMAbilitySystemGlobals.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#if WITH_EDITORONLY_DATA
#include "Components/ArrowComponent.h"
#endif
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Combat/Animation/BBMCombatantAnimInstance.h"

ABBMCombatantBase::ABBMCombatantBase() :
	CapsuleComponent(nullptr),
	Mesh(nullptr),
#if WITH_EDITORONLY_DATA
	ArrowComponent(nullptr),
#endif
	AbilitySystemComponent(nullptr),
	AttributeSet(nullptr),
	CharacterDefinition(nullptr),
	TeamId(FGenericTeamId(static_cast<uint8>(EBBMTeamType::Player))),
	CharacterLevel(1),
	LaneIndex(0)
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 캡슐 콜리전 컴포넌트 생성 및 루트 설정 (언리얼 ACharacter 표준 규격)
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCylinder"));
	CapsuleComponent->InitCapsuleSize(34.0f, 88.0f);
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->bDynamicObstacle = true;
	RootComponent = CapsuleComponent;

#if WITH_EDITORONLY_DATA
	// 2. 에디터 전용 방향 표시 화살표 컴포넌트 생성
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (ArrowComponent != nullptr)
	{
		ArrowComponent->ArrowColor = FColor(150, 200, 255);
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->SetupAttachment(CapsuleComponent);
		ArrowComponent->bIsScreenSizeScaled = true;
	}
#endif

	// 3. 스켈레탈 메시 컴포넌트 생성 및 부착 (언리얼 ACharacter 표준 규격)
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh0"));
	if (Mesh != nullptr)
	{
		Mesh->AlwaysLoadOnClient = true;
		Mesh->AlwaysLoadOnServer = true;
		Mesh->bOwnerNoSee = false;
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		Mesh->bCastDynamicShadow = true;
		Mesh->bAffectDynamicIndirectLighting = true;
		Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Mesh->SetupAttachment(CapsuleComponent);
		static FName MeshCollisionProfileName(TEXT("CharacterMesh"));
		Mesh->SetCollisionProfileName(MeshCollisionProfileName);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
	}

	// 4. 언리얼 GAS 서브오브젝트 생성
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

	// 사전 주입된(DeferredSpawn 등) CharacterDefinition이 있는 경우 초기화 실행
	if (CharacterDefinition != nullptr)
	{
		InitializeFromDefinition();
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
		InitializeFromDefinition();
	}
}

void ABBMCombatantBase::InitializeFromDefinition()
{
	if (CharacterDefinition == nullptr)
	{
		return;
	}

	SetGenericTeamId(FGenericTeamId(static_cast<uint8>(CharacterDefinition->GetDefaultTeam())));

	if (AbilitySystemComponent != nullptr && AbilitySystemComponent->AbilityActorInfo.IsValid())
	{
		InitializeStats();
	}
}

void ABBMCombatantBase::UpdateBeatSequence()
{
	CachedBeatSequence = FBBMBeatSequence();
}

const FBBMBeatSequence& ABBMCombatantBase::GetCurrentBeatSequence() const
{
	return CachedBeatSequence;
}

UBBMAttributeSet* ABBMCombatantBase::GetAttributeSet() const
{
	return AttributeSet;
}

FBBMCombatantSnapshot ABBMCombatantBase::CreateAttributeSnapshot() const
{
	if (AttributeSet != nullptr)
	{
		return AttributeSet->CreateAttributeSnapshot();
	}
	return FBBMCombatantSnapshot();
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

	// BBMGlobals 기준 Initter 미할당 또는 CurveTable 미등록 시 초기화 건너뜀 (에러 로그 방지)
	if (BBMGlobals != nullptr && (BBMGlobals->HasAttributeSetInitter() == false || BBMGlobals->HasAttributeDefaultTables() == false))
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

		if (AttributeSet != nullptr && AbilitySystemComponent->HasAttributeSetForAttribute(UBBMAttributeSet::GetHealthAttribute()))
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

UCapsuleComponent* ABBMCombatantBase::GetCapsuleComponent() const
{
	return CapsuleComponent;
}

USkeletalMeshComponent* ABBMCombatantBase::GetMesh() const
{
	return Mesh;
}

void ABBMCombatantBase::LinkAnimLayerByTag(const FGameplayTag& LayerTag)
{
	if (Mesh == nullptr || LayerTag.IsValid() == false)
	{
		return;
	}

	if (UBBMCombatantAnimInstance* CombatantAnimInst = Cast<UBBMCombatantAnimInstance>(Mesh->GetAnimInstance()))
	{
		if (TSubclassOf<UAnimInstance> LayerClass = CombatantAnimInst->FindAnimLayerClass(LayerTag))
		{
			Mesh->LinkAnimClassLayers(LayerClass);
		}
	}
}

void ABBMCombatantBase::UnlinkAnimLayerByTag(const FGameplayTag& LayerTag)
{
	if (Mesh == nullptr || LayerTag.IsValid() == false)
	{
		return;
	}

	if (UBBMCombatantAnimInstance* CombatantAnimInst = Cast<UBBMCombatantAnimInstance>(Mesh->GetAnimInstance()))
	{
		if (TSubclassOf<UAnimInstance> LayerClass = CombatantAnimInst->FindAnimLayerClass(LayerTag))
		{
			Mesh->UnlinkAnimClassLayers(LayerClass);
		}
	}
}

void ABBMCombatantBase::LinkAnimLayers(const TArray<TSubclassOf<UAnimInstance>>& InLayers)
{
	if (Mesh == nullptr)
	{
		return;
	}

	for (const TSubclassOf<UAnimInstance>& LayerClass : InLayers)
	{
		if (LayerClass != nullptr)
		{
			Mesh->LinkAnimClassLayers(LayerClass);
		}
	}
}

void ABBMCombatantBase::UnlinkAnimLayers(const TArray<TSubclassOf<UAnimInstance>>& InLayers)
{
	if (Mesh == nullptr)
	{
		return;
	}

	for (const TSubclassOf<UAnimInstance>& LayerClass : InLayers)
	{
		if (LayerClass != nullptr)
		{
			Mesh->UnlinkAnimClassLayers(LayerClass);
		}
	}
}

void ABBMCombatantBase::StartDissolve()
{
	// [TODO] 디졸브 머티리얼 파라미터 애니메이션 또는 타임라인 연출 구현 예정 (현재는 껍데기 가상 함수로 유지)
}

void ABBMCombatantBase::EnqueueAnimationTag(const FGameplayTag& InTag)
{
	if (InTag.IsValid() == true)
	{
		AnimationTagQueue.Enqueue(InTag);
	}
}

bool ABBMCombatantBase::DequeueAnimationTag(FGameplayTag& OutTag)
{
	return AnimationTagQueue.Dequeue(OutTag);
}

bool ABBMCombatantBase::HasPendingAnimationTags() const
{
	return AnimationTagQueue.IsEmpty() == false;
}

void ABBMCombatantBase::ClearAnimationTags()
{
	AnimationTagQueue.Empty();
}

#if WITH_EDITORONLY_DATA
UArrowComponent* ABBMCombatantBase::GetArrowComponent() const
{
	return ArrowComponent;
}
#endif
