/*****************************************************************//**
 * @file   BBMPlayerController.cpp
 * @brief  BBM(비트 배낭 상인) 기본 플레이어 컨트롤러 클래스 구현
 * @date   2026-11-06
 *********************************************************************/

#include "Level/BBMPlayerController.h"
#include "Combat/Pawn/BBMPlayerCharacterDefinition.h"
#include "Combat/Pawn/BBMPlayerMerchantBase.h"
#include "Combat/Pawn/BBMCombatantBase.h"
#include "Combat/BBMCombatManagerSubsystem.h"
#include "Combat/BBMCombatTypes.h"
#include "Combat/GAS/BBMAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Stage/BBMStageSubsystem.h"
#include "Stage/BBMRegionDefinition.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"

ABBMPlayerController::ABBMPlayerController() :
	DefaultMerchantDefinition(FSoftObjectPath(TEXT("/BBMGameFeatureCore/Combat/Player/DA_Player_DefaultMerchant.DA_Player_DefaultMerchant"))),
	InGameLevelName(FName(TEXT("L_Test"))),
	TitleLevelName(FName(TEXT("L_Title")))
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ABBMPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ABBMPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

#if WITH_EDITOR || !UE_BUILD_SHIPPING
	if (InputComponent != nullptr)
	{
		InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ABBMPlayerController::OnF1Pressed);
		InputComponent->BindKey(EKeys::F2, IE_Pressed, this, &ABBMPlayerController::OnF2Pressed);
		InputComponent->BindKey(EKeys::F3, IE_Pressed, this, &ABBMPlayerController::OnF3Pressed);
		InputComponent->BindKey(EKeys::F4, IE_Pressed, this, &ABBMPlayerController::OnF4Pressed);
		InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &ABBMPlayerController::OnF5Pressed);
	}
#endif
}

#if WITH_EDITOR || !UE_BUILD_SHIPPING
bool ABBMPlayerController::ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor)
{
	if (FParse::Command(&Cmd, TEXT("bbm.StartRun")))
	{
		StartRun();
		return true;
	}
	if (FParse::Command(&Cmd, TEXT("bbm.Next")))
	{
		NextEncounter();
		return true;
	}
	if (FParse::Command(&Cmd, TEXT("bbm.SimulateTurn")))
	{
		SimulateCombatTurn();
		return true;
	}
	if (FParse::Command(&Cmd, TEXT("bbm.Win")))
	{
		ForceWinCombat();
		return true;
	}
	if (FParse::Command(&Cmd, TEXT("bbm.SelectRegion")))
	{
		int32 RegionIndex = 0;
		FParse::Value(Cmd, TEXT(""), RegionIndex);
		SelectNextRegionIndex(RegionIndex);
		return true;
	}

	return Super::ProcessConsoleExec(Cmd, Ar, Executor);
}

void ABBMPlayerController::StartRun()
{
	UGameInstance* GI = GetGameInstance();
	if (GI == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABBMPlayerController::StartRun] GameInstance is null."));
		return;
	}

	UBBMStageSubsystem* StageSS = GI->GetSubsystem<UBBMStageSubsystem>();
	if (StageSS == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABBMPlayerController::StartRun] UBBMStageSubsystem is null."));
		return;
	}

	const UBBMPlayerCharacterDefinition* PlayerDef = DefaultMerchantDefinition.LoadSynchronous();
	if (PlayerDef == nullptr)
	{
		PlayerDef = LoadObject<UBBMPlayerCharacterDefinition>(nullptr, TEXT("/BBMGameFeatureCore/Combat/Player/DA_Player_DefaultMerchant.DA_Player_DefaultMerchant"));
	}

	if (PlayerDef == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[ABBMPlayerController::StartRun] Failed to load DefaultMerchantDefinition."));
		return;
	}

	const bool bStarted = StageSS->StartNewRun(PlayerDef);
	if (bStarted == true)
	{
		UE_LOG(LogTemp, Log, TEXT("[ABBMPlayerController::StartRun] Run started successfully! Opening level: %s"), *InGameLevelName.ToString());
		UGameplayStatics::OpenLevel(this, InGameLevelName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABBMPlayerController::StartRun] StartNewRun returned false."));
	}
}

void ABBMPlayerController::NextEncounter()
{
	UGameInstance* GI = GetGameInstance();
	if (GI == nullptr)
	{
		return;
	}

	UBBMStageSubsystem* StageSS = GI->GetSubsystem<UBBMStageSubsystem>();
	if (StageSS == nullptr)
	{
		return;
	}

	ABBMPlayerMerchantBase* Merchant = FindPlayerMerchant();
	StageSS->AdvanceToNextEncounter(Merchant);
	UE_LOG(LogTemp, Log, TEXT("[ABBMPlayerController::NextEncounter] AdvanceToNextEncounter executed. Current Encounter: %d"), StageSS->GetCurrentEncounterIndex());
}

void ABBMPlayerController::SimulateCombatTurn()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	UBBMCombatManagerSubsystem* CombatMgr = World->GetSubsystem<UBBMCombatManagerSubsystem>();
	if (CombatMgr == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABBMPlayerController::SimulateCombatTurn] CombatManagerSubsystem is null."));
		return;
	}

	TArray<FBBMCombatStepVisualEvent> OutVisualEvents;
	const bool bResult = CombatMgr->SimulateTurn(OutVisualEvents);
	UE_LOG(LogTemp, Log, TEXT("[ABBMPlayerController::SimulateCombatTurn] SimulateTurn Result=%s, VisualEvents Count=%d"),
		bResult ? TEXT("True") : TEXT("False"), OutVisualEvents.Num());
}

void ABBMPlayerController::ForceWinCombat()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	UBBMCombatManagerSubsystem* CombatMgr = World->GetSubsystem<UBBMCombatManagerSubsystem>();
	if (CombatMgr == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABBMPlayerController::ForceWinCombat] CombatManagerSubsystem is null."));
		return;
	}

	TArray<ABBMCombatantBase*> Enemies = CombatMgr->GetLivingCombatantsInLane(EBBMCombatLane::EnemyLane);
	for (ABBMCombatantBase* Enemy : Enemies)
	{
		if (Enemy != nullptr && Enemy->GetAbilitySystemComponent() != nullptr)
		{
			Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(UBBMAttributeSet::GetHealthAttribute(), 0.0f);
		}
	}

	TArray<FBBMCombatStepVisualEvent> OutVisualEvents;
	CombatMgr->SimulateTurn(OutVisualEvents);
	UE_LOG(LogTemp, Log, TEXT("[ABBMPlayerController::ForceWinCombat] Set %d enemies health to 0 and triggered SimulateTurn."), Enemies.Num());
}

void ABBMPlayerController::SelectNextRegionIndex(int32 RegionIndex)
{
	UGameInstance* GI = GetGameInstance();
	if (GI == nullptr)
	{
		return;
	}

	UBBMStageSubsystem* StageSS = GI->GetSubsystem<UBBMStageSubsystem>();
	if (StageSS == nullptr)
	{
		return;
	}

	TArray<UBBMRegionDefinition*> Options = StageSS->GetNextRegionOptions();
	if (Options.IsValidIndex(RegionIndex) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABBMPlayerController::SelectNextRegionIndex] Invalid Index %d (Options Count=%d)"), RegionIndex, Options.Num());
		return;
	}

	ABBMPlayerMerchantBase* Merchant = FindPlayerMerchant();
	const bool bSuccess = StageSS->SelectNextRegion(Options[RegionIndex], Merchant);
	UE_LOG(LogTemp, Log, TEXT("[ABBMPlayerController::SelectNextRegionIndex] Selected Option %d (%s), Result=%s"),
		RegionIndex, *Options[RegionIndex]->GetName(), bSuccess ? TEXT("Success") : TEXT("Failed"));
}

void ABBMPlayerController::OnF1Pressed()
{
	StartRun();
}

void ABBMPlayerController::OnF2Pressed()
{
	NextEncounter();
}

void ABBMPlayerController::OnF3Pressed()
{
	SimulateCombatTurn();
}

void ABBMPlayerController::OnF4Pressed()
{
	ForceWinCombat();
}

void ABBMPlayerController::OnF5Pressed()
{
	SelectNextRegionIndex(0);
}
#endif

ABBMPlayerMerchantBase* ABBMPlayerController::FindPlayerMerchant() const
{
	if (ABBMPlayerMerchantBase* AsMerchant = Cast<ABBMPlayerMerchantBase>(GetPawn()))
	{
		return AsMerchant;
	}

	if (UWorld* World = GetWorld())
	{
		return Cast<ABBMPlayerMerchantBase>(UGameplayStatics::GetActorOfClass(World, ABBMPlayerMerchantBase::StaticClass()));
	}

	return nullptr;
}
