#include "Combat/Animation/BBMCombatantAnimInstance.h"
#include "Combat/Pawn/BBMCombatantBase.h"

UBBMCombatantAnimInstance::UBBMCombatantAnimInstance()
{
}

void UBBMCombatantAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 소유 폰(ABBMCombatantBase)의 애니메이션 태그 큐에서 1개씩 Dequeue하여 반영
	if (APawn* OwningPawn = TryGetPawnOwner())
	{
		if (ABBMCombatantBase* Combatant = Cast<ABBMCombatantBase>(OwningPawn))
		{
			FGameplayTag NextTag;
			if (Combatant->DequeueAnimationTag(NextTag) == true)
			{
				CurrentAnimationStateTag = NextTag;
			}
			else
			{
				CurrentAnimationStateTag = FGameplayTag::EmptyTag;
			}
		}
	}
}

FGameplayTag UBBMCombatantAnimInstance::GetCurrentAnimationStateTag() const
{
	return CurrentAnimationStateTag;
}

void UBBMCombatantAnimInstance::SetCurrentAnimationStateTag(const FGameplayTag& InTag)
{
	CurrentAnimationStateTag = InTag;
}

void UBBMCombatantAnimInstance::ClearAnimationStateTag()
{
	CurrentAnimationStateTag = FGameplayTag::EmptyTag;
}

TSubclassOf<UAnimInstance> UBBMCombatantAnimInstance::FindAnimLayerClass(const FGameplayTag& LayerTag) const
{
	if (LayerTag.IsValid() == false)
	{
		return nullptr;
	}

	if (const TSubclassOf<UAnimInstance>* FoundClass = AnimLayerMap.Find(LayerTag))
	{
		return *FoundClass;
	}

	return nullptr;
}

bool UBBMCombatantAnimInstance::HasAnimLayer(const FGameplayTag& LayerTag) const
{
	if (LayerTag.IsValid() == false)
	{
		return false;
	}

	return AnimLayerMap.Contains(LayerTag);
}

const TMap<FGameplayTag, TSubclassOf<UAnimInstance>>& UBBMCombatantAnimInstance::GetAnimLayerMap() const
{
	return AnimLayerMap;
}

