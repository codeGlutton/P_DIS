#include "Item/DISItemStack.h"
#include "Item/DISItemEntry.h"
#include "Item/DISItemDefinition.h"
#include "Inventory/DISInventory.h"
#include "Global/DISGlobalSubsystem.h"
#include "Global/DISGlobalLogic.h"
#include "Global/DISGlobalSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

namespace
{
	/* 아이템 엔트리의 최대 스택 허용 수량 조회 헬퍼 */
	int32 GetItemMaxStackCount(const UDISItemEntry* InEntry)
	{
		if (InEntry == nullptr || InEntry->GetItemDefinition() == nullptr)
		{
			return 1;
		}

		const FGameplayTag& TypeTag = InEntry->GetItemDefinition()->ItemTypeTag;

		// 1. 활성 GameInstance의 전역 로직 서브오브젝트에서 프로파일 검색 시도
		if (GEngine != nullptr)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UGameInstance* GameInstance = Context.OwningGameInstance;
				if (GameInstance != nullptr)
				{
					UDISGlobalLogic* Logic = UDISGlobalSubsystem::GetLogic(GameInstance);
					if (Logic != nullptr)
					{
						return Logic->GetItemTypeProfile(TypeTag).GetCustomDataChecked().MaxStackCount;
					}
				}
			}
		}

		// 2. 전역 설정의 기본 프로파일 폴백
		const UDISGlobalSettings* Settings = GetDefault<UDISGlobalSettings>();
		if (Settings != nullptr)
		{
			return Settings->DefaultItemProfile.GetCustomDataChecked().MaxStackCount;
		}

		return 1;
	}
}

UDISItemStack::UDISItemStack() :
	Entries(),
	Rotation(EDISItemRotation::Rotate0),
	OwningInventory(nullptr)
{
}

bool UDISItemStack::IsEmpty() const
{
	return Entries.IsEmpty() == true;
}

int32 UDISItemStack::GetStackCount() const
{
	return Entries.Num();
}

int32 UDISItemStack::GetMaxStackCount() const
{
	const UDISItemEntry* Top = GetTopEntry();
	if (Top != nullptr)
	{
		return GetItemMaxStackCount(Top);
	}

	return 1;
}

UDISItemEntry* UDISItemStack::GetTopEntry() const
{
	if (Entries.IsEmpty() == true)
	{
		return nullptr;
	}

	return Entries[0].Get();
}

const TArray<TObjectPtr<UDISItemEntry>>& UDISItemStack::GetEntries() const
{
	return Entries;
}

TArray<UDISItemEntry*> UDISItemStack::K2_GetEntries() const
{
	TArray<UDISItemEntry*> Result;
	Result.Reserve(Entries.Num());

	for (const TObjectPtr<UDISItemEntry>& Entry : Entries)
	{
		if (Entry != nullptr)
		{
			Result.Add(Entry.Get());
		}
	}

	return Result;
}

float UDISItemStack::GetTotalWeight() const
{
	float TotalWeight = 0.0f;
	for (const TObjectPtr<UDISItemEntry>& Entry : Entries)
	{
		if (Entry != nullptr && Entry->GetItemDefinition() != nullptr)
		{
			TotalWeight += Entry->GetItemDefinition()->Weight;
		}
	}

	return TotalWeight;
}

bool UDISItemStack::CanStackWith(const UDISItemEntry* InEntry) const
{
	if (InEntry == nullptr || InEntry->GetItemDefinition() == nullptr)
	{
		return false;
	}

	// 스택이 비어있는 경우 어떤 아이템이든 배치 가능
	if (IsEmpty() == true)
	{
		return true;
	}

	const UDISItemEntry* Top = GetTopEntry();
	if (Top == nullptr)
	{
		return true;
	}

	// 1. 아이템 정의 및 강화 일치 여부 검증 (내구도는 무관)
	if (Top->CanStackWith(InEntry) == false)
	{
		return false;
	}

	// 2. 아이템 타입의 최대 허용 스택 수량 검증
	const int32 MaxStack = GetItemMaxStackCount(InEntry);
	if (MaxStack <= 1)
	{
		// 1 이하인 경우 스택 불가
		return false;
	}

	if (Entries.Num() >= MaxStack)
	{
		// 스택 한도 초과
		return false;
	}

	return true;
}

bool UDISItemStack::PushEntry(UDISItemEntry* InEntry)
{
	if (InEntry == nullptr)
	{
		return false;
	}

	if (CanStackWith(InEntry) == false)
	{
		return false;
	}

	const bool bWasEmpty = IsEmpty();

	// 내구도 오름차순(Ascending) 정렬 삽입: 인덱스 0이 가장 내구도가 낮은 엔트리가 됨
	const float NewDurability = InEntry->GetCurrentDurability();
	int32 InsertIndex = 0;
	while (InsertIndex < Entries.Num() && Entries[InsertIndex] != nullptr && Entries[InsertIndex]->GetCurrentDurability() <= NewDurability)
	{
		++InsertIndex;
	}

	Entries.Insert(InEntry, InsertIndex);

	// 첫 번째 아이템 삽입 시 정적 정의의 초기 회전값 적용
	if (bWasEmpty == true && InEntry->GetItemDefinition() != nullptr)
	{
		Rotation = InEntry->GetItemDefinition()->InitialRotation;
	}

	return true;
}

UDISItemEntry* UDISItemStack::PopEntry()
{
	if (Entries.IsEmpty() == true)
	{
		return nullptr;
	}

	UDISItemEntry* TopEntry = Entries[0].Get();
	Entries.RemoveAt(0);
	return TopEntry;
}

TArray<UDISItemEntry*> UDISItemStack::Clear()
{
	TArray<UDISItemEntry*> RemovedEntries;
	RemovedEntries.Reserve(Entries.Num());

	for (const TObjectPtr<UDISItemEntry>& Entry : Entries)
	{
		if (Entry != nullptr)
		{
			RemovedEntries.Add(Entry.Get());
		}
	}

	Entries.Empty();
	return RemovedEntries;
}

EDISItemRotation UDISItemStack::GetRotation() const
{
	return Rotation;
}

void UDISItemStack::SetRotation(EDISItemRotation NewRotation)
{
	if (CanRotate() == true || NewRotation == EDISItemRotation::Rotate0)
	{
		Rotation = NewRotation;
	}
}

bool UDISItemStack::CanRotate() const
{
	const UDISItemEntry* Top = GetTopEntry();
	if (Top == nullptr || Top->GetItemDefinition() == nullptr)
	{
		return false;
	}

	return Top->GetItemDefinition()->bCanRotate;
}

bool UDISItemStack::RotateClockwise()
{
	if (CanRotate() == false)
	{
		return false;
	}

	Rotation = DISItemRotation::GetNextClockwise(Rotation);
	return true;
}

bool UDISItemStack::RotateCounterClockwise()
{
	if (CanRotate() == false)
	{
		return false;
	}

	Rotation = DISItemRotation::GetNextCounterClockwise(Rotation);
	return true;
}

TArray<FIntPoint> UDISItemStack::GetOccupiedPoints() const
{
	const UDISItemEntry* Top = GetTopEntry();
	if (Top == nullptr || Top->GetItemDefinition() == nullptr)
	{
		return {};
	}

	return Top->GetItemDefinition()->Shape.GetRotatedPoints(Rotation);
}

UDISInventory* UDISItemStack::GetOwningInventory() const
{
	return OwningInventory.Get();
}

void UDISItemStack::SetOwningInventory(UDISInventory* InInventory)
{
	if (OwningInventory.Get() != InInventory)
	{
		OwningInventory = InInventory;
		OnOwningInventoryChanged.Broadcast(this, InInventory);
	}
}

