#include "Global/BBMGlobalLogic.h"
#include "Global/BBMGlobalDefinition.h"

UBBMGlobalLogic::UBBMGlobalLogic()
{
}

void UBBMGlobalLogic::Deinitialize()
{
	RegisteredItemGroupProfiles.Empty();

	Super::Deinitialize();
}

EBBMItemUsageRule UBBMGlobalLogic::GetItemUsageRule(const FGameplayTag& ItemTypeTag) const
{
	const FDISItemTypeProfile& Profile = GetItemTypeProfile(ItemTypeTag);
	const FBBMItemTypeCustomData* CustomDataPtr = Profile.CustomData.GetPtr<FBBMItemTypeCustomData>();
	if (CustomDataPtr != nullptr)
	{
		return CustomDataPtr->UsageRule;
	}

	return EBBMItemUsageRule::Unlimited;
}

bool UBBMGlobalLogic::RegisterItemGroupProfile(const FGameplayTag& GroupTag, const FBBMItemGroupProfile& Profile)
{
	if (GroupTag.IsValid() == false)
	{
		return false;
	}

	// 이미 등록된 그룹 프로파일이 존재하는 경우 중복 등록 방지
	if (RegisteredItemGroupProfiles.Contains(GroupTag) == true)
	{
		return false;
	}

	RegisteredItemGroupProfiles.Add(GroupTag, Profile);
	return true;
}

bool UBBMGlobalLogic::UnregisterItemGroupProfile(const FGameplayTag& GroupTag)
{
	if (GroupTag.IsValid() == false)
	{
		return false;
	}

	const int32 RemovedCount = RegisteredItemGroupProfiles.Remove(GroupTag);
	return RemovedCount > 0;
}

const FBBMItemGroupProfile& UBBMGlobalLogic::GetItemGroupProfile(const FGameplayTag& GroupTag) const
{
	const FBBMItemGroupProfile* FoundProfile = RegisteredItemGroupProfiles.Find(GroupTag);
	if (FoundProfile != nullptr)
	{
		return *FoundProfile;
	}

	return FallbackGroupProfile;
}

bool UBBMGlobalLogic::HasItemGroupProfile(const FGameplayTag& GroupTag) const
{
	if (GroupTag.IsValid() == false)
	{
		return false;
	}

	return RegisteredItemGroupProfiles.Contains(GroupTag) == true;
}

bool UBBMGlobalLogic::RegisterBBMGlobalDefinition(const UBBMGlobalDefinition* BBMDefinition)
{
	if (BBMDefinition == nullptr)
	{
		return false;
	}

	for (const TPair<FGameplayTag, FBBMItemGroupProfile>& Pair : BBMDefinition->ItemGroupProfiles)
	{
		RegisterItemGroupProfile(Pair.Key, Pair.Value);
	}

	return true;
}

bool UBBMGlobalLogic::UnregisterBBMGlobalDefinition(const UBBMGlobalDefinition* BBMDefinition)
{
	if (BBMDefinition == nullptr)
	{
		return false;
	}

	for (const TPair<FGameplayTag, FBBMItemGroupProfile>& Pair : BBMDefinition->ItemGroupProfiles)
	{
		UnregisterItemGroupProfile(Pair.Key);
	}

	return true;
}
