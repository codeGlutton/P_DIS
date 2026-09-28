#include "Global/DISGlobalSubsystem.h"
#include "Global/DISGlobalSettings.h"
#include "Global/DISGlobalDefinition.h"
#include "DISAssetBundles.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

UDISGlobalSubsystem::UDISGlobalSubsystem()
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

	// 런타임 등록 데이터 정리
	RegisteredItemTypeProfiles.Empty();

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

	return true;
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
	return RemovedCount > 0;
}

const FDISItemTypeProfile& UDISGlobalSubsystem::GetItemTypeProfile(const FGameplayTag& ItemTypeTag) const
{
	// 런타임에 등록된 프로파일 맵에서 우선 검색
	const FDISItemTypeProfile* FoundProfile = RegisteredItemTypeProfiles.Find(ItemTypeTag);
	if (FoundProfile != nullptr)
	{
		return *FoundProfile;
	}

	// 매핑된 프로파일이 없는 경우 기본 폴백 프로파일 반환
	return FallbackItemProfile;
}
