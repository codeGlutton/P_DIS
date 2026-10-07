#include "Enhancement/DISEnhancementContainer.h"
#include "Enhancement/DISEnhancement.h"
#include "Enhancement/DISEnhancementEffect.h"
#include "Enhancement/DISEnhanceableInterface.h"
#include "Global/DISGlobalSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UDISEnhancementContainer::UDISEnhancementContainer() :
	OwnerInterface(nullptr),
	EffectTarget(nullptr)
{
}

void UDISEnhancementContainer::Initialize(const TScriptInterface<IDISEnhanceableInterface>& InOwner, UObject* InEffectTarget)
{
	OwnerInterface = InOwner;
	SetEffectTarget(InEffectTarget);
}

TScriptInterface<IDISEnhanceableInterface> UDISEnhancementContainer::GetOwnerInterface() const
{
	if (OwnerInterface.GetObject() != nullptr)
	{
		return OwnerInterface;
	}

	// 명시적 소유자가 지정되지 않은 경우 Outer 객체로부터 인터페이스 구현 여부 탐색
	UObject* OuterObject = GetOuter();
	if (OuterObject != nullptr && OuterObject->Implements<UDISEnhanceableInterface>() == true)
	{
		TScriptInterface<IDISEnhanceableInterface> ResolvedInterface;
		ResolvedInterface.SetObject(OuterObject);
		ResolvedInterface.SetInterface(Cast<IDISEnhanceableInterface>(OuterObject));
		return ResolvedInterface;
	}

	return nullptr;
}

void UDISEnhancementContainer::SetEffectTarget(UObject* InEffectTarget)
{
	if (EffectTarget.Get() == InEffectTarget)
	{
		return;
	}

	UObject* OldTarget = EffectTarget.Get();
	EffectTarget = InEffectTarget;

	OnEffectTargetChange(OldTarget, InEffectTarget);
}

UObject* UDISEnhancementContainer::GetEffectTarget() const
{
	return EffectTarget.Get();
}

FDISActiveEnhancementEffectHandle UDISEnhancementContainer::ApplyEnhancementEffectByClass(TSubclassOf<UDISEnhancementEffect> InEffectClass)
{
	if (InEffectClass == nullptr)
	{
		return FDISActiveEnhancementEffectHandle();
	}

	return ApplyEnhancementEffectSpec(FDISEnhancementEffectSpec(InEffectClass));
}

FDISActiveEnhancementEffectHandle UDISEnhancementContainer::ApplyEnhancementEffectSpec(const FDISEnhancementEffectSpec& InSpec)
{
	if (InSpec.IsValid() == false)
	{
		return FDISActiveEnhancementEffectHandle();
	}

	const FDISActiveEnhancementEffectHandle NewHandle = AllocateEffectHandle();
	if (NewHandle.IsValid() == false)
	{
		return FDISActiveEnhancementEffectHandle();
	}

	FDISActiveEnhancementEffect NewEffect(InSpec, NewHandle);
	ActiveEffects.Add(NewEffect);

	// 영향을 받는 대상 강화 스탯들에 모디파이어 추가 및 라이프사이클 전파
	for (const auto& Pair : NewEffect.GetCalculatedModifiers())
	{
		AddModifierToEnhancement(Pair.Key, Pair.Value.LevelOp, NewHandle, Pair.Value.LevelMagnitude);
	}

	return NewHandle;
}

bool UDISEnhancementContainer::RemoveEnhancementEffect(const FDISActiveEnhancementEffectHandle& InHandle)
{
	if (InHandle.IsValid() == false)
	{
		return false;
	}

	for (int32 Index = 0; Index < ActiveEffects.Num(); ++Index)
	{
		if (ActiveEffects[Index].GetHandle() == InHandle)
		{
			const FDISActiveEnhancementEffect EffectToRemove = ActiveEffects[Index];
			ActiveEffects.RemoveAtSwap(Index);

			ReleaseEffectHandle(InHandle);

			// 영향받은 모든 대상 강화 스탯에서 모디파이어 제거 및 라이프사이클 처리
			for (const auto& Pair : EffectToRemove.GetCalculatedModifiers())
			{
				RemoveModifierFromEnhancement(Pair.Key, InHandle);
			}

			return true;
		}
	}

	return false;
}

int32 UDISEnhancementContainer::RemoveEnhancementEffectsByClass(TSubclassOf<UDISEnhancementEffect> InEffectClass)
{
	if (InEffectClass == nullptr)
	{
		return 0;
	}

	TArray<FDISActiveEnhancementEffectHandle> HandlesToRemove;
	for (const FDISActiveEnhancementEffect& Effect : ActiveEffects)
	{
		if (Effect.GetEffectClass() == InEffectClass)
		{
			HandlesToRemove.Add(Effect.GetHandle());
		}
	}

	int32 RemovedTotalCount = 0;
	for (const FDISActiveEnhancementEffectHandle& Handle : HandlesToRemove)
	{
		if (RemoveEnhancementEffect(Handle) == true)
		{
			RemovedTotalCount++;
		}
	}

	return RemovedTotalCount;
}

const FDISActiveEnhancementEffect* UDISEnhancementContainer::FindActiveEnhancementEffect(const FDISActiveEnhancementEffectHandle& Handle) const
{
	return const_cast<UDISEnhancementContainer*>(this)->FindActiveEnhancementEffect(Handle);
}

bool UDISEnhancementContainer::K2_FindActiveEnhancementEffect(const FDISActiveEnhancementEffectHandle& Handle, FDISActiveEnhancementEffect& OutActiveEffect) const
{
	if (const FDISActiveEnhancementEffect* Found = FindActiveEnhancementEffect(Handle))
	{
		OutActiveEffect = *Found;
		return true;
	}

	OutActiveEffect = FDISActiveEnhancementEffect();
	return false;
}

const TArray<FDISActiveEnhancementEffect>& UDISEnhancementContainer::GetActiveEnhancementEffects() const
{
	return ActiveEffects;
}

const FDISActiveEnhancement* UDISEnhancementContainer::FindActiveEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass) const
{
	return const_cast<UDISEnhancementContainer*>(this)->FindActiveEnhancement(InEnhancementClass);
}

bool UDISEnhancementContainer::K2_FindActiveEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass, FDISActiveEnhancement& OutActiveEnhancement) const
{
	if (const FDISActiveEnhancement* Found = FindActiveEnhancement(InEnhancementClass))
	{
		OutActiveEnhancement = *Found;
		return true;
	}

	OutActiveEnhancement = FDISActiveEnhancement();
	return false;
}

const TArray<FDISActiveEnhancement>& UDISEnhancementContainer::GetActiveEnhancements() const
{
	return ActiveEnhancements;
}

void UDISEnhancementContainer::RecalculateEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass)
{
	
}

FText UDISEnhancementContainer::GetEnhancedDisplayName(const FText& BaseName) const
{
	// 1. 유효한 활성 강화의 접두사와 접미사를 수집
	struct FAffixSortEntry
	{
		FText Text;
		int32 Priority = 0;
		int32 OrderIndex = 0;
	};

	TArray<FAffixSortEntry> ValidPrefixes;
	TArray<FAffixSortEntry> ValidSuffixes;

	for (int32 Index = 0; Index < ActiveEnhancements.Num(); ++Index)
	{
		const FDISActiveEnhancement& ActiveEnhancement = ActiveEnhancements[Index];
		const UDISEnhancement* CDO = ActiveEnhancement.GetEnhancementCDO();
		if (CDO == nullptr || ActiveEnhancement.bIsActive == false || ActiveEnhancement.GetLevel() <= 0)
		{
			continue;
		}

		const FText PrefixText = CDO->GetPrefixText(ActiveEnhancement.GetLevel());
		if (PrefixText.IsEmptyOrWhitespace() == false)
		{
			ValidPrefixes.Add({ PrefixText, CDO->GetPrefixPriority(), Index });
		}

		const FText SuffixText = CDO->GetSuffixText(ActiveEnhancement.GetLevel());
		if (SuffixText.IsEmptyOrWhitespace() == false)
		{
			ValidSuffixes.Add({ SuffixText, CDO->GetSuffixPriority(), Index });
		}
	}

	// 수식어가 없는 경우 기본 이름 반환
	if (ValidPrefixes.IsEmpty() == true && ValidSuffixes.IsEmpty() == true)
	{
		return BaseName;
	}

	// 2. 우선순위 정렬 (우선순위 내림차순: 높은 값이 앞쪽, 동률 시 등록 순서 유지)
	auto AffixSorter = [](const FAffixSortEntry& A, const FAffixSortEntry& B)
	{
		if (A.Priority != B.Priority)
		{
			return A.Priority > B.Priority;
		}
		return A.OrderIndex < B.OrderIndex;
	};

	ValidPrefixes.Sort(AffixSorter);
	ValidSuffixes.Sort(AffixSorter);

	// 3. 접두사 결합 (우선순위 높은 것이 맨 앞쪽)
	FString ResultString;
	for (const FAffixSortEntry& Entry : ValidPrefixes)
	{
		ResultString.Append(Entry.Text.ToString());
		ResultString.Append(TEXT(" "));
	}

	// 4. 본래 이름 추가
	ResultString.Append(BaseName.ToString());

	// 5. 접미사 결합 (우선순위 높은 것이 기본 이름 바로 뒤쪽)
	for (const FAffixSortEntry& Entry : ValidSuffixes)
	{
		ResultString.Append(TEXT(" "));
		ResultString.Append(Entry.Text.ToString());
	}

	return FText::FromString(ResultString.TrimStartAndEnd());
}

UDISGlobalSubsystem* UDISEnhancementContainer::GetGlobalSubsystem() const
{
	const UObject* CurrentOuter = this;
	while (CurrentOuter != nullptr)
	{
		if (const UWorld* World = CurrentOuter->GetWorld())
		{
			if (const UGameInstance* GI = World->GetGameInstance())
			{
				return GI->GetSubsystem<UDISGlobalSubsystem>();
			}
		}
		CurrentOuter = CurrentOuter->GetOuter();
	}
	return nullptr;
}

FDISActiveEnhancementEffectHandle UDISEnhancementContainer::AllocateEffectHandle()
{
	if (UDISGlobalSubsystem* GlobalSubsystem = GetGlobalSubsystem())
	{
		return GlobalSubsystem->AllocateEnhancementEffectHandle(this);
	}

	return FDISActiveEnhancementEffectHandle();
}

void UDISEnhancementContainer::ReleaseEffectHandle(const FDISActiveEnhancementEffectHandle& Handle)
{
	if (Handle.IsValid() == false)
	{
		return;
	}

	if (UDISGlobalSubsystem* GlobalSubsystem = GetGlobalSubsystem())
	{
		GlobalSubsystem->ReleaseEnhancementEffectHandle(Handle);
	}
}

FDISActiveEnhancementEffect* UDISEnhancementContainer::FindActiveEnhancementEffect(const FDISActiveEnhancementEffectHandle& Handle)
{
	if (Handle.IsValid() == false)
	{
		return nullptr;
	}

	for (FDISActiveEnhancementEffect& Effect : ActiveEffects)
	{
		if (Effect.GetHandle() == Handle)
		{
			return &Effect;
		}
	}

	return nullptr;
}

FDISActiveEnhancement* UDISEnhancementContainer::FindActiveEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass)
{
	if (InEnhancementClass == nullptr)
	{
		return nullptr;
	}

	for (FDISActiveEnhancement& Existing : ActiveEnhancements)
	{
		if (Existing.GetEnhancementClass() == InEnhancementClass)
		{
			return &Existing;
		}
	}

	return nullptr;
}

void UDISEnhancementContainer::AddModifierToEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass, EDISEnhancementLevelOp InLevelOp, const FDISActiveEnhancementEffectHandle& InHandle, float InMagnitude)
{
	if (InEnhancementClass == nullptr)
	{
		return;
	}

	bool bWasCreated = false;
	FDISActiveEnhancement* FoundEnhancement = FindActiveEnhancement(InEnhancementClass);
	if (FoundEnhancement == nullptr)
	{
		FoundEnhancement = RegisterEnhancement(InEnhancementClass);
		if (FoundEnhancement == nullptr)
		{
			return;
		}
	}

	const int32 OldLevel = FoundEnhancement->GetLevel();
	FoundEnhancement->AddModifier(InLevelOp, InHandle, InMagnitude);
	const int32 NewLevel = FoundEnhancement->GetLevel();

	OnEnhancementLevelChange(*FoundEnhancement, OldLevel, NewLevel);
}

void UDISEnhancementContainer::RemoveModifierFromEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass, const FDISActiveEnhancementEffectHandle& InHandle)
{
	if (InEnhancementClass == nullptr || InHandle.IsValid() == false)
	{
		return;
	}

	FDISActiveEnhancement* FoundEnhancement = FindActiveEnhancement(InEnhancementClass);
	if (FoundEnhancement == nullptr)
	{
		return;
	}

	const int32 OldLevel = FoundEnhancement->GetLevel();
	FoundEnhancement->RemoveModifier(InHandle);
	const int32 NewLevel = FoundEnhancement->GetLevel();

	OnEnhancementLevelChange(*FoundEnhancement, OldLevel, NewLevel);

	// 영향을 주는 모디파이어가 더 이상 남아있지 않으면 컨테이너에서 완전 해제
	if (FoundEnhancement->HasAnyModifiers() == false)
	{
		UnregisterEnhancement(InEnhancementClass);
	}
}

FDISActiveEnhancement* UDISEnhancementContainer::RegisterEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass)
{
	if (InEnhancementClass == nullptr)
	{
		return nullptr;
	}

	int32 FoundIndex = INDEX_NONE;
	for (int32 Index = 0; Index < ActiveEnhancements.Num(); ++Index)
	{
		if (ActiveEnhancements[Index].GetEnhancementClass() == InEnhancementClass)
		{
			FoundIndex = Index;
			break;
		}
	}

	if (FoundIndex != INDEX_NONE)
	{
		return &ActiveEnhancements[FoundIndex];
	}

	FDISActiveEnhancement& FoundEnhancement = ActiveEnhancements.Add_GetRef(FDISActiveEnhancement(InEnhancementClass));
	UObject* CurrentTarget = GetEffectTarget();
	const UDISEnhancement* CDO = FoundEnhancement.GetEnhancementCDO();

	// 스탯이 컨테이너에 최초 등록된 경우 OnEnhancementAdded 호출
	if (CDO != nullptr && CurrentTarget != nullptr)
	{
		CDO->OnEnhancementAdded(CurrentTarget, FoundEnhancement);
	}

	return &FoundEnhancement;
}

void UDISEnhancementContainer::UnregisterEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass)
{
	if (InEnhancementClass == nullptr)
	{
		return;
	}

	int32 FoundIndex = INDEX_NONE;
	for (int32 Index = 0; Index < ActiveEnhancements.Num(); ++Index)
	{
		if (ActiveEnhancements[Index].GetEnhancementClass() == InEnhancementClass)
		{
			FoundIndex = Index;
			break;
		}
	}

	if (FoundIndex == INDEX_NONE)
	{
		return;
	}

	FDISActiveEnhancement& FoundEnhancement = ActiveEnhancements[FoundIndex];
	const UDISEnhancement* CDO = FoundEnhancement.GetEnhancementCDO();
	UObject* CurrentTarget = GetEffectTarget();

	// 스탯 완전 제거 훅 호출
	if (CDO != nullptr && CurrentTarget != nullptr)
	{
		CDO->OnEnhancementRemoved(CurrentTarget, FoundEnhancement);
	}

	ActiveEnhancements.RemoveAtSwap(FoundIndex);
}

void UDISEnhancementContainer::OnEnhancementLevelChange(FDISActiveEnhancement& Enhancement, int32 OldLevel, int32 NewLevel)
{
	const UDISEnhancement* CDO = Enhancement.GetEnhancementCDO();
	UObject* CurrentTarget = GetEffectTarget();

	if (CurrentTarget != nullptr && CDO != nullptr)
	{
		if (OldLevel == 0 && NewLevel > 0)
		{
			// 신규 활성화 (0 -> 양수)
			Enhancement.bIsActive = true;
			CDO->OnEnhancementActivated(CurrentTarget, Enhancement);
			CDO->OnEnhancementLevelChanged(CurrentTarget, Enhancement, 0, NewLevel);
		}
		else if (OldLevel > 0 && NewLevel > 0)
		{
			// 활성 중 레벨 수치 변경
			if (OldLevel != NewLevel)
			{
				CDO->OnEnhancementLevelChanged(CurrentTarget, Enhancement, OldLevel, NewLevel);
			}
		}
		else if (OldLevel > 0 && NewLevel <= 0)
		{
			// 완전 비활성화 (양수 -> 0)
			CDO->OnEnhancementLevelChanged(CurrentTarget, Enhancement, OldLevel, 0);
			CDO->OnEnhancementDeactivated(CurrentTarget, Enhancement);
			Enhancement.bIsActive = false;
		}
	}
	else
	{
		Enhancement.bIsActive = (NewLevel > 0);
	}
}

void UDISEnhancementContainer::OnEffectTargetChange(UObject* OldTarget, UObject* NewTarget)
{
	// 이전 대상에서 활성화되어 있던 모든 강화 스탯 일괄 비활성화 및 해제
	if (OldTarget != nullptr)
	{
		for (const FDISActiveEnhancement& ActiveEnhancement : ActiveEnhancements)
		{
			const UDISEnhancement* CDO = ActiveEnhancement.GetEnhancementCDO();
			if (CDO == nullptr)
			{
				continue;
			}

			if (ActiveEnhancement.GetLevel() > 0)
			{
				CDO->OnEnhancementLevelChanged(OldTarget, ActiveEnhancement, ActiveEnhancement.GetLevel(), 0);
				CDO->OnEnhancementDeactivated(OldTarget, ActiveEnhancement);
			}
			CDO->OnEnhancementRemoved(OldTarget, ActiveEnhancement);
		}
	}

	// 신규 대상에게 활성 강화 스탯 등록 및 활성화 갱신
	if (NewTarget != nullptr)
	{
		for (FDISActiveEnhancement& ActiveEnhancement : ActiveEnhancements)
		{
			const UDISEnhancement* CDO = ActiveEnhancement.GetEnhancementCDO();
			if (CDO == nullptr)
			{
				return;
			}

			CDO->OnEnhancementAdded(NewTarget, ActiveEnhancement);
			if (ActiveEnhancement.GetLevel() > 0)
			{
				CDO->OnEnhancementActivated(NewTarget, ActiveEnhancement);
				CDO->OnEnhancementLevelChanged(NewTarget, ActiveEnhancement, 0, ActiveEnhancement.GetLevel());
			}
		}
	}
}

