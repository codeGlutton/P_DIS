#include "Global/DISGlobalSubsystem.h"
#include "Global/DISGlobalSettings.h"
#include "Global/DISGlobalDefinition.h"
#include "DISAssetBundles.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

#include "Enhancement/DISEnhancementContainer.h"

UDISGlobalSubsystem::UDISGlobalSubsystem() :
	NextEnhancementEffectHandleId(1)
{
}

void UDISGlobalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UDISGlobalSettings* Settings = GetDefault<UDISGlobalSettings>();
	if (Settings == nullptr)
	{
		return;
	}

	// 전역 환경설정에서 기본 폴백 프로파일 설정
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

void UDISGlobalSubsystem::Deinitialize()
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
	InvalidateProfileCache();
	RegisteredItemTypeProfiles.Empty();
	RegisteredRarityProfiles.Empty();
	ActiveEnhancementEffectHandles.Empty();

	Super::Deinitialize();
}

bool UDISGlobalSubsystem::RegisterGlobalDefinition(const UDISGlobalDefinition* GlobalDefinition)
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

bool UDISGlobalSubsystem::UnregisterGlobalDefinition(const UDISGlobalDefinition* GlobalDefinition)
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

void UDISGlobalSubsystem::InvalidateProfileCache()
{
	ResolvedProfileCache.Empty();
}

bool UDISGlobalSubsystem::RegisterItemTypeProfile(const FGameplayTag& ItemTypeTag, const FDISItemTypeProfile& Profile)
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
	InvalidateProfileCache();
	return true;
}

bool UDISGlobalSubsystem::UnregisterItemTypeProfile(const FGameplayTag& ItemTypeTag)
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

const FDISItemTypeProfile& UDISGlobalSubsystem::GetItemTypeProfile(const FGameplayTag& ItemTypeTag) const
{
	if (ItemTypeTag.IsValid() == false)
	{
		return FallbackItemProfile;
	}

	// 1. 이미 해석되어 캐시된 프로파일이 존재하는 경우 O(1) 즉각 반환
	if (const FDISItemTypeProfile* const* CachedProfilePtr = ResolvedProfileCache.Find(ItemTypeTag))
	{
		if (*CachedProfilePtr != nullptr)
		{
			return **CachedProfilePtr;
		}
	}

	// 2. 소분류부터 시작하여 부모 계층을 타고 올라가며 상속된 프로파일 탐색 (소분류 우선 순서 보장)
	const FDISItemTypeProfile* ResolvedProfile = &FallbackItemProfile;
	for (FGameplayTag CurrentTag = ItemTypeTag; CurrentTag.IsValid() == true; CurrentTag = CurrentTag.RequestDirectParent())
	{
		const FDISItemTypeProfile* FoundProfile = RegisteredItemTypeProfiles.Find(CurrentTag);
		if (FoundProfile != nullptr)
		{
			ResolvedProfile = FoundProfile;
			break;
		}
	}

	// 3. 탐색 결과를 캐시에 등록하여 다음 호출부터 O(1) 조회 보장
	ResolvedProfileCache.Add(ItemTypeTag, ResolvedProfile);
	return *ResolvedProfile;
}

TArray<FGameplayTag> UDISGlobalSubsystem::GetItemTypeTagHierarchy(const FGameplayTag& ItemTypeTag) const
{
	TArray<FGameplayTag> HierarchyTags;
	if (ItemTypeTag.IsValid() == false)
	{
		return HierarchyTags;
	}

	// 부모 태그를 포함한 모든 태그 컨테이너를 엔진 캐시 조회로 1회 획득
	ItemTypeTag.GetGameplayTagParents().GetGameplayTagArray(HierarchyTags);

	// Root -> Leaf 순서 보장 (부모 태그의 문자열 길이는 항상 자식 태그보다 짧음)
	HierarchyTags.Sort([](const FGameplayTag& TagA, const FGameplayTag& TagB)
	{
		return TagA.GetTagName().GetStringLength() < TagB.GetTagName().GetStringLength();
	});

	return HierarchyTags;
}

bool UDISGlobalSubsystem::MatchesItemCategory(const FGameplayTag& ItemTypeTag, const FGameplayTag& CategoryTag)
{
	return ItemTypeTag.MatchesTag(CategoryTag);
}

bool UDISGlobalSubsystem::MatchesAnyItemCategory(const FGameplayTag& ItemTypeTag, const FGameplayTagContainer& CategoryContainer)
{
	return ItemTypeTag.MatchesAny(CategoryContainer);
}

bool UDISGlobalSubsystem::RegisterRarityProfile(const FGameplayTag& RarityTag, const FDISRarityProfile& Profile)
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

bool UDISGlobalSubsystem::UnregisterRarityProfile(const FGameplayTag& RarityTag)
{
	if (RarityTag.IsValid() == false)
	{
		return false;
	}

	// 등록된 프로파일이 없거나 제거 실패 시 false 반환
	const int32 RemovedCount = RegisteredRarityProfiles.Remove(RarityTag);
	return RemovedCount > 0;
}

const FDISRarityProfile& UDISGlobalSubsystem::GetRarityProfile(const FGameplayTag& RarityTag) const
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

int32 UDISGlobalSubsystem::GetRarityRank(const FGameplayTag& RarityTag) const
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

int32 UDISGlobalSubsystem::CompareRarity(const FGameplayTag& TagA, const FGameplayTag& TagB) const
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

FDISActiveEnhancementEffectHandle UDISGlobalSubsystem::AllocateEnhancementEffectHandle(UDISEnhancementContainer* InOwnerContainer)
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

bool UDISGlobalSubsystem::ReleaseEnhancementEffectHandle(const FDISActiveEnhancementEffectHandle& Handle)
{
	if (Handle.IsValid() == false)
	{
		return false;
	}

	const int32 RemovedCount = ActiveEnhancementEffectHandles.Remove(Handle);
	return RemovedCount > 0;
}

bool UDISGlobalSubsystem::IsEnhancementEffectHandleActive(const FDISActiveEnhancementEffectHandle& Handle) const
{
	if (Handle.IsValid() == false)
	{
		return false;
	}

	return ActiveEnhancementEffectHandles.Contains(Handle) == true;
}

UDISEnhancementContainer* UDISGlobalSubsystem::GetEnhancementContainerByEffectHandle(const FDISActiveEnhancementEffectHandle& Handle) const
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
