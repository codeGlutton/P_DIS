#include "Item/DISItemEntry.h"
#include "Item/DISItemDefinition.h"
#include "Enhancement/DISEnhancementContainer.h"

UDISItemEntry::UDISItemEntry() :
	ItemDefinition(nullptr),
	CurrentDurability(0.0f),
	EnhancementContainer(nullptr)
{
}

void UDISItemEntry::InitializeEntry(const UDISItemDefinition* InItemDefinition, float InDurability)
{
	ItemDefinition = InItemDefinition;

	if (ItemDefinition == nullptr)
	{
		CurrentDurability = 0.0f;
		return;
	}

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

