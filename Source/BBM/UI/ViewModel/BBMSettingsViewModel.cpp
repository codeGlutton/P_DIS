#include "UI/ViewModel/BBMSettingsViewModel.h"
#include "UI/ViewModel/BBMComboBoxTextViewModel.h"
#include "UI/ViewModel/BBMTabEntryViewModel.h"
#include "UI/ViewModel/BBMKeyMappingEntryViewModel.h"
#include "UI/ViewModel/BBMKeyMappingCategoryEntryViewModel.h"
#include "Global/BBMGlobalSettings.h"
#include "Save/BBMGameDataSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

namespace
{
	template <typename OptionStructType>
	void PopulateComboBoxFromOptions(UBBMComboBoxTextViewModel* InComboBoxVM, const TArray<OptionStructType>& InOptions)
	{
		static_assert(TIsDerivedFrom<OptionStructType, FBBMSelectionDataBase>::IsDerived, "OptionStructType must derive from FBBMSelectionDataBase");

		if (InComboBoxVM == nullptr)
		{
			return;
		}

		InComboBoxVM->RemoveAllComboBoxTextEntries();
		for (const FBBMSelectionDataBase& Option : InOptions)
		{
			InComboBoxVM->AddComboBoxTextEntry(Option.Key, Option.DisplayName);
		}
	}

	template <typename OptionStructType, typename ValueType>
	FName FindOptionKeyForValue(const TArray<OptionStructType>& InOptions, const ValueType& InValue)
	{
		static_assert(TIsDerivedFrom<OptionStructType, FBBMSelectionDataBase>::IsDerived, "OptionStructType must derive from FBBMSelectionDataBase");

		OptionStructType TargetOption;
		TargetOption.Value = InValue;
		for (const FBBMSelectionDataBase& Option : InOptions)
		{
			if (Option.IsSameValue(TargetOption) == true)
			{
				return Option.Key;
			}
		}

		return NAME_None;
	}

	template <typename OptionStructType>
	const OptionStructType* FindOptionByKey(const TArray<OptionStructType>& InOptions, FName InKey)
	{
		static_assert(TIsDerivedFrom<OptionStructType, FBBMSelectionDataBase>::IsDerived, "OptionStructType must derive from FBBMSelectionDataBase");

		for (const OptionStructType& Option : InOptions)
		{
			if (Option.IsSameKey(InKey) == true)
			{
				return &Option;
			}
		}

		return nullptr;
	}

	bool ConvertCheckBoxStateToBool(ECheckBoxState InCheckBoxState)
	{
		return (InCheckBoxState == ECheckBoxState::Checked);
	}

	ECheckBoxState ConvertBoolToCheckBoxState(bool bInEnabled)
	{
		return (bInEnabled == true) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
}

UBBMSettingsViewModel::UBBMSettingsViewModel()
{
	LanguageComboBoxViewModel = CreateDefaultSubobject<UBBMComboBoxTextViewModel>(TEXT("LanguageComboBoxViewModel"));
	WindowModeComboBoxViewModel = CreateDefaultSubobject<UBBMComboBoxTextViewModel>(TEXT("WindowModeComboBoxViewModel"));
	ScreenResolutionComboBoxViewModel = CreateDefaultSubobject<UBBMComboBoxTextViewModel>(TEXT("ScreenResolutionComboBoxViewModel"));
}

void UBBMSettingsViewModel::BeginViewModel()
{
	Super::BeginViewModel();

	/* 1. 고정 크기 엔트리 구성 */

	AddFixedTabEntries();
	AddFixedComboBoxEntries();

	/* 2. 세이브 데이터 동기화 */

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
}

void UBBMSettingsViewModel::EndViewModel()
{
	RemoveAllComboBoxEntries();
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

ECheckBoxState UBBMSettingsViewModel::GetEnableBeatVisualAssist() const
{
	return EnableBeatVisualAssist;
}

void UBBMSettingsViewModel::SetEnableBeatVisualAssist(ECheckBoxState InEnableBeatVisualAssist)
{
	CachedCurrentSettings.bEnableBeatVisualAssist = ConvertCheckBoxStateToBool(InEnableBeatVisualAssist);

	if (UE_MVVM_SET_PROPERTY_VALUE(EnableBeatVisualAssist, InEnableBeatVisualAssist) == false)
	{
		return;
	}

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

FName UBBMSettingsViewModel::GetLanguageKey() const
{
	return LanguageKey;
}

void UBBMSettingsViewModel::SetLanguageKey(FName InLanguageKey)
{
	const UBBMGlobalSettings* GlobalSettings = GetDefault<UBBMGlobalSettings>();
	const FBBMStringSelectionData* FoundOption = (GlobalSettings != nullptr)
		? FindOptionByKey(GlobalSettings->LanguageOptions, InLanguageKey)
		: nullptr;
	if (FoundOption == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMSettingsViewModel::SetLanguageKey] 미등록 언어 키(%s)"), *InLanguageKey.ToString());
		return;
	}

	FString SanitizedLanguage = FoundOption->Value.TrimStartAndEnd();
	if (SanitizedLanguage.IsEmpty() == true)
	{
		SanitizedLanguage = TEXT("ko-KR");
	}
	CachedCurrentSettings.Language = SanitizedLanguage;

	if (UE_MVVM_SET_PROPERTY_VALUE(LanguageKey, InLanguageKey) == false)
	{
		return;
	}

	RefreshComboBoxSelection(LanguageComboBoxViewModel, LanguageKey);

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

FName UBBMSettingsViewModel::GetWindowModeKey() const
{
	return WindowModeKey;
}

void UBBMSettingsViewModel::SetWindowModeKey(FName InWindowModeKey)
{
	const UBBMGlobalSettings* GlobalSettings = GetDefault<UBBMGlobalSettings>();
	const FBBMWindowModeSelectionData* FoundOption = (GlobalSettings != nullptr)
		? FindOptionByKey(GlobalSettings->WindowModeOptions, InWindowModeKey)
		: nullptr;
	if (FoundOption == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMSettingsViewModel::SetWindowModeKey] 미등록 창 모드 키(%s)"), *InWindowModeKey.ToString());
		return;
	}

	CachedCurrentSettings.WindowMode = FoundOption->Value;

	if (UE_MVVM_SET_PROPERTY_VALUE(WindowModeKey, InWindowModeKey) == false)
	{
		return;
	}

	RefreshComboBoxSelection(WindowModeComboBoxViewModel, WindowModeKey);

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

FName UBBMSettingsViewModel::GetScreenResolutionKey() const
{
	return ScreenResolutionKey;
}

void UBBMSettingsViewModel::SetScreenResolutionKey(FName InScreenResolutionKey)
{
	const UBBMGlobalSettings* GlobalSettings = GetDefault<UBBMGlobalSettings>();
	const FBBMIntPointSelectionData* FoundOption = (GlobalSettings != nullptr)
		? FindOptionByKey(GlobalSettings->ScreenResolutionOptions, InScreenResolutionKey)
		: nullptr;
	if (FoundOption == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMSettingsViewModel::SetScreenResolutionKey] 미등록 해상도 키(%s)"), *InScreenResolutionKey.ToString());
		return;
	}

	CachedCurrentSettings.ScreenResolution = FIntPoint(
		FMath::Max(320, FoundOption->Value.X),
		FMath::Max(240, FoundOption->Value.Y)
	);

	if (UE_MVVM_SET_PROPERTY_VALUE(ScreenResolutionKey, InScreenResolutionKey) == false)
	{
		return;
	}

	RefreshComboBoxSelection(ScreenResolutionComboBoxViewModel, ScreenResolutionKey);

	if (bIsSyncingFromSubsystem == false)
	{
		PushCurrentValuesToSubsystem();
	}
}

ECheckBoxState UBBMSettingsViewModel::GetVSyncEnabled() const
{
	return VSyncEnabled;
}

void UBBMSettingsViewModel::SetVSyncEnabled(ECheckBoxState InVSyncEnabled)
{
	CachedCurrentSettings.bVSyncEnabled = ConvertCheckBoxStateToBool(InVSyncEnabled);

	if (UE_MVVM_SET_PROPERTY_VALUE(VSyncEnabled, InVSyncEnabled) == false)
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

	RefreshKeyMappingSingleEntry(InCategoryIndex, InEntryIndex, InNewChord);

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

void UBBMSettingsViewModel::AddFixedComboBoxEntries()
{
	const UBBMGlobalSettings* GlobalSettings = GetDefault<UBBMGlobalSettings>();
	if (GlobalSettings == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMSettingsViewModel::AddFixedComboBoxEntries] UBBMGlobalSettings CDO 조회 실패"));
		return;
	}

	UnbindComboBoxViewModels();

	PopulateComboBoxFromOptions(LanguageComboBoxViewModel, GlobalSettings->LanguageOptions);
	PopulateComboBoxFromOptions(WindowModeComboBoxViewModel, GlobalSettings->WindowModeOptions);
	PopulateComboBoxFromOptions(ScreenResolutionComboBoxViewModel, GlobalSettings->ScreenResolutionOptions);

	RebindComboBoxViewModels();
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

void UBBMSettingsViewModel::RemoveAllComboBoxEntries()
{
	UnbindComboBoxViewModels();

	if (LanguageComboBoxViewModel != nullptr)
	{
		LanguageComboBoxViewModel->RemoveAllComboBoxTextEntries();
	}
	if (WindowModeComboBoxViewModel != nullptr)
	{
		WindowModeComboBoxViewModel->RemoveAllComboBoxTextEntries();
	}
	if (ScreenResolutionComboBoxViewModel != nullptr)
	{
		ScreenResolutionComboBoxViewModel->RemoveAllComboBoxTextEntries();
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
	UE_MVVM_SET_PROPERTY_VALUE(EnableBeatVisualAssist, ConvertBoolToCheckBoxState(ClampedSettings.bEnableBeatVisualAssist));

	const UBBMGlobalSettings* GlobalSettings = GetDefault<UBBMGlobalSettings>();
	const FName MatchedLanguageKey = (GlobalSettings != nullptr)
		? FindOptionKeyForValue(GlobalSettings->LanguageOptions, ClampedSettings.Language)
		: NAME_None;
	const FName MatchedWindowModeKey = (GlobalSettings != nullptr)
		? FindOptionKeyForValue(GlobalSettings->WindowModeOptions, ClampedSettings.WindowMode.GetValue())
		: NAME_None;
	const FName MatchedResolutionKey = (GlobalSettings != nullptr)
		? FindOptionKeyForValue(GlobalSettings->ScreenResolutionOptions, ClampedSettings.ScreenResolution)
		: NAME_None;

	CachedCurrentSettings.Language = ClampedSettings.Language;
	if (UE_MVVM_SET_PROPERTY_VALUE(LanguageKey, MatchedLanguageKey) == true)
	{
		RefreshComboBoxSelection(LanguageComboBoxViewModel, MatchedLanguageKey);
	}

	CachedCurrentSettings.WindowMode = ClampedSettings.WindowMode;
	if (UE_MVVM_SET_PROPERTY_VALUE(WindowModeKey, MatchedWindowModeKey) == true)
	{
		RefreshComboBoxSelection(WindowModeComboBoxViewModel, MatchedWindowModeKey);
	}

	CachedCurrentSettings.ScreenResolution = ClampedSettings.ScreenResolution;
	if (UE_MVVM_SET_PROPERTY_VALUE(ScreenResolutionKey, MatchedResolutionKey) == true)
	{
		RefreshComboBoxSelection(ScreenResolutionComboBoxViewModel, MatchedResolutionKey);
	}

	CachedCurrentSettings.bVSyncEnabled = ClampedSettings.bVSyncEnabled;
	UE_MVVM_SET_PROPERTY_VALUE(VSyncEnabled, ConvertBoolToCheckBoxState(ClampedSettings.bVSyncEnabled));

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

	TGuardValue<bool> SyncGuard(bIsSyncingFromSubsystem, true);
	SyncFromSettingsData(NewSettings);
}

void UBBMSettingsViewModel::SetKeyMappingCategoryEntries(const TArray<TObjectPtr<UBBMKeyMappingCategoryEntryViewModel>>& InKeyMappingCategoryEntries)
{
	UnbindKeyMappingCategoryEntries();

	UE_MVVM_SET_PROPERTY_VALUE(KeyMappingCategoryEntries, InKeyMappingCategoryEntries);

	RebindKeyMappingCategoryEntries();
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

void UBBMSettingsViewModel::RefreshKeyMappingSingleEntry(int32 InCategoryIndex, int32 InEntryIndex, const FInputChord& InNewChord)
{
	if (bIsSyncingKeyMappings == true)
	{
		return;
	}
	TGuardValue<bool> KeyMappingSyncGuard(bIsSyncingKeyMappings, true);

	if (KeyMappingCategoryEntries.IsValidIndex(InCategoryIndex) == true && KeyMappingCategoryEntries[InCategoryIndex] != nullptr)
	{
		KeyMappingCategoryEntries[InCategoryIndex]->RemapKey(InEntryIndex, InNewChord);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMSettingsViewModel::RemapKey] UBBMKeyMappingCategoryEntryViewModel 접근 실패 (CategoryIndex=%d)"), InCategoryIndex);
	}
}

void UBBMSettingsViewModel::RefreshKeyMappingCategoryEntries()
{
	if (bIsSyncingKeyMappings == true)
	{
		return;
	}
	TGuardValue<bool> KeyMappingSyncGuard(bIsSyncingKeyMappings, true);

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

void UBBMSettingsViewModel::HandleCategoryInputChordChanged(int32 InCategoryIndex, int32 InEntryIndex, FName /* InMappingName */, const FInputChord& InNewChord)
{
	if (bIsSyncingKeyMappings == true)
	{
		return;
	}
	TGuardValue<bool> KeyMappingSyncGuard(bIsSyncingKeyMappings, true);

	RemapKey(InCategoryIndex, InEntryIndex, InNewChord);
}

void UBBMSettingsViewModel::RebindComboBoxViewModels()
{
	UnbindComboBoxViewModels();

	if (LanguageComboBoxViewModel != nullptr)
	{
		LanguageComboChangedHandle = LanguageComboBoxViewModel->OnSelectedKeyChangedNative.AddUObject(
			this,
			&UBBMSettingsViewModel::HandleLanguageComboBoxSelectedKeyChanged
		);
	}

	if (WindowModeComboBoxViewModel != nullptr)
	{
		WindowModeComboChangedHandle = WindowModeComboBoxViewModel->OnSelectedKeyChangedNative.AddUObject(
			this,
			&UBBMSettingsViewModel::HandleWindowModeComboBoxSelectedKeyChanged
		);
	}

	if (ScreenResolutionComboBoxViewModel != nullptr)
	{
		ScreenResolutionComboChangedHandle = ScreenResolutionComboBoxViewModel->OnSelectedKeyChangedNative.AddUObject(
			this,
			&UBBMSettingsViewModel::HandleScreenResolutionComboBoxSelectedKeyChanged
		);
	}
}

void UBBMSettingsViewModel::UnbindComboBoxViewModels()
{
	if (LanguageComboBoxViewModel != nullptr && LanguageComboChangedHandle.IsValid() == true)
	{
		LanguageComboBoxViewModel->OnSelectedKeyChangedNative.Remove(LanguageComboChangedHandle);
	}
	LanguageComboChangedHandle.Reset();

	if (WindowModeComboBoxViewModel != nullptr && WindowModeComboChangedHandle.IsValid() == true)
	{
		WindowModeComboBoxViewModel->OnSelectedKeyChangedNative.Remove(WindowModeComboChangedHandle);
	}
	WindowModeComboChangedHandle.Reset();

	if (ScreenResolutionComboBoxViewModel != nullptr && ScreenResolutionComboChangedHandle.IsValid() == true)
	{
		ScreenResolutionComboBoxViewModel->OnSelectedKeyChangedNative.Remove(ScreenResolutionComboChangedHandle);
	}
	ScreenResolutionComboChangedHandle.Reset();
}

void UBBMSettingsViewModel::RefreshComboBoxSelections()
{
	RefreshComboBoxSelection(LanguageComboBoxViewModel, LanguageKey);
	RefreshComboBoxSelection(WindowModeComboBoxViewModel, WindowModeKey);
	RefreshComboBoxSelection(ScreenResolutionComboBoxViewModel, ScreenResolutionKey);
}

void UBBMSettingsViewModel::RefreshComboBoxSelection(UBBMComboBoxTextViewModel* InComboBoxVM, FName InSelectedKey)
{
	if (InComboBoxVM == nullptr)
	{
		return;
	}

	if (bIsSyncingComboBoxes == true)
	{
		return;
	}
	TGuardValue<bool> ComboSyncGuard(bIsSyncingComboBoxes, true);
	InComboBoxVM->SetSelectedKey(InSelectedKey);
}

void UBBMSettingsViewModel::HandleLanguageComboBoxSelectedKeyChanged(FName InSelectedKey)
{
	if (InSelectedKey.IsNone() == true)
	{
		return;
	}

	if (bIsSyncingComboBoxes == true)
	{
		return;
	}
	TGuardValue<bool> ComboSyncGuard(bIsSyncingComboBoxes, true);
	SetLanguageKey(InSelectedKey);
}

void UBBMSettingsViewModel::HandleWindowModeComboBoxSelectedKeyChanged(FName InSelectedKey)
{
	if (InSelectedKey.IsNone() == true)
	{
		return;
	}

	if (bIsSyncingComboBoxes == true)
	{
		return;
	}
	TGuardValue<bool> ComboSyncGuard(bIsSyncingComboBoxes, true);
	SetWindowModeKey(InSelectedKey);
}

void UBBMSettingsViewModel::HandleScreenResolutionComboBoxSelectedKeyChanged(FName InSelectedKey)
{
	if (InSelectedKey.IsNone() == true)
	{
		return;
	}

	if (bIsSyncingComboBoxes == true)
	{
		return;
	}
	TGuardValue<bool> ComboSyncGuard(bIsSyncingComboBoxes, true);
	SetScreenResolutionKey(InSelectedKey);
}
