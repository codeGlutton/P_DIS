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
#include "Misc/App.h"
#include "Async/Async.h"
#include "Global/BBMGlobalSettings.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameUserSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "EnhancedActionKeyMapping.h"
#include "EnhancedInputLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

UBBMGameDataSubsystem::UBBMGameDataSubsystem() :
	UserSaveData(nullptr),
	CurrentRunData(nullptr),
	bIsAsyncOperationInProgress(false),
	bAppliedInitialViewportSettings(false),
	RuntimeIMC(nullptr),
	CachedDefaultIMC(nullptr),
	CachedModifierIMC(nullptr),
	CachedShiftChordAction(nullptr),
	CachedCtrlChordAction(nullptr),
	CachedAltChordAction(nullptr)
{
}

void UBBMGameDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 1. Default IMC 및 모디파이어 IMC/Chord Action 로드 후 복제 RuntimeIMC와 기본 키 매핑 맵 캐싱
	InitializeInputMappingsFromSettings();

	// 2. 기본 인메모리 세이브 객체 생성 및 Default IMC 기반 초기 키 매핑 채우기 (디스크 로드 전 선행)
	UserSaveData = NewObject<UBBMUserSaveGame>(this, UBBMUserSaveGame::StaticClass());
	if (UserSaveData != nullptr)
	{
		FBBMUserSettingsData InitialUserSettings = UserSaveData->GetSettingsData();
		InitialUserSettings.KeyMappings = CachedDefaultKeyMappings;
		UserSaveData->SetSettingsData(InitialUserSettings);
	}
	CurrentRunData = NewObject<UBBMRunSaveGame>(this, UBBMRunSaveGame::StaticClass());

	// 3. 필요 시 백업(.bak) 파일로부터 복구 시도 후 디스크 유저 세팅 로드
	RestoreFromBackupIfNeeded(UBBMUserSaveGame::SlotName);
	if (UGameplayStatics::DoesSaveGameExist(UBBMUserSaveGame::SlotName, UBBMUserSaveGame::UserIndex) == true)
	{
		USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(UBBMUserSaveGame::SlotName, UBBMUserSaveGame::UserIndex);
		if (UBBMUserSaveGame* CastedUser = Cast<UBBMUserSaveGame>(Loaded))
		{
			UserSaveData = CastedUser;
		}
	}

	// 4. 필요 시 백업(.bak) 파일로부터 복구 시도 후 디스크 런 세이브 로드
	RestoreFromBackupIfNeeded(UBBMRunSaveGame::SlotName);
	if (UGameplayStatics::DoesSaveGameExist(UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex) == true)
	{
		USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(UBBMRunSaveGame::SlotName, UBBMRunSaveGame::UserIndex);
		if (UBBMRunSaveGame* CastedRun = Cast<UBBMRunSaveGame>(Loaded))
		{
			CurrentRunData = CastedRun;
		}
	}

	// 5. 초기 유저 환경설정 정규화 및 오디오/화면/RuntimeIMC 메모리 반영
	if (UserSaveData != nullptr)
	{
		const FBBMUserSettingsData InitialSettings = GetUserSettingsData();
		ApplyUserSettingsRuntime(InitialSettings);
	}

	// Standalone 초기화 시점에는 GameViewport가 아직 생성 전일 수 있으므로 생성 완료 시 1회 화면 설정 보정 바인딩
	if (GEngine != nullptr && GEngine->GameViewport == nullptr && FApp::CanEverRender() == true && IsRunningCommandlet() == false)
	{
		ViewportCreatedHandle = UGameViewportClient::OnViewportCreated().AddUObject(
			this,
			&UBBMGameDataSubsystem::HandleViewportCreatedForInitialSettings
		);
	}
}

void UBBMGameDataSubsystem::Deinitialize()
{
	if (ViewportCreatedHandle.IsValid() == true)
	{
		UGameViewportClient::OnViewportCreated().Remove(ViewportCreatedHandle);
		ViewportCreatedHandle.Reset();
	}

	CachedDefaultIMC = nullptr;
	RuntimeIMC = nullptr;
	CachedModifierIMC = nullptr;
	CachedShiftChordAction = nullptr;
	CachedCtrlChordAction = nullptr;
	CachedAltChordAction = nullptr;
	CachedDefaultKeyMappings.Empty();

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

void UBBMGameDataSubsystem::ResetAndInitializeNewRun(const UBBMPlayerCharacterDefinition* PlayerDef)
{
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
			ApplyUserSettingsRuntime(GetUserSettingsData());
		}
		OnComplete.ExecuteIfBound();
	});
}

const FBBMUserSettingsData& UBBMGameDataSubsystem::GetUserSettingsData() const
{
	if (UserSaveData == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMGameDataSubsystem::GetUserSettingsData] UserSaveData 미생성"));
		static const FBBMUserSettingsData EmptyDefaultSettings;
		return EmptyDefaultSettings;
	}

	return UserSaveData->GetSettingsData();
}

void UBBMGameDataSubsystem::ApplyUserSettingsRuntime(const FBBMUserSettingsData& InSettings)
{
	FBBMUserSettingsData SanitizedSettings = InSettings;
	SanitizedSettings.ClampValues();

	if (UserSaveData != nullptr)
	{
		UserSaveData->SetSettingsData(SanitizedSettings);
	}

	ApplyAudioSettingsInternal(SanitizedSettings);
	ApplyLanguageSettingsInternal(SanitizedSettings);
	ApplyDisplaySettingsInternal(SanitizedSettings);
	ApplyInputSettingsInternal(SanitizedSettings);

	if (OnUserSettingsChanged.IsBound() == true)
	{
		OnUserSettingsChanged.Broadcast(SanitizedSettings);
	}
}

void UBBMGameDataSubsystem::CommitAndSaveUserSettings(FSimpleDelegate OnComplete)
{
	const FBBMUserSettingsData CurrentSettings = GetUserSettingsData();
	ApplyUserSettingsRuntime(CurrentSettings);
	SaveUserSettingAsync(OnComplete);
}

void UBBMGameDataSubsystem::ResetUserSettingsToDefaults()
{
	FBBMUserSettingsData DefaultSettings;
	DefaultSettings.KeyMappings = CachedDefaultKeyMappings;
	ApplyUserSettingsRuntime(DefaultSettings);
}

void UBBMGameDataSubsystem::RemapByName(FName MappingName, const FInputChord& Chord, bool bNeedRebuild)
{
	if (RuntimeIMC == nullptr || MappingName.IsNone() == true)
	{
		return;
	}

	const TArray<FEnhancedActionKeyMapping>& Mappings = RuntimeIMC->GetMappings();
	for (int32 Index = 0; Index < Mappings.Num(); ++Index)
	{
		const FEnhancedActionKeyMapping& Existing = Mappings[Index];
		if (Existing.GetMappingName() != MappingName)
		{
			continue;
		}

		FEnhancedActionKeyMapping& Mapping = RuntimeIMC->GetMapping(Index);
		Mapping.Key = Chord.Key;

		// 이전에 동적/정적으로 부착된 Chord 트리거를 제거한 뒤 새 Chord 상태에 맞춰 재구성함
		Mapping.Triggers.RemoveAll([](const TObjectPtr<UInputTrigger>& Trigger)
		{
			return Trigger != nullptr && Trigger->IsA<UInputTriggerChordAction>();
		});
		AddChordTriggers(Mapping, Chord);
		break;
	}

	if (bNeedRebuild == true)
	{
		RebuildControlMappingsIfRegistered();
	}
}

void UBBMGameDataSubsystem::RegisterIMCs(ULocalPlayer* InLocalPlayer)
{
	if (InLocalPlayer == nullptr)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(InLocalPlayer);
	if (InputSS == nullptr)
	{
		return;
	}

	// 모디파이어 전용 IMC를 RuntimeIMC보다 높은 우선순위로 등록해 동일 프레임에 ChordAction 상태가 먼저 평가되도록 함
	if (CachedModifierIMC != nullptr && InputSS->HasMappingContext(CachedModifierIMC) == false)
	{
		InputSS->AddMappingContext(CachedModifierIMC, 1);
	}

	if (RuntimeIMC != nullptr && InputSS->HasMappingContext(RuntimeIMC) == false)
	{
		InputSS->AddMappingContext(RuntimeIMC, 0);
	}

	// LocalPlayer 확보 시점에 현재 유저 설정(키 리매핑 포함)을 다시 적용해 활성 컨트롤 매핑 리빌드까지 확정함
	ApplyUserSettingsRuntime(GetUserSettingsData());
}

void UBBMGameDataSubsystem::InitializeInputMappingsFromSettings()
{
	CachedDefaultIMC = nullptr;
	RuntimeIMC = nullptr;
	CachedModifierIMC = nullptr;
	CachedShiftChordAction = nullptr;
	CachedCtrlChordAction = nullptr;
	CachedAltChordAction = nullptr;
	CachedDefaultKeyMappings.Empty();

	const UBBMGlobalSettings* GlobalSettings = GetDefault<UBBMGlobalSettings>();
	if (GlobalSettings == nullptr)
	{
		return;
	}

	if (GlobalSettings->ShiftChordAction.IsNull() == false)
	{
		CachedShiftChordAction = GlobalSettings->ShiftChordAction.LoadSynchronous();
	}
	if (GlobalSettings->CtrlChordAction.IsNull() == false)
	{
		CachedCtrlChordAction = GlobalSettings->CtrlChordAction.LoadSynchronous();
	}
	if (GlobalSettings->AltChordAction.IsNull() == false)
	{
		CachedAltChordAction = GlobalSettings->AltChordAction.LoadSynchronous();
	}
	if (GlobalSettings->ModifierInputMappingContext.IsNull() == false)
	{
		CachedModifierIMC = GlobalSettings->ModifierInputMappingContext.LoadSynchronous();
	}
	if (GlobalSettings->DefaultInputMappingContext.IsNull() == false)
	{
		CachedDefaultIMC = GlobalSettings->DefaultInputMappingContext.LoadSynchronous();
	}

	if (CachedDefaultIMC == nullptr)
	{
		return;
	}

	RuntimeIMC = DuplicateObject<UInputMappingContext>(CachedDefaultIMC, this);
	if (RuntimeIMC == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMGameDataSubsystem::InitializeInputMappingsFromSettings] RuntimeIMC 복제 생성 실패"));
		return;
	}

	const TArray<FEnhancedActionKeyMapping>& DefaultMappings = CachedDefaultIMC->GetMappings();
	TMap<FName, int32> CategoryIndexMap;
	TSet<FName> SeenMappingNames;

	for (const FEnhancedActionKeyMapping& Mapping : DefaultMappings)
	{
		const FName MappingName = Mapping.GetMappingName();
		if (MappingName.IsNone() == true || SeenMappingNames.Contains(MappingName) == true)
		{
			continue;
		}
		SeenMappingNames.Add(MappingName);

		const FText CategoryText = Mapping.GetDisplayCategory();
		const FName CategoryKey(*CategoryText.ToString());

		int32 CategoryIndex = INDEX_NONE;
		if (const int32* FoundCategoryIndex = CategoryIndexMap.Find(CategoryKey))
		{
			CategoryIndex = *FoundCategoryIndex;
		}
		else
		{
			FBBMKeyCategoryMappingData NewCategory;
			NewCategory.CategoryName = CategoryText;
			CategoryIndex = CachedDefaultKeyMappings.Add(NewCategory);
			CategoryIndexMap.Add(CategoryKey, CategoryIndex);
		}

		FBBMKeyMappingData MappingEntry;
		MappingEntry.MappingName = MappingName;
		const FText& RawDisplayName = Mapping.GetDisplayName();
		MappingEntry.DisplayName = (RawDisplayName.IsEmpty() == false) ? RawDisplayName : FText::FromName(MappingName);
		MappingEntry.InputChord = ExtractChordFromMapping(Mapping);

		CachedDefaultKeyMappings[CategoryIndex].KeyMappings.Add(MappingEntry);
	}
}

FInputChord UBBMGameDataSubsystem::ExtractChordFromMapping(const FEnhancedActionKeyMapping& InMapping) const
{
	FInputChord ResultChord(InMapping.Key);

	for (const TObjectPtr<UInputTrigger>& Trigger : InMapping.Triggers)
	{
		const UInputTriggerChordAction* ChordTrigger = Cast<UInputTriggerChordAction>(Trigger.Get());
		if (ChordTrigger == nullptr || ChordTrigger->ChordAction == nullptr)
		{
			continue;
		}

		if (CachedShiftChordAction != nullptr && ChordTrigger->ChordAction == CachedShiftChordAction)
		{
			ResultChord.bShift = true;
		}
		else if (CachedCtrlChordAction != nullptr && ChordTrigger->ChordAction == CachedCtrlChordAction)
		{
			ResultChord.bCtrl = true;
		}
		else if (CachedAltChordAction != nullptr && ChordTrigger->ChordAction == CachedAltChordAction)
		{
			ResultChord.bAlt = true;
		}
	}

	return ResultChord;
}

void UBBMGameDataSubsystem::AddChordTriggers(FEnhancedActionKeyMapping& InOutMapping, const FInputChord& InChord)
{
	if (RuntimeIMC == nullptr)
	{
		return;
	}

	auto AppendChordTriggerIfNeeded = [this, &InOutMapping](bool bEnabled, const UInputAction* InChordAction)
	{
		if (bEnabled == false || InChordAction == nullptr)
		{
			return;
		}

		UInputTriggerChordAction* ChordTrigger = NewObject<UInputTriggerChordAction>(RuntimeIMC);
		if (ChordTrigger != nullptr)
		{
			ChordTrigger->ChordAction = InChordAction;
			InOutMapping.Triggers.Add(ChordTrigger);
		}
	};

	AppendChordTriggerIfNeeded(InChord.bShift, CachedShiftChordAction);
	AppendChordTriggerIfNeeded(InChord.bCtrl, CachedCtrlChordAction);
	AppendChordTriggerIfNeeded(InChord.bAlt, CachedAltChordAction);
}

void UBBMGameDataSubsystem::RebuildControlMappingsIfRegistered() const
{
	if (RuntimeIMC == nullptr)
	{
		return;
	}

	// RuntimeIMC를 실제로 등록 중인 활성 EnhancedInput 서브시스템이 있을 때만 리빌드를 요청하므로 초기화 초기 시점에서도 안전함
	UEnhancedInputLibrary::RequestRebuildControlMappingsUsingContext(RuntimeIMC);
}

void UBBMGameDataSubsystem::ApplyAudioSettingsInternal(const FBBMUserSettingsData& InSettings)
{
	if (GEngine == nullptr || GEngine->UseSound() == false)
	{
		return;
	}

	const UBBMGlobalSettings* GlobalSettings = GetDefault<UBBMGlobalSettings>();
	if (GlobalSettings == nullptr)
	{
		return;
	}

	USoundMix* SoundMix = GlobalSettings->UserSettingsSoundMix.LoadSynchronous();
	if (SoundMix == nullptr)
	{
		return;
	}

	const UWorld* World = GetWorld();
	FAudioDeviceHandle AudioDevice = (World != nullptr) ? World->GetAudioDevice() : GEngine->GetMainAudioDevice();
	if (AudioDevice.IsValid() == false)
	{
		return;
	}

	AudioDevice->PushSoundMixModifier(SoundMix);

	const TPair<const TSoftObjectPtr<USoundClass>*, float> ClassVolumePairs[] =
	{
		{ &GlobalSettings->MasterSoundClass, InSettings.MasterVolume },
		{ &GlobalSettings->BGMSoundClass, InSettings.BGMVolume },
		{ &GlobalSettings->SFXSoundClass, InSettings.SFXVolume },
		{ &GlobalSettings->DialogueSoundClass, InSettings.DialogueVolume }
	};

	for (const TPair<const TSoftObjectPtr<USoundClass>*, float>& Pair : ClassVolumePairs)
	{
		if (Pair.Key == nullptr || Pair.Key->IsNull() == true)
		{
			continue;
		}

		if (USoundClass* SoundClass = Pair.Key->LoadSynchronous())
		{
			AudioDevice->SetSoundMixClassOverride(SoundMix, SoundClass, Pair.Value, 1.0f, 0.0f, false);
		}
	}
}

void UBBMGameDataSubsystem::ApplyLanguageSettingsInternal(const FBBMUserSettingsData& InSettings)
{
	// 에디터 및 커맨드릿 환경에서는 에디터 UI 언어가 동반 변경되지 않도록 전환을 건너뜀
	if (GIsEditor == true || IsRunningCommandlet() == true)
	{
		return;
	}

	if (InSettings.Language.IsEmpty() == true)
	{
		return;
	}

	FInternationalization& I18n = FInternationalization::Get();
	if (I18n.GetCurrentLanguage()->GetName() == InSettings.Language)
	{
		return;
	}

	if (I18n.SetCurrentLanguage(InSettings.Language) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMGameDataSubsystem::ApplyLanguageSettingsInternal] 언어 전환 실패 (Language=%s)"), *InSettings.Language);
	}
}

void UBBMGameDataSubsystem::ApplyDisplaySettingsInternal(const FBBMUserSettingsData& InSettings)
{
	if (GEngine == nullptr)
	{
		return;
	}

	UGameUserSettings* EngineUserSettings = GEngine->GetGameUserSettings();
	if (EngineUserSettings == nullptr)
	{
		return;
	}

	EngineUserSettings->SetVSyncEnabled(InSettings.bVSyncEnabled);

#if PLATFORM_DESKTOP
	EngineUserSettings->SetFullscreenMode(InSettings.WindowMode.GetValue());
	EngineUserSettings->SetScreenResolution(InSettings.ScreenResolution);
#endif

	// 실제 렌더링 환경에서만 뷰포트 해상도 및 창모드/VSync 즉각 반영 (SaveSettings는 호출하지 않음)
	if (FApp::CanEverRender() == true && IsRunningCommandlet() == false)
	{
		EngineUserSettings->ApplyResolutionSettings(false);
		EngineUserSettings->ApplyNonResolutionSettings();
		EngineUserSettings->RequestUIUpdate();
	}
}

void UBBMGameDataSubsystem::ApplyInputSettingsInternal(const FBBMUserSettingsData& InSettings)
{
	if (RuntimeIMC == nullptr)
	{
		return;
	}

	for (const FBBMKeyCategoryMappingData& CategoryData : InSettings.KeyMappings)
	{
		for (const FBBMKeyMappingData& KeyData : CategoryData.KeyMappings)
		{
			RemapByName(KeyData.MappingName, KeyData.InputChord, false);
		}
	}

	RebuildControlMappingsIfRegistered();
}

void UBBMGameDataSubsystem::HandleViewportCreatedForInitialSettings()
{
	if (bAppliedInitialViewportSettings == true)
	{
		return;
	}

	bAppliedInitialViewportSettings = true;
	ApplyDisplaySettingsInternal(GetUserSettingsData());

	if (ViewportCreatedHandle.IsValid() == true)
	{
		UGameViewportClient::OnViewportCreated().Remove(ViewportCreatedHandle);
		ViewportCreatedHandle.Reset();
	}
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
	if (OnSaveStartedNative.IsBound() == true)
	{
		OnSaveStartedNative.Broadcast();
	}
	if (OnSaveStarted.IsBound() == true)
	{
		OnSaveStarted.Broadcast();
	}

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
			if (WeakThis.IsValid() == true)
			{
				WeakThis->bIsAsyncOperationInProgress = false;
				if (WeakThis->OnSaveCompletedNative.IsBound() == true)
				{
					WeakThis->OnSaveCompletedNative.Broadcast(bSaved);
				}
				if (WeakThis->OnSaveCompleted.IsBound() == true)
				{
					WeakThis->OnSaveCompleted.Broadcast(bSaved);
				}
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
