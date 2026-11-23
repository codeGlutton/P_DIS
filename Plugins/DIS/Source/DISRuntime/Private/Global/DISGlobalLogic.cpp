/*****************************************************************//**
 * @file   DISGlobalLogic.cpp
 * @brief  DIS 플러그인의 전역 비즈니스 로직 및 프로파일/상태 관리 서브오브젝트 구현부
 * @date   2026-10-28
 *********************************************************************/

#include "Global/DISGlobalLogic.h"
#include "Global/DISGlobalSubsystem.h"
#include "Global/DISGlobalSettings.h"
#include "Global/DISGlobalDefinition.h"
#include "DISAssetBundles.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Enhancement/DISEnhancementContainer.h"

UDISGlobalLogic::UDISGlobalLogic() :
	NextEnhancementEffectHandleId(1),
	OwningSubsystem(nullptr),
	LoadedCoreGlobalDefinition(nullptr),
	CoreGlobalDefinitionHandle(nullptr)
{
}

void UDISGlobalLogic::Initialize(UDISGlobalSubsystem* InSubsystem)
{
	OwningSubsystem = InSubsystem;

	const UDISGlobalSettings* Settings = GetDefault<UDISGlobalSettings>();
	if (Settings == nullptr)
	{
		return;
	}

	// 전역 환경설정에서 기본 폴백 프로파일 초기화
	FallbackItemProfile = Settings->DefaultItemProfile;

	// 전역 런타임 설정 에셋(UDISGlobalDefinition) 번들 사전 로드 및 초기 프로파일 등록
	if (Settings->GlobalDefinitionAssetId.IsValid() == true)
	{
		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (AssetManager != nullptr)
		{
			// Data 번들을 포함하여 코어 에셋 사전 로드 및 상시 유지를 위한 스트리밍 핸들 보관
			const TArray<FName> BundlesToLoad = { DISAssetBundles::UI, DISAssetBundles::World, DISAssetBundles::Data };
			CoreGlobalDefinitionHandle = AssetManager->LoadPrimaryAsset(
				Settings->GlobalDefinitionAssetId,
				BundlesToLoad
			);

			if (CoreGlobalDefinitionHandle.IsValid() == true)
			{
				// 게임 인스턴스 초기화 단계에서 동기 완료 보장
				CoreGlobalDefinitionHandle->WaitUntilComplete();
			}

			LoadedCoreGlobalDefinition = Cast<UDISGlobalDefinition>(AssetManager->GetPrimaryAssetObject(Settings->GlobalDefinitionAssetId));
			if (LoadedCoreGlobalDefinition != nullptr)
			{
				// 로드된 코어 전역 정의 에셋의 구성 멤버 등록
				RegisterGlobalDefinition(LoadedCoreGlobalDefinition);
			}
		}
	}
}

void UDISGlobalLogic::Deinitialize()
{
	// 기본 로드된 코어 전역 정의 에셋 등록 해제
	if (LoadedCoreGlobalDefinition != nullptr)
	{
		UnregisterGlobalDefinition(LoadedCoreGlobalDefinition);
		LoadedCoreGlobalDefinition = nullptr;
	}

	// 코어 전역 에셋 스트리밍 핸들 해제 및 상주 포인터 정리
	if (CoreGlobalDefinitionHandle.IsValid() == true)
	{
		CoreGlobalDefinitionHandle->CancelHandle();
		CoreGlobalDefinitionHandle.Reset();
	}

	// 런타임 등록 데이터 및 캐시 정리
	RegisteredItemTypeProfiles.Empty();
	ResolvedProfileCache.Empty();
	RegisteredRarityProfiles.Empty();
	ActiveEnhancementEffectHandles.Empty();
	OwningSubsystem.Reset();
}

bool UDISGlobalLogic::RegisterGlobalDefinition(const UDISGlobalDefinition* GlobalDefinition)
{
	if (GlobalDefinition == nullptr)
	{
		return false;
	}

	// 전역 정의 에셋 내 아이템 타입 프로파일 등록
	for (const TPair<FGameplayTag, FDISItemTypeProfile>& Pair : GlobalDefinition->ItemTypeProfiles)
	{
		RegisterItemTypeProfile(Pair.Key, Pair.Value);
	}

	// 전역 정의 에셋 내 희귀도 프로파일 등록
	for (const TPair<FGameplayTag, FDISRarityProfile>& Pair : GlobalDefinition->RarityProfiles)
	{
		RegisterRarityProfile(Pair.Key, Pair.Value);
	}

	return true;
}

bool UDISGlobalLogic::UnregisterGlobalDefinition(const UDISGlobalDefinition* GlobalDefinition)
{
	if (GlobalDefinition == nullptr)
	{
		return false;
	}

	// 전역 정의 에셋 내 아이템 타입 프로파일 해제
	for (const TPair<FGameplayTag, FDISItemTypeProfile>& Pair : GlobalDefinition->ItemTypeProfiles)
	{
		UnregisterItemTypeProfile(Pair.Key);
	}

	// 전역 정의 에셋 내 희귀도 프로파일 해제
	for (const TPair<FGameplayTag, FDISRarityProfile>& Pair : GlobalDefinition->RarityProfiles)
	{
		UnregisterRarityProfile(Pair.Key);
	}

	return true;
}

bool UDISGlobalLogic::RegisterItemTypeProfile(const FGameplayTag& ItemTypeTag, const FDISItemTypeProfile& Profile)
{
	if (ItemTypeTag.IsValid() == false)
	{
		return false;
	}

	// 이미 등록된 프로파일이 존재하는 경우 중복 등록 방지
	if (RegisteredItemTypeProfiles.Contains(ItemTypeTag) == true)
	{
		return false;
	}

	RegisteredItemTypeProfiles.Add(ItemTypeTag, Profile);

	// 프로파일 등록 변경으로 인한 상속 해석 캐시 무효화
	InvalidateProfileCache();
	return true;
}

bool UDISGlobalLogic::UnregisterItemTypeProfile(const FGameplayTag& ItemTypeTag)
{
	if (ItemTypeTag.IsValid() == false)
	{
		return false;
	}

	// 등록된 프로파일이 없거나 제거 실패 시 false 반환
	const int32 RemovedCount = RegisteredItemTypeProfiles.Remove(ItemTypeTag);
	if (RemovedCount > 0)
	{
		InvalidateProfileCache();
		return true;
	}

	return false;
}

const FDISItemTypeProfile& UDISGlobalLogic::GetItemTypeProfile(const FGameplayTag& ItemTypeTag) const
{
	if (ItemTypeTag.IsValid() == false)
	{
		return FallbackItemProfile;
	}

	// 1. 이미 상속 병합이 완료되어 캐싱된 프로파일이 존재하는지 확인
	const FDISItemTypeProfile* CachedProfile = ResolvedProfileCache.Find(ItemTypeTag);
	if (CachedProfile != nullptr)
	{
		return *CachedProfile;
	}

	// 2. 루트부터 요청 태그까지의 계층 순서 태그 배열 획득
	TArray<FGameplayTag> TagHierarchy;
	GetItemTypeTagHierarchy(ItemTypeTag, TagHierarchy);

	// 3. Fallback 기본값에서 시작하여 계층 순서대로 상속 병합 수행
	FDISItemTypeProfile MergedProfile = FallbackItemProfile;

	for (const FGameplayTag& CurrentTag : TagHierarchy)
	{
		const FDISItemTypeProfile* FoundProfile = RegisteredItemTypeProfiles.Find(CurrentTag);
		if (FoundProfile != nullptr)
		{
			if (FoundProfile->DisplayName.IsEmpty() == false)
			{
				MergedProfile.DisplayName = FoundProfile->DisplayName;
			}
			if (FoundProfile->Icon.IsNull() == false)
			{
				MergedProfile.Icon = FoundProfile->Icon;
			}

			// CustomData 상속 제어: Override가 켜진 경우에만 부모를 상속받지 않고 자신의 CustomData 적용
			if (FoundProfile->bOverrideCustomData == true)
			{
				MergedProfile.bOverrideCustomData = true;
				MergedProfile.CustomData = FoundProfile->CustomData;
			}
		}
	}

	// 4. 캐싱 및 반환
	ResolvedProfileCache.Add(ItemTypeTag, MergedProfile);
	return ResolvedProfileCache.FindChecked(ItemTypeTag);
}

void UDISGlobalLogic::InvalidateProfileCache()
{
	ResolvedProfileCache.Empty();
}

void UDISGlobalLogic::GetItemTypeTagHierarchy(const FGameplayTag& InItemTypeTag, TArray<FGameplayTag>& OutHierarchyTags) const
{
	OutHierarchyTags.Reset();

	if (InItemTypeTag.IsValid() == false)
	{
		return;
	}

	// 부모 태그들을 루트부터 순서대로 수집한 뒤 마지막에 본인 태그 추가
	InItemTypeTag.ParseParentTags(OutHierarchyTags);
	OutHierarchyTags.Add(InItemTypeTag);
}

bool UDISGlobalLogic::MatchesItemCategory(const FGameplayTag& InItemTypeTag, const FGameplayTag& InCategoryTag) const
{
	if (InItemTypeTag.IsValid() == false || InCategoryTag.IsValid() == false)
	{
		return false;
	}

	return InItemTypeTag.MatchesTag(InCategoryTag);
}

bool UDISGlobalLogic::RegisterRarityProfile(const FGameplayTag& RarityTag, const FDISRarityProfile& Profile)
{
	if (RarityTag.IsValid() == false)
	{
		return false;
	}

	// 이미 등록된 프로파일이 존재하는 경우 중복 등록 방지
	if (RegisteredRarityProfiles.Contains(RarityTag) == true)
	{
		return false;
	}

	RegisteredRarityProfiles.Add(RarityTag, Profile);
	return true;
}

bool UDISGlobalLogic::UnregisterRarityProfile(const FGameplayTag& RarityTag)
{
	if (RarityTag.IsValid() == false)
	{
		return false;
	}

	// 등록된 프로파일이 없거나 제거 실패 시 false 반환
	const int32 RemovedCount = RegisteredRarityProfiles.Remove(RarityTag);
	return RemovedCount > 0;
}

const FDISRarityProfile& UDISGlobalLogic::GetRarityProfile(const FGameplayTag& RarityTag) const
{
	// 런타임에 등록된 프로파일 맵에서 우선 검색
	const FDISRarityProfile* FoundProfile = RegisteredRarityProfiles.Find(RarityTag);
	if (FoundProfile != nullptr)
	{
		return *FoundProfile;
	}

	// 매핑된 프로파일이 없는 경우 기본 폴백 프로파일 반환
	return FallbackRarityProfile;
}

int32 UDISGlobalLogic::GetRarityRank(const FGameplayTag& RarityTag) const
{
	if (RarityTag.IsValid() == false)
	{
		return 0;
	}

	const FDISRarityProfile* FoundProfile = RegisteredRarityProfiles.Find(RarityTag);
	if (FoundProfile != nullptr)
	{
		return FoundProfile->Rank;
	}

	return FallbackRarityProfile.Rank;
}

int32 UDISGlobalLogic::CompareRarity(const FGameplayTag& TagA, const FGameplayTag& TagB) const
{
	const int32 RankA = GetRarityRank(TagA);
	const int32 RankB = GetRarityRank(TagB);

	if (RankA > RankB)
	{
		return 1;
	}
	else if (RankA < RankB)
	{
		return -1;
	}

	return 0;
}

FDISActiveEnhancementEffectHandle UDISGlobalLogic::AllocateEnhancementEffectHandle(UDISEnhancementContainer* InOwnerContainer)
{
	// 전역에서 유일하고 중복되지 않는 양수 정수 핸들 번호 생성
	int32 CandidateId = NextEnhancementEffectHandleId++;
	if (CandidateId <= 0)
	{
		CandidateId = 1;
		NextEnhancementEffectHandleId = 2;
	}

	FDISActiveEnhancementEffectHandle NewHandle(CandidateId);

	// 만약 TMap에 이미 존재할 경우 미사용 번호가 나올 때까지 반복 탐색
	while (ActiveEnhancementEffectHandles.Contains(NewHandle) == true)
	{
		CandidateId = NextEnhancementEffectHandleId++;
		if (CandidateId <= 0)
		{
			CandidateId = 1;
			NextEnhancementEffectHandleId = 2;
		}
		NewHandle = FDISActiveEnhancementEffectHandle(CandidateId);
	}

	// TMap에 소유 컨테이너와 함께 등록
	ActiveEnhancementEffectHandles.Add(NewHandle, InOwnerContainer);
	return NewHandle;
}

bool UDISGlobalLogic::ReleaseEnhancementEffectHandle(const FDISActiveEnhancementEffectHandle& Handle)
{
	if (Handle.IsValid() == false)
	{
		return false;
	}

	const int32 RemovedCount = ActiveEnhancementEffectHandles.Remove(Handle);
	return RemovedCount > 0;
}

bool UDISGlobalLogic::IsEnhancementEffectHandleActive(const FDISActiveEnhancementEffectHandle& Handle) const
{
	if (Handle.IsValid() == false)
	{
		return false;
	}

	return ActiveEnhancementEffectHandles.Contains(Handle) == true;
}

UDISEnhancementContainer* UDISGlobalLogic::GetEnhancementContainerByEffectHandle(const FDISActiveEnhancementEffectHandle& Handle) const
{
	if (Handle.IsValid() == false)
	{
		return nullptr;
	}

	const TWeakObjectPtr<UDISEnhancementContainer>* FoundContainer = ActiveEnhancementEffectHandles.Find(Handle);
	if (FoundContainer != nullptr)
	{
		return FoundContainer->Get();
	}

	return nullptr;
}

UDISGlobalSubsystem* UDISGlobalLogic::GetOwningSubsystem() const
{
	return OwningSubsystem.Get();
}
