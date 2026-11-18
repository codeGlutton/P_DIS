#include "UI/ViewModel/BBMSettingsViewModel.h"
#include "UI/ViewModel/BBMTabEntryViewModel.h"
#include "UI/ViewModel/BBMKeyMappingEntryViewModel.h"
#include "UI/ViewModel/BBMKeyMappingCategoryEntryViewModel.h"
#include "Save/BBMGameDataSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UBBMSettingsViewModel::BeginViewModel()
{
	Super::BeginViewModel();

	/* 1. 세이브 데이터 동기화 */

	if (UBBMGameDataSubsystem* GameDataSS = GetGameDataSubsystem())
	{
		UserSettingsChangedHandle = GameDataSS->OnUserSettingsChanged.AddUObject(
			this,
			&UBBMSettingsViewModel::HandleSubsystemSettingsChanged
		);

		CachedSettingsBeforeEdit = GameDataSS->GetUserSettingsData();
	}
	else
	{
		CachedSettingsBeforeEdit = CachedCurrentSettings;
	}

	HandleSubsystemSettingsChanged(CachedSettingsBeforeEdit);

	/* 2. 데이터로 엔트리 구성 */

	AddFixedTabEntries();
}

void UBBMSettingsViewModel::EndViewModel()
{
	RemoveAllKeyMappingCategoryEntries();
	RemoveAllTabs();

	if (CachedGameDataSubsystem.IsValid() == true && UserSettingsChangedHandle.IsValid() == true)
	{
		CachedGameDataSubsystem->OnUserSettingsChanged.Remove(UserSettingsChangedHandle);
	}
	UserSettingsChangedHandle.Reset();
	CachedGameDataSubsystem.Reset();

	Super::EndViewModel();
}

void UBBMSettingsViewModel::OnOpen()
{
	Super::OnOpen();

	if (UBBMGameDataSubsystem* GameDataSS = GetGameDataSubsystem())
	{
		CachedSettingsBeforeEdit = GameDataSS->GetUserSettingsData();
	}
	else
	{
		CachedSettingsBeforeEdit = CachedCurrentSettings;
	}

	HandleSubsystemSettingsChanged(CachedSettingsBeforeEdit);
}

void UBBMSettingsViewModel::OnClosed()
{
	// Apply 또는 Cancel 없이 외부 닫힘/ESC 등으로 종료된 경우 진입 전 스냅샷으로 안전 롤백
	if (bHasPendingEdit == true)
	{
		CancelSettings();
	}

	Super::OnClosed();
}

float UBBMSettingsViewModel::GetMasterVolume() const
{
	return MasterVolume;
}

void UBBMSettingsViewModel::SetMasterVolume(float InMasterVolume)
{
	const float ClampedVolume = FMath::Clamp(InMasterVolume, 0.0f, 1.0f);
	CachedCurrentSettings.MasterVolume = ClampedVolume;

	if (UE_MVVM_SET_PROPERTY_VALUE(MasterVolume, ClampedVolume) == false)
	{
		return;
	}
	
	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

float UBBMSettingsViewModel::GetBGMVolume() const
{
	return BGMVolume;
}

void UBBMSettingsViewModel::SetBGMVolume(float InBGMVolume)
{
	const float ClampedVolume = FMath::Clamp(InBGMVolume, 0.0f, 1.0f);
	CachedCurrentSettings.BGMVolume = ClampedVolume;

	if (UE_MVVM_SET_PROPERTY_VALUE(BGMVolume, ClampedVolume) == false)
	{
		return;
	}

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

float UBBMSettingsViewModel::GetSFXVolume() const
{
	return SFXVolume;
}

void UBBMSettingsViewModel::SetSFXVolume(float InSFXVolume)
{
	const float ClampedVolume = FMath::Clamp(InSFXVolume, 0.0f, 1.0f);
	CachedCurrentSettings.SFXVolume = ClampedVolume;

	if (UE_MVVM_SET_PROPERTY_VALUE(SFXVolume, ClampedVolume) == false)
	{
		return;
	}

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

float UBBMSettingsViewModel::GetDialogueVolume() const
{
	return DialogueVolume;
}

void UBBMSettingsViewModel::SetDialogueVolume(float InDialogueVolume)
{
	const float ClampedVolume = FMath::Clamp(InDialogueVolume, 0.0f, 1.0f);
	CachedCurrentSettings.DialogueVolume = ClampedVolume;

	if (UE_MVVM_SET_PROPERTY_VALUE(DialogueVolume, ClampedVolume) == false)
	{
		return;
	}

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

bool UBBMSettingsViewModel::GetEnableBeatVisualAssist() const
{
	return bEnableBeatVisualAssist;
}

void UBBMSettingsViewModel::SetEnableBeatVisualAssist(bool bInEnableBeatVisualAssist)
{
	CachedCurrentSettings.bEnableBeatVisualAssist = bInEnableBeatVisualAssist;

	if (UE_MVVM_SET_PROPERTY_VALUE(bEnableBeatVisualAssist, bInEnableBeatVisualAssist) == false)
	{
		return;
	}

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

EWindowMode::Type UBBMSettingsViewModel::GetWindowMode() const
{
	return WindowMode;
}

void UBBMSettingsViewModel::SetWindowMode(EWindowMode::Type InWindowMode)
{
	CachedCurrentSettings.WindowMode = InWindowMode;

	if (UE_MVVM_SET_PROPERTY_VALUE(WindowMode, InWindowMode) == false)
	{
		return;
	}

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

FIntPoint UBBMSettingsViewModel::GetScreenResolution() const
{
	return ScreenResolution;
}

void UBBMSettingsViewModel::SetScreenResolution(FIntPoint InScreenResolution)
{
	const FIntPoint ClampedResolution(
		FMath::Max(320, InScreenResolution.X),
		FMath::Max(240, InScreenResolution.Y)
	);
	CachedCurrentSettings.ScreenResolution = ClampedResolution;

	if (UE_MVVM_SET_PROPERTY_VALUE(ScreenResolution, ClampedResolution) == false)
	{
		return;
	}

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

bool UBBMSettingsViewModel::GetVSyncEnabled() const
{
	return bVSyncEnabled;
}

void UBBMSettingsViewModel::SetVSyncEnabled(bool bInVSyncEnabled)
{
	CachedCurrentSettings.bVSyncEnabled = bInVSyncEnabled;

	if (UE_MVVM_SET_PROPERTY_VALUE(bVSyncEnabled, bInVSyncEnabled) == false)
	{
		return;
	}

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

void UBBMSettingsViewModel::SetKeyMappings(const TArray<FBBMKeyCategoryMappingData>& InKeyMappings)
{
	TArray<FBBMKeyCategoryMappingData> SanitizedKeyMappings = InKeyMappings;
	for (FBBMKeyCategoryMappingData& CategoryData : SanitizedKeyMappings)
	{
		CategoryData.KeyMappings.RemoveAll([](const FBBMKeyMappingData& MappingData)
		{
			return MappingData.MappingName.IsNone() == true;
		});
	}
	CachedCurrentSettings.KeyMappings = SanitizedKeyMappings;

	if (UE_MVVM_SET_PROPERTY_VALUE(KeyMappings, SanitizedKeyMappings) == false)
	{
		return;
	}

	RefreshKeyMappingCategoryEntries();

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

void UBBMSettingsViewModel::RemapKey(int32 InCategoryIndex, int32 InEntryIndex, const FInputChord& InNewChord)
{
	if (KeyMappings.IsValidIndex(InCategoryIndex) == false || KeyMappings[InCategoryIndex].KeyMappings.IsValidIndex(InEntryIndex) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMSettingsViewModel::RemapKey] 유효하지 않은 KeyMappings 인덱스 (CategoryIndex=%d, EntryIndex=%d)"), InCategoryIndex, InEntryIndex);
		return;
	}

	if (CachedCurrentSettings.KeyMappings.IsValidIndex(InCategoryIndex) == false || CachedCurrentSettings.KeyMappings[InCategoryIndex].KeyMappings.IsValidIndex(InEntryIndex) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMSettingsViewModel::RemapKey] 유효하지 않은 CachedCurrentSettings.KeyMappings 인덱스 (CategoryIndex=%d, EntryIndex=%d)"), InCategoryIndex, InEntryIndex);
		return;
	}

	FInputChord& CurrentChord = KeyMappings[InCategoryIndex].KeyMappings[InEntryIndex].InputChord;
	FInputChord& CachedChord = CachedCurrentSettings.KeyMappings[InCategoryIndex].KeyMappings[InEntryIndex].InputChord;
	if (CurrentChord == InNewChord)
	{
		return;
	}

	CachedChord = InNewChord;
	CurrentChord = InNewChord;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(KeyMappings);

	if (KeyMappingCategoryEntries.IsValidIndex(InCategoryIndex) == true && KeyMappingCategoryEntries[InCategoryIndex] != nullptr)
	{
		KeyMappingCategoryEntries[InCategoryIndex]->RemapKey(InEntryIndex, InNewChord);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMSettingsViewModel::RemapKey] UBBMKeyMappingCategoryEntryViewModel 접근 실패 (CategoryIndex=%d)"), InCategoryIndex);
	}

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

void UBBMSettingsViewModel::ApplySettingsAndCloseWithAnim()
{
	ApplySettings();
	CloseWithAnim();
}

void UBBMSettingsViewModel::CancelSettingsAndCloseWithAnim()
{
	CancelSettings();
	CloseWithAnim();
}

void UBBMSettingsViewModel::ApplySettings()
{
	if (bHasPendingEdit == false)
	{
		return;
	}

	CachedSettingsBeforeEdit = CachedCurrentSettings;
	if (UBBMGameDataSubsystem* GameDataSS = GetGameDataSubsystem())
	{
		GameDataSS->CommitAndSaveUserSettings();
	}
	else
	{
		HandleSubsystemSettingsChanged(CachedSettingsBeforeEdit);
	}
}

void UBBMSettingsViewModel::CancelSettings()
{
	if (bHasPendingEdit == false)
	{
		return;
	}

	if (UBBMGameDataSubsystem* GameDataSS = GetGameDataSubsystem())
	{
		// OnUserSettingsChanged -> HandleSubsystemSettingsChanged 콜백 경로로 ViewModel 프로퍼티 자동 동기화
		GameDataSS->ApplyUserSettingsRuntime(CachedSettingsBeforeEdit);
	}
	else
	{
		HandleSubsystemSettingsChanged(CachedSettingsBeforeEdit);
	}
}

void UBBMSettingsViewModel::ResetToDefaults()
{
	if (UBBMGameDataSubsystem* GameDataSS = GetGameDataSubsystem())
	{
		GameDataSS->ResetUserSettingsToDefaults();
	}
	else
	{
		HandleSubsystemSettingsChanged(FBBMUserSettingsData());
	}
}

void UBBMSettingsViewModel::AddFixedTabEntries()
{
	// 환경설정 4종 탭(Game, Audio, Graphics, Controls) 엔트리 뷰모델 초기화 및 대리자 바인딩
	static const FName SettingsStringTableId(TEXT("/BBMGameFeatureCore/UI/ST_BBMGameFeatureCore_UI.ST_BBMGameFeatureCore_UI"));
	const FName SettingsTabKeys[] = {
		FName(TEXT("Settings_Tab_Game")),
		FName(TEXT("Settings_Tab_Audio")),
		FName(TEXT("Settings_Tab_Graphics")),
		FName(TEXT("Settings_Tab_Controls"))
	};

	TArray<TObjectPtr<UBBMTabEntryViewModel>> InitialTabEntries;
	InitialTabEntries.Reserve(UE_ARRAY_COUNT(SettingsTabKeys));

	for (const FName& TabKey : SettingsTabKeys)
	{
		UBBMTabEntryViewModel* EntryVM = NewObject<UBBMTabEntryViewModel>(this);
		if (EntryVM != nullptr)
		{
			EntryVM->SetTabText(FText::FromStringTable(SettingsStringTableId, TabKey.ToString()));
			InitialTabEntries.Add(EntryVM);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[UBBMSettingsViewModel::BeginViewModel] UBBMTabEntryViewModel 생성 실패"));
		}
	}

	SetTabEntries(InitialTabEntries);
}

void UBBMSettingsViewModel::RefreshKeyMappingCategoryEntries()
{
	const int32 NumCategories = KeyMappings.Num();
	const int32 NumCategoryEntries = KeyMappingCategoryEntries.Num();
	if (NumCategories != NumCategoryEntries)
	{
		/* 크기 변경 시 VM 재구성 */

		RemoveAllKeyMappingCategoryEntries();

		TArray<TObjectPtr<UBBMKeyMappingCategoryEntryViewModel>> InitialKeyMappingCategoryEntries;
		InitialKeyMappingCategoryEntries.Reserve(NumCategories);

		for (int32 CategoryIndex = 0; CategoryIndex < NumCategories; ++CategoryIndex)
		{
			UBBMKeyMappingCategoryEntryViewModel* CategoryVM = NewObject<UBBMKeyMappingCategoryEntryViewModel>(this);
			if (CategoryVM != nullptr)
			{
				CategoryVM->SetCategoryIndex(CategoryIndex);
				CategoryVM->SetKeyMappings(KeyMappings[CategoryIndex]);
				InitialKeyMappingCategoryEntries.Add(CategoryVM);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[UBBMSettingsViewModel::RefreshKeyMappingCategoryEntries] UBBMKeyMappingCategoryEntryViewModel 생성 실패 (CategoryIndex=%d)"), CategoryIndex);
			}
		}

		SetKeyMappingCategoryEntries(InitialKeyMappingCategoryEntries);
	}
	else
	{
		/* 크기 동일 시 VM 갱신 */

		for (int32 CategoryIndex = 0; CategoryIndex < NumCategories; ++CategoryIndex)
		{
			if (KeyMappingCategoryEntries.IsValidIndex(CategoryIndex) == true && KeyMappingCategoryEntries[CategoryIndex] != nullptr)
			{
				KeyMappingCategoryEntries[CategoryIndex]->SetCategoryIndex(CategoryIndex);
				KeyMappingCategoryEntries[CategoryIndex]->SetKeyMappings(KeyMappings[CategoryIndex]);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[UBBMSettingsViewModel::RefreshKeyMappingCategoryEntries] UBBMKeyMappingCategoryEntryViewModel 접근 실패 (CategoryIndex=%d)"), CategoryIndex);
			}
		}
	}
}

UBBMGameDataSubsystem* UBBMSettingsViewModel::GetGameDataSubsystem()
{
	if (CachedGameDataSubsystem.IsValid() == true)
	{
		return CachedGameDataSubsystem.Get();
	}

	UGameInstance* GameInstance = nullptr;
	if (const UWorld* World = GetWorld())
	{
		GameInstance = World->GetGameInstance();
	}
	if (GameInstance == nullptr)
	{
		GameInstance = GetTypedOuter<UGameInstance>();
	}

	UBBMGameDataSubsystem* GameDataSS = (GameInstance != nullptr) ? GameInstance->GetSubsystem<UBBMGameDataSubsystem>() : nullptr;
	if (GameDataSS == nullptr)
	{
		if (IsTemplate() == false)
		{
			UE_LOG(LogTemp, Error, TEXT("[UBBMSettingsViewModel::GetGameDataSubsystem] Failed to resolve UBBMGameDataSubsystem."));
		}
		return nullptr;
	}

	CachedGameDataSubsystem = GameDataSS;
	return GameDataSS;
}

void UBBMSettingsViewModel::PushCurrentValuesToSubsystem()
{
	if (UBBMGameDataSubsystem* GameDataSS = GetGameDataSubsystem())
	{
		GameDataSS->ApplyUserSettingsRuntime(CachedCurrentSettings);
	}
	else
	{
		HandleSubsystemSettingsChanged(CachedCurrentSettings);
	}
}

void UBBMSettingsViewModel::SyncFromSettingsData(const FBBMUserSettingsData& InSettings)
{
	FBBMUserSettingsData ClampedSettings = InSettings;
	ClampedSettings.ClampValues();

	CachedCurrentSettings.MasterVolume = ClampedSettings.MasterVolume;
	UE_MVVM_SET_PROPERTY_VALUE(MasterVolume, ClampedSettings.MasterVolume);

	CachedCurrentSettings.BGMVolume = ClampedSettings.BGMVolume;
	UE_MVVM_SET_PROPERTY_VALUE(BGMVolume, ClampedSettings.BGMVolume);

	CachedCurrentSettings.SFXVolume = ClampedSettings.SFXVolume;
	UE_MVVM_SET_PROPERTY_VALUE(SFXVolume, ClampedSettings.SFXVolume);

	CachedCurrentSettings.DialogueVolume = ClampedSettings.DialogueVolume;
	UE_MVVM_SET_PROPERTY_VALUE(DialogueVolume, ClampedSettings.DialogueVolume);

	CachedCurrentSettings.bEnableBeatVisualAssist = ClampedSettings.bEnableBeatVisualAssist;
	UE_MVVM_SET_PROPERTY_VALUE(bEnableBeatVisualAssist, ClampedSettings.bEnableBeatVisualAssist);

	CachedCurrentSettings.WindowMode = ClampedSettings.WindowMode;
	UE_MVVM_SET_PROPERTY_VALUE(WindowMode, ClampedSettings.WindowMode);

	CachedCurrentSettings.ScreenResolution = ClampedSettings.ScreenResolution;
	UE_MVVM_SET_PROPERTY_VALUE(ScreenResolution, ClampedSettings.ScreenResolution);

	CachedCurrentSettings.bVSyncEnabled = ClampedSettings.bVSyncEnabled;
	UE_MVVM_SET_PROPERTY_VALUE(bVSyncEnabled, ClampedSettings.bVSyncEnabled);

	CachedCurrentSettings.KeyMappings = ClampedSettings.KeyMappings;
	if (UE_MVVM_SET_PROPERTY_VALUE(KeyMappings, ClampedSettings.KeyMappings) == true)
	{
		RefreshKeyMappingCategoryEntries();
	}

	const bool bInHasPendingEdit = (CachedCurrentSettings != CachedSettingsBeforeEdit);
	UE_MVVM_SET_PROPERTY_VALUE(bHasPendingEdit, bInHasPendingEdit);
}

void UBBMSettingsViewModel::HandleSubsystemSettingsChanged(const FBBMUserSettingsData& NewSettings)
{
	if (bIsSyncingFromSubsystem == true)
	{
		return;
	}

	{
		TGuardValue<bool> SyncGuard(bIsSyncingFromSubsystem, true);
		SyncFromSettingsData(NewSettings);
	}
}

void UBBMSettingsViewModel::SetKeyMappingCategoryEntries(const TArray<TObjectPtr<UBBMKeyMappingCategoryEntryViewModel>>& InKeyMappingCategoryEntries)
{
	UnbindKeyMappingCategoryEntries();

	UE_MVVM_SET_PROPERTY_VALUE(KeyMappingCategoryEntries, InKeyMappingCategoryEntries);

	RebindKeyMappingCategoryEntries();
}

void UBBMSettingsViewModel::RemoveAllKeyMappingCategoryEntries()
{
	if (KeyMappingCategoryEntries.IsEmpty() == true)
	{
		return;
	}

	UnbindKeyMappingCategoryEntries();
	KeyMappingCategoryEntries.Reset();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(KeyMappingCategoryEntries);
}

void UBBMSettingsViewModel::RebindKeyMappingCategoryEntries()
{
	UnbindKeyMappingCategoryEntries();

	KeyMappingCategoryEntryBindings.Reserve(KeyMappingCategoryEntries.Num());
	for (UBBMKeyMappingCategoryEntryViewModel* Entry : KeyMappingCategoryEntries)
	{
		if (Entry == nullptr)
		{
			continue;
		}

		FKeyMappingCategoryEntryBinding NewBinding;
		NewBinding.Entry = Entry;
		NewBinding.ChordChangedHandle = Entry->OnCategoryInputChordChangedNative.AddUObject(
			this,
			&UBBMSettingsViewModel::HandleCategoryInputChordChanged
		);
		KeyMappingCategoryEntryBindings.Add(MoveTemp(NewBinding));
	}
}

void UBBMSettingsViewModel::UnbindKeyMappingCategoryEntries()
{
	for (const FKeyMappingCategoryEntryBinding& Binding : KeyMappingCategoryEntryBindings)
	{
		if (Binding.Entry.IsValid() == false)
		{
			continue;
		}

		if (Binding.ChordChangedHandle.IsValid() == true)
		{
			Binding.Entry->OnCategoryInputChordChangedNative.Remove(Binding.ChordChangedHandle);
		}
	}

	KeyMappingCategoryEntryBindings.Reset();
}

void UBBMSettingsViewModel::HandleCategoryInputChordChanged(int32 InCategoryIndex, int32 InEntryIndex, FName /* InMappingName */, const FInputChord& InNewChord)
{
	RemapKey(InCategoryIndex, InEntryIndex, InNewChord);
}
