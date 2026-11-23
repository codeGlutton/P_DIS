#include "Item/DISItemEntry.h"
#include "Item/DISItemDefinition.h"
#include "Enhancement/DISEnhancementContainer.h"

UDISItemEntry::UDISItemEntry() :
	ItemDefinition(nullptr),
	CurrentDurability(0.0f),
	EnhancementContainer(nullptr),
	PlacementRules()
{
}

void UDISItemEntry::InitializeEntry_Implementation(const UDISItemDefinition* InItemDefinition, float InDurability)
{
	ItemDefinition = InItemDefinition;

	if (ItemDefinition == nullptr)
	{
		CurrentDurability = 0.0f;
		PlacementRules.Empty();
		return;
	}

	// 정적 정의 에셋의 기본 배치 제한 규칙 복사
	PlacementRules = ItemDefinition->DefaultPlacementRules;

	// 내구도가 음수로 전달된 경우 정적 정의 에셋의 최대 내구도로 기본 초기화
	if (InDurability < 0.0f)
	{
		CurrentDurability = ItemDefinition->MaxDurability;
	}
	else
	{
		SetCurrentDurability(InDurability);
	}

	// 강화 컨테이너가 아직 생성되지 않았다면 동적 생성 및 소유자 바인딩
	if (EnhancementContainer == nullptr)
	{
		EnhancementContainer = NewObject<UDISEnhancementContainer>(this);

		TScriptInterface<IDISEnhanceableInterface> OwnerInterface;
		OwnerInterface.SetObject(this);
		OwnerInterface.SetInterface(Cast<IDISEnhanceableInterface>(this));

		// 소유자는 아이템 엔트리(this), 실제 효과 적용 대상(예: 장착 캐릭터)은 장착 시점에 SetEffectTarget으로 연동하도록 초기화
		EnhancementContainer->Initialize(OwnerInterface, nullptr);
	}
}

UDISEnhancementContainer* UDISItemEntry::GetEnhancementContainer() const
{
	return EnhancementContainer.Get();
}

const UDISItemDefinition* UDISItemEntry::GetItemDefinition() const
{
	return ItemDefinition.Get();
}

float UDISItemEntry::GetCurrentDurability() const
{
	return CurrentDurability;
}

void UDISItemEntry::SetCurrentDurability(float NewDurability)
{
	if (ItemDefinition == nullptr)
	{
		CurrentDurability = FMath::Max(0.0f, NewDurability);
		return;
	}

	// 0.0부터 아이템 최대 내구도 범위 내로 제한
	CurrentDurability = FMath::Clamp(NewDurability, 0.0f, ItemDefinition->MaxDurability);
}

FText UDISItemEntry::GetEnhancedDisplayName() const
{
	if (ItemDefinition == nullptr)
	{
		return FText::GetEmpty();
	}

	if (EnhancementContainer == nullptr)
	{
		return ItemDefinition->ItemName;
	}

	// 수식어(Prefix/Suffix) 결합 처리를 강화 전담 컨테이너에게 위임
	return EnhancementContainer->GetEnhancedDisplayName(ItemDefinition->ItemName);
}

void UDISItemEntry::AddPlacementRule(const TInstancedStruct<FDISItemPlacementRule>& InRule)
{
	if (InRule.IsValid() == true)
	{
		PlacementRules.Add(InRule);
	}
}

void UDISItemEntry::ClearPlacementRules()
{
	PlacementRules.Empty();
}

const TArray<TInstancedStruct<FDISItemPlacementRule>>& UDISItemEntry::GetPlacementRules() const
{
	return PlacementRules;
}

TArray<TInstancedStruct<FDISItemPlacementRule>>& UDISItemEntry::GetMutablePlacementRules()
{
	return PlacementRules;
}

bool UDISItemEntry::CanPlaceAtSlot(const FDISItemPlacementContext& Context) const
{
	// 등록된 모든 배치 제한 규칙 검사 (모든 규칙을 통과해야 함)
	for (const TInstancedStruct<FDISItemPlacementRule>& Rule : PlacementRules)
	{
		if (Rule.IsValid() == true)
		{
			const FDISItemPlacementRule& RuleRef = Rule.Get<FDISItemPlacementRule>();
			if (RuleRef.CanPlaceItem(this, Context) == false)
			{
				return false;
			}
		}
	}

	return true;
}

bool UDISItemEntry::CanStackWith(const UDISItemEntry* OtherEntry) const
{
	if (OtherEntry == nullptr || ItemDefinition == nullptr || OtherEntry->ItemDefinition == nullptr)
	{
		return false;
	}

	// 1. 기본 아이템 정의 에셋 일치 여부
	if (ItemDefinition != OtherEntry->ItemDefinition)
	{
		return false;
	}

	// 2. 강화 상태(강화 종류 및 각 레벨) 완전 일치 여부 (내구도는 무관)
	const UDISEnhancementContainer* MyContainer = GetEnhancementContainer();
	const UDISEnhancementContainer* OtherContainer = OtherEntry->GetEnhancementContainer();

	if (MyContainer != nullptr)
	{
		if (MyContainer->HasIdenticalEnhancements(OtherContainer) == false)
		{
			return false;
		}
	}
	else if (OtherContainer != nullptr)
	{
		if (OtherContainer->HasIdenticalEnhancements(nullptr) == false)
		{
			return false;
		}
	}

	return true;
}

bool UDISItemEntry::CanRotate() const
{
	return ItemDefinition != nullptr ? ItemDefinition->bCanRotate : false;
}

EDISItemRotation UDISItemEntry::GetInitialRotation() const
{
	return ItemDefinition != nullptr ? ItemDefinition->InitialRotation : EDISItemRotation::Rotate0;
}

