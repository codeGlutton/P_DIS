/*****************************************************************//**
 * @file   BBMTitleViewModel.cpp
 * @brief  BBM(비트 배낭 상인) 타이틀 화면 전용 MVVM 뷰모델 클래스 구현
 * @date   2026-11-07
 *********************************************************************/

#include "UI/ViewModel/BBMTitleViewModel.h"
#include "Save/BBMGameDataSubsystem.h"
#include "Save/BBMRunSaveGame.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"

#include "UI/ViewModel/BBMSettingsViewModel.h"
#include "UI/ViewModel/BBMCharacterSelectViewModel.h"
#include "UI/ViewModel/BBMCreditsViewModel.h"

UBBMTitleViewModel::UBBMTitleViewModel()
{
	ShiningTransition = CreateDefaultSubobject<UBBMSingleAnimTransition>(TEXT("ShiningTransition"));
	BlockingTransition = CreateDefaultSubobject<UBBMDualAnimTransition>(TEXT("BlockingTransition"));
}

void UBBMTitleViewModel::BeginViewModel()
{
	Super::BeginViewModel();

	// 필수 서브 뷰모델 3종이 사전 할당되지 않은 경우 에러 로그를 발생시키고 초기 바인딩을 차단
	if (SettingsViewModel == nullptr || CharacterSelectViewModel == nullptr || CreditsViewModel == nullptr)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[UBBMTitleViewModel::BeginViewModel] 필수 서브 뷰모델이 할당되지 않음 (Settings=%s, CharacterSelect=%s, Credits=%s)."),
			SettingsViewModel != nullptr ? TEXT("Valid") : TEXT("Null"),
			CharacterSelectViewModel != nullptr ? TEXT("Valid") : TEXT("Null"),
			CreditsViewModel != nullptr ? TEXT("Valid") : TEXT("Null")
		);
		return;
	}

	if (BlockingTransition != nullptr)
	{
		BlockingStartingHandle = BlockingTransition->OnStartingNative.AddUObject(
			this,
			&UBBMTitleViewModel::HandleBlockingTransitionStarting
		);
		BlockingEndedHandle = BlockingTransition->OnEndedNative.AddUObject(
			this,
			&UBBMTitleViewModel::HandleBlockingTransitionEnded
		);
	}

	BindSubViewModelOpenClose(SettingsViewModel);
	BindSubViewModelOpenClose(CharacterSelectViewModel);
	BindSubViewModelOpenClose(CreditsViewModel);

	UWorld* World = GetWorld();
	UGameInstance* GI = (World != nullptr) ? World->GetGameInstance() : nullptr;
	UBBMGameDataSubsystem* GameDataSS = (GI != nullptr) ? GI->GetSubsystem<UBBMGameDataSubsystem>() : nullptr;
	if (GameDataSS == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMTitleViewModel::BeginViewModel] UBBMGameDataSubsystem을 조회할 수 없어 세이브 시작 대리자 바인딩을 생략함"));
		return;
	}

	SaveStartedHandle = GameDataSS->OnSaveStartedNative.AddUObject(
		this,
		&UBBMTitleViewModel::HandleSaveStarted
	);
}

void UBBMTitleViewModel::EndViewModel()
{
	UnbindSubViewModelOpenClose();
	OpenSubPanelCount = 0;

	if (BlockingTransition != nullptr)
	{
		if (BlockingStartingHandle.IsValid() == true)
		{
			BlockingTransition->OnStartingNative.Remove(BlockingStartingHandle);
		}
		if (BlockingEndedHandle.IsValid() == true)
		{
			BlockingTransition->OnEndedNative.Remove(BlockingEndedHandle);
		}
	}
	BlockingStartingHandle.Reset();
	BlockingEndedHandle.Reset();

	UWorld* World = GetWorld();
	UGameInstance* GI = (World != nullptr) ? World->GetGameInstance() : nullptr;
	UBBMGameDataSubsystem* GameDataSS = (GI != nullptr) ? GI->GetSubsystem<UBBMGameDataSubsystem>() : nullptr;
	if (GameDataSS != nullptr && SaveStartedHandle.IsValid() == true)
	{
		GameDataSS->OnSaveStartedNative.Remove(SaveStartedHandle);
	}
	SaveStartedHandle.Reset();

	Super::EndViewModel();
}

void UBBMTitleViewModel::OnOpen()
{
	Super::OnOpen();

	// 타이틀 뷰모델이 열리는 시점에 최신 세이브 런 상태 동기화 및 반짝임 루프 애니메이션 시작
	RefreshTitleState();
}

void UBBMTitleViewModel::OnFinishOpenAnim(bool bInterrupted)
{
	Super::OnFinishOpenAnim(bInterrupted);

	if (bInterrupted == false && ShiningTransition != nullptr)
	{
		ShiningTransition->Execute(true);
	}
}

void UBBMTitleViewModel::OnClosed()
{
	Super::OnClosed();

	// 타이틀이 완전히 닫히면 무한 반복 중인 반짝임 애니메이션 및 블로킹 상태를 즉시 정리
	if (ShiningTransition != nullptr)
	{
		ShiningTransition->Execute(false);
	}
}

void UBBMTitleViewModel::SetCanContinue(bool bInCanContinue)
{
	UE_MVVM_SET_PROPERTY_VALUE(bCanContinue, bInCanContinue);
}

void UBBMTitleViewModel::SetActiveRunEncounterIndex(int32 InEncounterIndex)
{
	UE_MVVM_SET_PROPERTY_VALUE(ActiveRunEncounterIndex, InEncounterIndex);
}

void UBBMTitleViewModel::SetBlockingPanelVisibility(ESlateVisibility InVisibility)
{
	UE_MVVM_SET_PROPERTY_VALUE(BlockingPanelVisibility, InVisibility);
}

void UBBMTitleViewModel::SetSettingsViewModel(UBBMSettingsViewModel* InSettingsViewModel)
{
	UE_MVVM_SET_PROPERTY_VALUE(SettingsViewModel, InSettingsViewModel);
}

void UBBMTitleViewModel::SetCharacterSelectViewModel(UBBMCharacterSelectViewModel* InCharacterSelectViewModel)
{
	UE_MVVM_SET_PROPERTY_VALUE(CharacterSelectViewModel, InCharacterSelectViewModel);
}

void UBBMTitleViewModel::SetCreditsViewModel(UBBMCreditsViewModel* InCreditsViewModel)
{
	UE_MVVM_SET_PROPERTY_VALUE(CreditsViewModel, InCreditsViewModel);
}

void UBBMTitleViewModel::RefreshTitleState()
{
	UWorld* World = GetWorld();
	UGameInstance* GI = World != nullptr ? World->GetGameInstance() : nullptr;
	UBBMGameDataSubsystem* GameDataSS = GI != nullptr ? GI->GetSubsystem<UBBMGameDataSubsystem>() : nullptr;
	if (GameDataSS == nullptr)
	{
		SetCanContinue(false);
		SetActiveRunEncounterIndex(0);
		return;
	}

	const bool bHasRun = GameDataSS->HasActiveRun();
	SetCanContinue(bHasRun);

	const UBBMRunSaveGame* RunData = (bHasRun == true) ? GameDataSS->GetCurrentRunData() : nullptr;
	SetActiveRunEncounterIndex(RunData != nullptr ? RunData->CurrentEncounterIndex : 0);
}

void UBBMTitleViewModel::HandleSaveStarted()
{
	RefreshTitleState();
}

void UBBMTitleViewModel::HandleBlockingTransitionStarting(bool /*bWithAnim*/)
{
	SetBlockingPanelVisibility(ESlateVisibility::Visible);
}

void UBBMTitleViewModel::HandleBlockingTransitionEnded(bool /*bInterrupted*/)
{
	SetBlockingPanelVisibility(ESlateVisibility::Collapsed);
}

void UBBMTitleViewModel::HandleSubPanelStarting(bool /*bWithAnim*/)
{
	++OpenSubPanelCount;
	if (OpenSubPanelCount == 1 && BlockingTransition != nullptr)
	{
		BlockingTransition->StartWithAnim();
	}
}

void UBBMTitleViewModel::HandleSubPanelEnded(bool /*bInterrupted*/)
{
	if (OpenSubPanelCount <= 0)
	{
		return;
	}

	--OpenSubPanelCount;
	if (OpenSubPanelCount == 0 && BlockingTransition != nullptr)
	{
		BlockingTransition->EndWithAnim();
	}
}

void UBBMTitleViewModel::BindSubViewModelOpenClose(UBBMViewModelBase* InSubViewModel)
{
	if (InSubViewModel == nullptr)
	{
		return;
	}

	UBBMDualAnimTransition* OpenClose = InSubViewModel->GetOpenCloseTransition();
	if (OpenClose == nullptr)
	{
		return;
	}

	FSubPanelHookBinding Binding;
	Binding.Transition = OpenClose;
	Binding.StartingHandle = OpenClose->OnStartingNative.AddUObject(
		this,
		&UBBMTitleViewModel::HandleSubPanelStarting
	);
	Binding.EndedHandle = OpenClose->OnEndedNative.AddUObject(
		this,
		&UBBMTitleViewModel::HandleSubPanelEnded
	);
	SubPanelHookBindings.Add(MoveTemp(Binding));
}

void UBBMTitleViewModel::UnbindSubViewModelOpenClose()
{
	for (FSubPanelHookBinding& Binding : SubPanelHookBindings)
	{
		if (Binding.Transition.IsValid() == false)
		{
			continue;
		}

		UBBMDualAnimTransition* Transition = Binding.Transition.Get();
		if (Binding.StartingHandle.IsValid() == true)
		{
			Transition->OnStartingNative.Remove(Binding.StartingHandle);
		}
		if (Binding.EndedHandle.IsValid() == true)
		{
			Transition->OnEndedNative.Remove(Binding.EndedHandle);
		}
	}

	SubPanelHookBindings.Reset();
}

void UBBMTitleViewModel::ContinueGame()
{
	if (bCanContinue == true)
	{
	}
}

void UBBMTitleViewModel::OpenSettings()
{
	if (SettingsViewModel != nullptr)
	{
		SettingsViewModel->OpenWithAnim();
	}
}

void UBBMTitleViewModel::OpenCredits()
{
	if (CreditsViewModel != nullptr)
	{
		CreditsViewModel->OpenWithAnim();
	}
}

void UBBMTitleViewModel::OpenCharacterSelections()
{
	if (CharacterSelectViewModel != nullptr)
	{
		CharacterSelectViewModel->OpenWithAnim();
	}
}

void UBBMTitleViewModel::QuitGame()
{
	if (UWorld* World = GetWorld())
	{
		UKismetSystemLibrary::QuitGame(World, nullptr, EQuitPreference::Quit, false);
	}
}

