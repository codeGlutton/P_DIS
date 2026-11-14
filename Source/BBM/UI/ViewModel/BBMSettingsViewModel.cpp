#include "UI/ViewModel/BBMSettingsViewModel.h"
#include "Save/BBMGameDataSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UBBMSettingsViewModel::BeginViewModel()
{
	Super::BeginViewModel();

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

	RefreshHasPendingEdit();
}

void UBBMSettingsViewModel::RefreshHasPendingEdit()
{
	const bool bInHasPendingEdit = (CachedCurrentSettings != CachedSettingsBeforeEdit);
	UE_MVVM_SET_PROPERTY_VALUE(bHasPendingEdit, bInHasPendingEdit);
}
