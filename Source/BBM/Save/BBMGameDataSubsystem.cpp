/*****************************************************************//**
 * @file   BBMGameDataSubsystem.cpp
 * @brief  BBM(비트 배낭 상인) 영속 게임 데이터(PersistData) 및 비동기 파일 저장 총괄 서브시스템 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Save/BBMGameDataSubsystem.h"
#include "Save/BBMUserSaveGame.h"
#include "Save/BBMRunSaveGame.h"
#include "Combat/Pawn/BBMPlayerMerchantBase.h"
#include "Combat/Pawn/BBMCharacterDefinition.h"
#include "Combat/Pawn/BBMPlayerCharacterDefinition.h"
#include "Stage/BBMRegionDefinition.h"
#include "Combat/GAS/BBMAttributeSet.h"
#include "Inventory/BBMInventoryComponent.h"
#include "Inventory/BBMInventory.h"
#include "Inventory/BBMInventorySlot.h"
#include "Item/DISItemStack.h"
#include "Item/DISItemEntry.h"
#include "Item/DISItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Async/Async.h"

UBBMGameDataSubsystem::UBBMGameDataSubsystem() :
	UserSaveData(nullptr),
	CurrentRunData(nullptr),
	bIsAsyncOperationInProgress(false)
{
}

void UBBMGameDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 1. 기본 인메모리 세이브 객체 생성
	UserSaveData = NewObject<UBBMUserSaveGame>(this, UBBMUserSaveGame::StaticClass());
	CurrentRunData = NewObject<UBBMRunSaveGame>(this, UBBMRunSaveGame::StaticClass());

	// 2. 필요 시 백업(.bak) 파일로부터 복구 시도 후 디스크 유저 세팅 로드
	RestoreFromBackupIfNeeded(UBBMUserSaveGame::SlotName);
	if (UGameplayStatics::DoesSaveGameExist(UBBMUserSaveGame::SlotName, UBBMUserSaveGame::UserIndex) == true)
	{
		USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(UBBMUserSaveGame::SlotName, UBBMUserSaveGame::UserIndex);
		if (UBBMUserSaveGame* CastedUser = Cast<UBBMUserSaveGame>(Loaded))
		{
			UserSaveData = CastedUser;
		}
	}

	// 3. 필요 시 백업(.bak) 파일로부터 복구 시도 후 디스크 런 세이브 로드
	RestoreFromBackupIfNeeded(UBBMRunSaveGame::SlotName);
	if (UGameplayStatics::DoesSaveGameExist(UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex) == true)
	{
		USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex);
		if (UBBMRunSaveGame* CastedRun = Cast<UBBMRunSaveGame>(Loaded))
		{
			CurrentRunData = CastedRun;
		}
	}
}

void UBBMGameDataSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

bool UBBMGameDataSubsystem::HasActiveRun() const
{
	return CurrentRunData != nullptr && CurrentRunData->bHasActiveRun == true;
}

UBBMUserSaveGame* UBBMGameDataSubsystem::GetUserSaveData() const
{
	return UserSaveData;
}

UBBMRunSaveGame* UBBMGameDataSubsystem::GetCurrentRunData() const
{
	return CurrentRunData;
}

bool UBBMGameDataSubsystem::EnsureSaveDataLoaded()
{
	bool bLoadedOrCreated = true;

	if (UserSaveData == nullptr)
	{
		UserSaveData = NewObject<UBBMUserSaveGame>(this, UBBMUserSaveGame::StaticClass());
		RestoreFromBackupIfNeeded(UBBMUserSaveGame::SlotName);
		if (UGameplayStatics::DoesSaveGameExist(UBBMUserSaveGame::SlotName, UBBMUserSaveGame::UserIndex) == true)
		{
			if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(UBBMUserSaveGame::SlotName, UBBMUserSaveGame::UserIndex))
			{
				if (UBBMUserSaveGame* CastedUser = Cast<UBBMUserSaveGame>(Loaded))
				{
					UserSaveData = CastedUser;
				}
			}
		}
	}

	if (CurrentRunData == nullptr)
	{
		CurrentRunData = NewObject<UBBMRunSaveGame>(this, UBBMRunSaveGame::StaticClass());
		RestoreFromBackupIfNeeded(UBBMRunSaveGame::SlotName);
		if (UGameplayStatics::DoesSaveGameExist(UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex) == true)
		{
			if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex))
			{
				if (UBBMRunSaveGame* CastedRun = Cast<UBBMRunSaveGame>(Loaded))
				{
					CurrentRunData = CastedRun;
				}
			}
		}
	}

	return bLoadedOrCreated;
}

void UBBMGameDataSubsystem::ResetAndInitializeNewRun(const UBBMPlayerCharacterDefinition* PlayerDef)
{
	EnsureSaveDataLoaded();

	if (CurrentRunData != nullptr)
	{
		CurrentRunData->ResetRunData();
		CurrentRunData->bHasActiveRun = true;
		CurrentRunData->CurrentGold = 0;
		CurrentRunData->CurrentEncounterIndex = 1;

		if (PlayerDef != nullptr)
		{
			CurrentRunData->PlayerCharacterId = PlayerDef->GetPrimaryAssetId();
			if (const UBBMRegionDefinition* StartRegion = PlayerDef->GetStartRegionDefinition())
			{
				CurrentRunData->CurrentRegionId = StartRegion->GetPrimaryAssetId();
			}
		}
	}
}

bool UBBMGameDataSubsystem::ApplyRunDataToMerchant(ABBMPlayerMerchantBase* TargetMerchant)
{
	if (TargetMerchant == nullptr || CurrentRunData == nullptr)
	{
		return false;
	}

	return TargetMerchant->ApplyRunSaveData(CurrentRunData);
}

void UBBMGameDataSubsystem::ExtractLevelData(
	const ABBMPlayerMerchantBase* InMerchant,
	const FPrimaryAssetId& InCurrentRegionId,
	int32 InNextEncounterIndex,
	const TArray<FPrimaryAssetId>& InPlannedEncounters,
	int32 InRngSeed)
{
	static const TArray<TInstancedStruct<FBBMRuntimeEncounterData>> EmptyRuntimeEncounters;
	ExtractLevelData(InMerchant, InCurrentRegionId, InNextEncounterIndex, InPlannedEncounters, InRngSeed, EmptyRuntimeEncounters);
}

void UBBMGameDataSubsystem::ExtractLevelData(
	const ABBMPlayerMerchantBase* InMerchant,
	const FPrimaryAssetId& InCurrentRegionId,
	int32 InNextEncounterIndex,
	const TArray<FPrimaryAssetId>& InPlannedEncounters,
	int32 InRngSeed,
	const TArray<TInstancedStruct<FBBMRuntimeEncounterData>>& InRuntimeEncounters)
{
	if (CurrentRunData == nullptr)
	{
		CurrentRunData = NewObject<UBBMRunSaveGame>(this, UBBMRunSaveGame::StaticClass());
	}

	CurrentRunData->bHasActiveRun = true;
	CurrentRunData->CurrentRegionId = InCurrentRegionId;
	CurrentRunData->CurrentEncounterIndex = InNextEncounterIndex;
	CurrentRunData->PlannedEncounterIds = InPlannedEncounters;
	CurrentRunData->SavedPlannedEncounters = InRuntimeEncounters;
	CurrentRunData->RngSeed = InRngSeed;

	if (InMerchant == nullptr)
	{
		return;
	}

	// 플레이어 캐릭터 정의 식별자 추출
	if (const UBBMCharacterDefinition* CharDef = InMerchant->GetCharacterDefinition())
	{
		CurrentRunData->PlayerCharacterId = CharDef->GetPrimaryAssetId();
	}

	// GAS 어트리뷰트 스탯 추출
	if (const UBBMAttributeSet* AttrSet = InMerchant->GetAttributeSet())
	{
		CurrentRunData->CurrentHealth = AttrSet->GetHealth();
		CurrentRunData->MaxHealth = AttrSet->GetMaxHealth();
	}

	// 플레이어 골드(재화) 추출
	CurrentRunData->CurrentGold = InMerchant->GetGold();

	// 배낭 인벤토리 슬롯 상태 추출
	CurrentRunData->SavedInventorySlots.Empty();
	CurrentRunData->ArchivedEquipments.Empty();

	const UBBMInventoryComponent* InvenComp = InMerchant->GetInventoryComponent();
	if (InvenComp != nullptr)
	{
		const UBBMInventory* Inventory = InvenComp->GetInventory();
		if (Inventory != nullptr)
		{
			const int32 TotalSlots = Inventory->GetSlotCount();
			for (int32 SlotIdx = 0; SlotIdx < TotalSlots; ++SlotIdx)
			{
				const UDISItemStack* Stack = Inventory->GetSlotItemStack(SlotIdx);
				if (Stack != nullptr && Stack->IsEmpty() == false)
				{
					// 중복 저장을 방지하기 위해 해당 슬롯이 스택의 기준 좌상단 루트 슬롯인 경우에만 레코드 생성
					const int32 BaseSlot = Inventory->GetItemStackRootSlot(Stack);
					if (BaseSlot == SlotIdx)
					{
						FBBMInventorySlotSaveRecord Record;
						Record.SlotIndex = SlotIdx;
						Record.StackCount = Stack->GetStackCount();
						Record.Rotation = Stack->GetRotation();

						if (Stack->GetTopEntry() != nullptr && Stack->GetTopEntry()->GetItemDefinition() != nullptr)
						{
							Record.ItemDefinitionPath = FSoftObjectPath(Stack->GetTopEntry()->GetItemDefinition());
						}

						CurrentRunData->SavedInventorySlots.Add(Record);
					}
				}
			}
		}

		// 분리 보관소(UsedEquipmentStorage) 장비 목록 추출
		const TArray<UDISItemStack*> Archived = InvenComp->GetArchivedEquipmentStacks();
		for (const UDISItemStack* EquipStack : Archived)
		{
			if (EquipStack != nullptr && EquipStack->IsEmpty() == false)
			{
				FBBMEquipmentSaveRecord EquipRecord;
				EquipRecord.StackCount = EquipStack->GetStackCount();
				EquipRecord.Rotation = EquipStack->GetRotation();
				if (EquipStack->GetTopEntry() != nullptr && EquipStack->GetTopEntry()->GetItemDefinition() != nullptr)
				{
					EquipRecord.ItemDefinitionPath = FSoftObjectPath(EquipStack->GetTopEntry()->GetItemDefinition());
				}
				CurrentRunData->ArchivedEquipments.Add(EquipRecord);
			}
		}
	}
}

void UBBMGameDataSubsystem::SaveCurrentRunAsync(FSimpleDelegate OnComplete)
{
	SaveGameAsyncInternal(CurrentRunData, UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex, [OnComplete](bool /* bSuccess */)
	{
		OnComplete.ExecuteIfBound();
	});
}

void UBBMGameDataSubsystem::K2_SaveCurrentRunAsync(FBBMAsyncSaveCompleteDynamicDelegate OnComplete)
{
	SaveGameAsyncInternal(CurrentRunData, UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex, [OnComplete](bool bSuccess)
	{
		OnComplete.ExecuteIfBound(bSuccess);
	});
}

void UBBMGameDataSubsystem::LoadCurrentRunAsync(FSimpleDelegate OnComplete)
{
	LoadGameAsyncInternal(UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex, [this, OnComplete](USaveGame* LoadedGame, bool /* bSuccess */)
	{
		if (UBBMRunSaveGame* Casted = Cast<UBBMRunSaveGame>(LoadedGame))
		{
			CurrentRunData = Casted;
		}
		OnComplete.ExecuteIfBound();
	});
}

void UBBMGameDataSubsystem::K2_LoadCurrentRunAsync(FBBMAsyncSaveCompleteDynamicDelegate OnComplete)
{
	LoadGameAsyncInternal(UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex, [this, OnComplete](USaveGame* LoadedGame, bool bSuccess)
	{
		if (UBBMRunSaveGame* Casted = Cast<UBBMRunSaveGame>(LoadedGame))
		{
			CurrentRunData = Casted;
		}
		OnComplete.ExecuteIfBound(bSuccess);
	});
}

void UBBMGameDataSubsystem::SaveUserSettingAsync(FSimpleDelegate OnComplete)
{
	SaveGameAsyncInternal(UserSaveData, UBBMUserSaveGame::SlotName, UBBMUserSaveGame::UserIndex, [OnComplete](bool /* bSuccess */)
	{
		OnComplete.ExecuteIfBound();
	});
}

void UBBMGameDataSubsystem::LoadUserSettingAsync(FSimpleDelegate OnComplete)
{
	LoadGameAsyncInternal(UBBMUserSaveGame::SlotName, UBBMUserSaveGame::UserIndex, [this, OnComplete](USaveGame* LoadedGame, bool /* bSuccess */)
	{
		if (UBBMUserSaveGame* Casted = Cast<UBBMUserSaveGame>(LoadedGame))
		{
			UserSaveData = Casted;
		}
		OnComplete.ExecuteIfBound();
	});
}

void UBBMGameDataSubsystem::SaveGameAsyncInternal(
	USaveGame* InSaveGame,
	const FString& InSlotName,
	int32 InUserIndex,
	TFunction<void(bool)> OnComplete)
{
	if (InSaveGame == nullptr || InSlotName.IsEmpty())
	{
		if (OnComplete)
		{
			OnComplete(false);
		}
		return;
	}

	// 1. [메인 스레드] USaveGame 객체를 메모리 바이트 배열로 직렬화 (UObject 스레드 안전성 보장)
	TSharedRef<TArray<uint8>> SaveDataRef = MakeShared<TArray<uint8>>();
	const bool bSerialized = UGameplayStatics::SaveGameToMemory(InSaveGame, *SaveDataRef);
	if (bSerialized == false || SaveDataRef->Num() == 0)
	{
		if (OnComplete)
		{
			OnComplete(false);
		}
		return;
	}

	bIsAsyncOperationInProgress = true;
	const TWeakObjectPtr<UBBMGameDataSubsystem> WeakThis(this);

	// 2. [백그라운드 스레드] 파일 시스템 I/O 순차 실행: (sav 확인 -> bak 복제) -> (새 바이트 데이터를 sav로 파일 쓰기)
	Async(EAsyncExecution::ThreadPool, [SaveDataRef, InSlotName, OnComplete, WeakThis]()
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		const FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
		if (PlatformFile.DirectoryExists(*SaveDir) == false)
		{
			PlatformFile.CreateDirectoryTree(*SaveDir);
		}

		const FString SavPath = GetSaveFilePath(InSlotName);
		const FString BakPath = GetBackupFilePath(InSlotName);

		// 저장 중간 데이터 유실 방지: 기존 파일이 있다면 .bak 파일로 복제
		if (PlatformFile.FileExists(*SavPath) == true)
		{
			if (PlatformFile.FileExists(*BakPath) == true)
			{
				PlatformFile.DeleteFile(*BakPath);
			}
			PlatformFile.CopyFile(*BakPath, *SavPath);
		}

		// 새로운 직렬화 바이트 데이터를 .sav 파일로 안전하게 저장
		const bool bSaved = FFileHelper::SaveArrayToFile(*SaveDataRef, *SavPath);

		// 3. [메인 스레드] 완료 통보 및 상태 플래그 해제
		Async(EAsyncExecution::TaskGraphMainThread, [WeakThis, OnComplete, bSaved]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->bIsAsyncOperationInProgress = false;
			}
			if (OnComplete)
			{
				OnComplete(bSaved);
			}
		});
	});
}

void UBBMGameDataSubsystem::LoadGameAsyncInternal(
	const FString& InSlotName,
	int32 InUserIndex,
	TFunction<void(USaveGame*, bool)> OnComplete)
{
	if (InSlotName.IsEmpty())
	{
		if (OnComplete)
		{
			OnComplete(nullptr, false);
		}
		return;
	}

	bIsAsyncOperationInProgress = true;
	const TWeakObjectPtr<UBBMGameDataSubsystem> WeakThis(this);

	// 1. [백그라운드 스레드] 파일 시스템 I/O 실행: (sav 확인 -> 없을 시 bak을 sav로 복제 및 전환 -> sav 파일 읽기)
	Async(EAsyncExecution::ThreadPool, [InSlotName, OnComplete, WeakThis]()
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		const FString SavPath = GetSaveFilePath(InSlotName);
		const FString BakPath = GetBackupFilePath(InSlotName);

		// 로드 시 기존 .sav 파일이 없다면 .bak 파일을 복제하여 .sav로 복구 전환
		if (PlatformFile.FileExists(*SavPath) == false && PlatformFile.FileExists(*BakPath) == true)
		{
			PlatformFile.CopyFile(*SavPath, *BakPath);
		}

		TSharedRef<TArray<uint8>> LoadedBytesRef = MakeShared<TArray<uint8>>();
		bool bFileRead = false;

		// .sav 파일이 존재하면 메모리로 바이트 로드
		if (PlatformFile.FileExists(*SavPath) == true)
		{
			bFileRead = FFileHelper::LoadFileToArray(*LoadedBytesRef, *SavPath);
		}

		// 2. [메인 스레드] 로드된 바이트를 USaveGame 객체로 역직렬화 (UObject 생성은 메인 스레드 전담)
		Async(EAsyncExecution::TaskGraphMainThread, [WeakThis, OnComplete, LoadedBytesRef, bFileRead]()
		{
			USaveGame* LoadedObject = nullptr;
			if (bFileRead == true && LoadedBytesRef->Num() > 0)
			{
				LoadedObject = UGameplayStatics::LoadGameFromMemory(*LoadedBytesRef);
			}

			if (WeakThis.IsValid())
			{
				WeakThis->bIsAsyncOperationInProgress = false;
			}

			const bool bSuccess = (LoadedObject != nullptr);
			if (OnComplete)
			{
				OnComplete(LoadedObject, bSuccess);
			}
		});
	});
}

void UBBMGameDataSubsystem::ClearCurrentRunData()
{
	if (CurrentRunData != nullptr)
	{
		CurrentRunData->ResetRunData();
	}

	if (UGameplayStatics::DoesSaveGameExist(UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex) == true)
	{
		UGameplayStatics::DeleteGameInSlot(UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex);
	}

	// 백업 파일도 함께 정리
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString BakPath = GetBackupFilePath(UBBMRunSaveGame::SlotName);
	if (PlatformFile.FileExists(*BakPath) == true)
	{
		PlatformFile.DeleteFile(*BakPath);
	}
}

FString UBBMGameDataSubsystem::GetSaveFilePath(const FString& InSlotName)
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SaveGames") / (InSlotName + TEXT(".sav")));
}

FString UBBMGameDataSubsystem::GetBackupFilePath(const FString& InSlotName)
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SaveGames") / (InSlotName + TEXT(".bak")));
}

bool UBBMGameDataSubsystem::PrepareBackupBeforeSave(const FString& InSlotName)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString SavPath = GetSaveFilePath(InSlotName);
	const FString BakPath = GetBackupFilePath(InSlotName);

	if (PlatformFile.FileExists(*SavPath) == false)
	{
		return false;
	}

	// 기존 .bak 파일이 존재하면 안전하게 먼저 제거
	if (PlatformFile.FileExists(*BakPath) == true)
	{
		PlatformFile.DeleteFile(*BakPath);
	}

	// 기존 .sav 파일을 .bak 파일로 복제 백업 (원자적 안전을 위해 Move 대신 Copy 수행)
	const bool bCopied = PlatformFile.CopyFile(*BakPath, *SavPath);
	return bCopied;
}

bool UBBMGameDataSubsystem::RestoreFromBackupIfNeeded(const FString& InSlotName)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString SavPath = GetSaveFilePath(InSlotName);
	const FString BakPath = GetBackupFilePath(InSlotName);

	// 이미 .sav 파일이 온전히 존재하면 복구 불필요
	if (PlatformFile.FileExists(*SavPath) == true)
	{
		return false;
	}

	// .sav 파일이 없고 .bak 파일이 존재하면 .sav로 복사 복구
	if (PlatformFile.FileExists(*BakPath) == true)
	{
		const bool bCopied = PlatformFile.CopyFile(*SavPath, *BakPath);
		return bCopied;
	}

	return false;
}
