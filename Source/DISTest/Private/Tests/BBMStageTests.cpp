/*****************************************************************//**
 * @file   BBMStageTests.cpp
 * @brief  BBM(비트 배낭 상인) 레벨 스폰 세팅, 지역·만남 진행 및 GameData 영속 저장 단위 테스트
 * @details ABBMWorldSettings 스폰 조회, UBBMRegionDefinition 5회 만남 생성, UBBMGameDataSubsystem 비동기 직렬화, UBBMStageSubsystem 5회 만남 순회 및 분기 전환 검증
 * @date   2026-11-03
 *********************************************************************/

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Level/BBMWorldSettings.h"
#include "Level/BBMSpawnTypes.h"
#include "Stage/BBMStageTypes.h"
#include "Stage/BBMEncounterDefinition.h"
#include "Stage/BBMChestActorBase.h"
#include "Stage/BBMEventNpcBase.h"
#include "Stage/BBMRegionDefinition.h"
#include "Stage/BBMStageSubsystem.h"
#include "Global/BBMGlobalLogic.h"
#include "Item/BBMItemProfileTypes.h"
#include "Save/BBMGameDataSubsystem.h"
#include "Save/BBMUserSaveGame.h"
#include "Save/BBMRunSaveGame.h"
#include "Combat/Pawn/BBMPlayerCharacterDefinition.h"
#include "Combat/Pawn/BBMPlayerMerchantBase.h"
#include "Combat/Pawn/BBMAICombatantDefinition.h"
#include "Combat/GAS/BBMAttributeSet.h"
#include "Inventory/BBMInventoryDefinition.h"
#include "Inventory/BBMInventoryComponent.h"
#include "Inventory/BBMInventory.h"
#include "Item/BBMItemDefinition.h"
#include "Item/BBMItemEntry.h"
#include "Item/DISItemStack.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/SubsystemCollection.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * 테스트 월드 생명주기 관리 스코프 구조체
 */
struct FBBMStageScopedTestWorld
{
	UWorld* World = nullptr;

	FBBMStageScopedTestWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("BBMStageTestWorld"));
		if (World != nullptr)
		{
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
		}
	}

	~FBBMStageScopedTestWorld()
	{
		if (World != nullptr)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			World = nullptr;
		}
	}
};

// 1. ABBMWorldSettings 스폰 및 카메라 포인트 등록/조회 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMWorldSettingsSpawnTest,
	"DIS.BBM.Stage.WorldSettingsSpawnQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMWorldSettingsSpawnTest::RunTest(const FString& Parameters)
{
	FBBMStageScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 1. ABBMWorldSettings 생성 및 테스트 스폰 포인트 액터 스폰
	ABBMWorldSettings* WorldSettings = NewObject<ABBMWorldSettings>(TestWorld.World);
	TestNotNull(TEXT("WorldSettings 생성 검증"), WorldSettings);
	if (WorldSettings == nullptr)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* CamA = TestWorld.World->SpawnActor<AActor>(AActor::StaticClass(), FVector(100, 0, 0), FRotator::ZeroRotator, SpawnParams);
	AActor* SpawnA = TestWorld.World->SpawnActor<AActor>(AActor::StaticClass(), FVector(100, 200, 0), FRotator::ZeroRotator, SpawnParams);

	AActor* CamB = TestWorld.World->SpawnActor<AActor>(AActor::StaticClass(), FVector(500, 0, 0), FRotator::ZeroRotator, SpawnParams);
	AActor* SpawnB = TestWorld.World->SpawnActor<AActor>(AActor::StaticClass(), FVector(500, 200, 0), FRotator::ZeroRotator, SpawnParams);

	TestNotNull(TEXT("CamA 유효 검증"), CamA);
	TestNotNull(TEXT("SpawnA 유효 검증"), SpawnA);

	// 2. 스폰 설정 등록
	const FName RoomKeyA = FName(TEXT("ForestRoomA"));
	const FName RoomKeyB = FName(TEXT("ForestRoomB"));

	WorldSettings->SetSpawnSetting(RoomKeyA, FBBMRoomSpawnSetting(CamA, SpawnA));
	WorldSettings->SetSpawnSetting(RoomKeyB, FBBMRoomSpawnSetting(CamB, SpawnB));

	// 3. 존재 확인 및 액터 조회 검증
	TestTrue(TEXT("RoomKeyA 존재 검증"), WorldSettings->HasSpawnSetting(RoomKeyA));
	TestTrue(TEXT("RoomKeyB 존재 검증"), WorldSettings->HasSpawnSetting(RoomKeyB));
	TestFalse(TEXT("미등록 키는 존재하지 않아야 함"), WorldSettings->HasSpawnSetting(FName(TEXT("NonExistentRoom"))));

	TestEqual(TEXT("RoomKeyA 메인 카메라 액터 일치 검증"), WorldSettings->GetMainCameraPoint(RoomKeyA), CamA);
	TestEqual(TEXT("RoomKeyA 시작 포인트 액터 일치 검증"), WorldSettings->GetRoomStartPoint(RoomKeyA), SpawnA);
	TestEqual(TEXT("RoomKeyB 메인 카메라 액터 일치 검증"), WorldSettings->GetMainCameraPoint(RoomKeyB), CamB);
	TestEqual(TEXT("RoomKeyB 시작 포인트 액터 일치 검증"), WorldSettings->GetRoomStartPoint(RoomKeyB), SpawnB);

	// 4. 전체 키 목록 및 무작위 키 선택 검증
	TArray<FName> AllKeys = WorldSettings->GetAllSpawnSettingNames();
	TestEqual(TEXT("등록된 스폰 세팅 키 수량은 2개여야 함"), AllKeys.Num(), 2);

	FRandomStream FixedStream(12345);
	const FName PickedKey = WorldSettings->GetRandomRoomSpawnSettingName(FixedStream);
	TestTrue(TEXT("랜덤 선택된 키는 등록된 키 중 하나여야 함"), PickedKey == RoomKeyA || PickedKey == RoomKeyB);

	return true;
}

// 2. UBBMRegionDefinition 5회 만남 시퀀스 생성 및 분기 선택지 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMRegionAndEncounterTest,
	"DIS.BBM.Stage.RegionAndEncounterGeneration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMRegionAndEncounterTest::RunTest(const FString& Parameters)
{
	// 1. 만남 정적 정의 에셋 파생 3종 구성 (전투, 이벤트, 보물)
	UBBMCombatEncounterDefinition* CombatEnc = NewObject<UBBMCombatEncounterDefinition>();
	CombatEnc->DisplayName = FText::FromString(TEXT("일반 몬스터 전투"));
	CombatEnc->SpawnSettingKey = FName(TEXT("CombatRoom"));

	UBBMEventEncounterDefinition* EventEnc = NewObject<UBBMEventEncounterDefinition>();
	EventEnc->DisplayName = FText::FromString(TEXT("모험가 조우"));
	EventEnc->SpawnSettingKey = FName(TEXT("EventRoom"));
	EventEnc->NpcClasses.Add(ABBMEventNpcBase::StaticClass());

	UBBMTreasureEncounterDefinition* TreasureEnc = NewObject<UBBMTreasureEncounterDefinition>();
	TreasureEnc->DisplayName = FText::FromString(TEXT("보물 상자"));
	TreasureEnc->SpawnSettingKey = FName(TEXT("TreasureRoom"));
	TreasureEnc->ChestClass = ABBMChestActorBase::StaticClass();

	// 확정 보스 전투 만남 (BossCombat 타입)
	UBBMCombatEncounterDefinition* BossEnc = NewObject<UBBMCombatEncounterDefinition>();
	BossEnc->EncounterType = EBBMEncounterType::BossCombat;
	BossEnc->DisplayName = FText::FromString(TEXT("지역 보스"));
	BossEnc->SpawnSettingKey = FName(TEXT("BossRoom"));

	// 파생 클래스 고유 속성 검증
	TestEqual(TEXT("보물 상자 액터 클래스 일치 검증"), TreasureEnc->GetChestClass().Get(), ABBMChestActorBase::StaticClass());
	TestEqual(TEXT("이벤트 NPC 클래스 수량 일치 검증"), EventEnc->GetNpcClasses().Num(), 1);

	// 2. 지역 정의 에셋 구성 (타입별 분리 풀 및 보스 풀)
	UBBMRegionDefinition* TestRegion = NewObject<UBBMRegionDefinition>();
	TestRegion->RegionName = FText::FromString(TEXT("초심자의 숲"));
	TestRegion->CombatEncountersPool.Add(CombatEnc);
	TestRegion->EventEncountersPool.Add(EventEnc);
	TestRegion->TreasureEncountersPool.Add(TreasureEnc);
	TestRegion->BossEncountersPool.Add(BossEnc);

	// 다음 분기 지역 3종 등록 (소프트 레퍼런스 방식)
	UBBMRegionDefinition* NextA = NewObject<UBBMRegionDefinition>();
	NextA->RegionName = FText::FromString(TEXT("수정 동굴"));
	UBBMRegionDefinition* NextB = NewObject<UBBMRegionDefinition>();
	NextB->RegionName = FText::FromString(TEXT("어둠의 늪지"));
	UBBMRegionDefinition* NextC = NewObject<UBBMRegionDefinition>();
	NextC->RegionName = FText::FromString(TEXT("폐허 도시"));

	TestRegion->NextPossibleRegions.Add(NextA);
	TestRegion->NextPossibleRegions.Add(NextB);
	TestRegion->NextPossibleRegions.Add(NextC);

	// 3. 5회 만남 시퀀스 생성 검증
	FRandomStream Stream(42);
	TArray<UBBMEncounterDefinition*> GeneratedEncounters = TestRegion->GenerateFiveEncounters(Stream);

	TestEqual(TEXT("생성된 만남 시퀀스는 정확히 5개여야 함"), GeneratedEncounters.Num(), 5);
	if (GeneratedEncounters.Num() == 5)
	{
		// 1~4회차는 풀 내에 존재하는 일반 만남이어야 함
		for (int32 i = 0; i < 4; ++i)
		{
			TestTrue(TEXT("1~4회차 만남은 일반 풀 내 에셋이어야 함"),
				GeneratedEncounters[i] == CombatEnc ||
				GeneratedEncounters[i] == EventEnc ||
				GeneratedEncounters[i] == TreasureEnc);
		}

		// 5회차는 보스 만남이어야 함 (GDD 포켓로그식 보스 확정 등장)
		TestTrue(TEXT("5회차 만남은 설정된 BossEnc여야 함"), GeneratedEncounters[4] == BossEnc);
		TestEqual(TEXT("5회차 만남 유형은 BossCombat이어야 함"), GeneratedEncounters[4]->GetEncounterType(), EBBMEncounterType::BossCombat);
	}

	// 4. 다음 지역 분기 선택지 목록 검증 (최대 2개 요청)
	TArray<UBBMRegionDefinition*> Options = TestRegion->GetNextRegionOptions(2, Stream);
	TestEqual(TEXT("요청한 최대 2개의 선택지가 반환되어야 함"), Options.Num(), 2);
	TestTrue(TEXT("선택지는 유효해야 함"), Options[0] != nullptr && Options[1] != nullptr);
	TestTrue(TEXT("선택지는 서로 다른 지역이어야 함"), Options[0] != Options[1]);

	return true;
}

// 3. UBBMPlayerCharacterDefinition 시작 지역(StartRegion) 연동 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMPlayerStartRegionTest,
	"DIS.BBM.Stage.PlayerCharacterStartRegion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMPlayerStartRegionTest::RunTest(const FString& Parameters)
{
	UBBMRegionDefinition* MerchantTown = NewObject<UBBMRegionDefinition>();
	MerchantTown->RegionName = FText::FromString(TEXT("상인의 고향 마을"));

	UBBMRegionDefinition* WarriorVillage = NewObject<UBBMRegionDefinition>();
	WarriorVillage->RegionName = FText::FromString(TEXT("전사의 요새"));

	// 캐릭터 A: 상인
	UBBMPlayerCharacterDefinition* PlayerMerchantDef = NewObject<UBBMPlayerCharacterDefinition>();
	PlayerMerchantDef->StartRegionDefinition = MerchantTown;

	// 캐릭터 B: 전사 상인
	UBBMPlayerCharacterDefinition* PlayerWarriorDef = NewObject<UBBMPlayerCharacterDefinition>();
	PlayerWarriorDef->StartRegionDefinition = WarriorVillage;

	TestTrue(TEXT("캐릭터 A 시작 지역 일치 검증"), PlayerMerchantDef->GetStartRegionDefinition() == MerchantTown);
	TestTrue(TEXT("캐릭터 B 시작 지역 일치 검증"), PlayerWarriorDef->GetStartRegionDefinition() == WarriorVillage);
	TestTrue(TEXT("캐릭터 종류마다 시작 지역이 달라야 함"), PlayerMerchantDef->GetStartRegionDefinition() != PlayerWarriorDef->GetStartRegionDefinition());

	return true;
}

// 4. UBBMGameDataSubsystem 런타임 데이터 추출 및 직렬화 무결성 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMGameDataExtractionTest,
	"DIS.BBM.Stage.GameDataExtractionAndSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMGameDataExtractionTest::RunTest(const FString& Parameters)
{
	FBBMStageScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 1. 플레이어 상인 스폰 및 인벤토리 구성
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABBMPlayerMerchantBase* Merchant = TestWorld.World->SpawnActor<ABBMPlayerMerchantBase>(
		ABBMPlayerMerchantBase::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	TestNotNull(TEXT("Merchant 스폰 검증"), Merchant);
	if (Merchant == nullptr)
	{
		return false;
	}

	UBBMInventoryDefinition* InvenDef = NewObject<UBBMInventoryDefinition>();
	InvenDef->DefaultCapacity = 8;
	InvenDef->LineBreakTileCount = 4;

	UBBMPlayerCharacterDefinition* PlayerDef = NewObject<UBBMPlayerCharacterDefinition>();
	PlayerDef->InventoryDefinition = InvenDef;
	PlayerDef->PawnClass = ABBMPlayerMerchantBase::StaticClass();
	Merchant->SetCharacterDefinition(PlayerDef);

	UBBMInventory* Inventory = Merchant->GetInventory();
	TestNotNull(TEXT("Inventory 유효 검증"), Inventory);
	if (Inventory == nullptr)
	{
		return false;
	}

	// 1x1 아이템 배치
	UBBMItemDefinition* ItemDef = NewObject<UBBMItemDefinition>();
	TArray<FIntPoint> ShapePoints;
	ShapePoints.Add(FIntPoint(0, 0));
	ItemDef->Shape = FDISTileShape(ShapePoints);

	UBBMItemEntry* Entry = NewObject<UBBMItemEntry>();
	Entry->InitializeEntry(ItemDef);
	UDISItemStack* Stack = NewObject<UDISItemStack>();
	Stack->PushEntry(Entry);

	Inventory->SetSlotItemStack(2, Stack);

	// 2. UBBMGameDataSubsystem 인스턴스 생성 및 데이터 추출 실행
	UGameInstance* GameInstanceA = NewObject<UGameInstance>();
	UBBMGameDataSubsystem* GameDataSubsystem = NewObject<UBBMGameDataSubsystem>(GameInstanceA);
	FSubsystemCollection<UGameInstanceSubsystem> SubsystemCollectionA;
	GameDataSubsystem->Initialize(SubsystemCollectionA);

	const FPrimaryAssetId RegionId(TEXT("BBMRegion"), TEXT("TestForest"));
	TArray<FPrimaryAssetId> PlannedEncounters;
	PlannedEncounters.Add(FPrimaryAssetId(TEXT("BBMEncounter"), TEXT("Enc1")));
	PlannedEncounters.Add(FPrimaryAssetId(TEXT("BBMEncounter"), TEXT("Enc2")));

	GameDataSubsystem->ExtractLevelData(
		Merchant,
		RegionId,
		2,
		PlannedEncounters,
		9999
	);

	// 3. 추출된 CurrentRunData 검증
	UBBMRunSaveGame* RunData = GameDataSubsystem->GetCurrentRunData();
	TestNotNull(TEXT("CurrentRunData 유효 검증"), RunData);
	if (RunData != nullptr)
	{
		TestTrue(TEXT("진행 중인 런으로 마킹되어야 함"), RunData->bHasActiveRun);
		TestEqual(TEXT("지역 식별자 일치 검증"), RunData->CurrentRegionId, RegionId);
		TestEqual(TEXT("다음 만남 인덱스 번호는 2여야 함"), RunData->CurrentEncounterIndex, 2);
		TestEqual(TEXT("계획된 만남 시퀀스 수량 일치"), RunData->PlannedEncounterIds.Num(), 2);
		TestEqual(TEXT("난수 시드 일치"), RunData->RngSeed, 9999);

		TestEqual(TEXT("저장된 인벤토리 슬롯 수량 검증"), RunData->SavedInventorySlots.Num(), 1);
		if (RunData->SavedInventorySlots.Num() > 0)
		{
			TestEqual(TEXT("슬롯 2에 아이템이 저장되어야 함"), RunData->SavedInventorySlots[0].SlotIndex, 2);
			TestEqual(TEXT("스택 수량은 1이어야 함"), RunData->SavedInventorySlots[0].StackCount, 1);
		}
	}

	// 4. 유저 세팅 데이터 검증
	UBBMUserSaveGame* UserData = GameDataSubsystem->GetUserSaveData();
	TestNotNull(TEXT("UserSaveData 유효 검증"), UserData);
	if (UserData != nullptr)
	{
		UserData->MasterVolume = 0.5f;
		TestEqual(TEXT("마스터 볼륨 설정 일치"), UserData->MasterVolume, 0.5f);
	}

	// 5. 런 데이터 삭제 검증
	GameDataSubsystem->ClearCurrentRunData();
	TestFalse(TEXT("클리어 후 bHasActiveRun은 false여야 함"), GameDataSubsystem->HasActiveRun());

	// 6. 안전 백업(.bak) 및 복구(.sav) 메커니즘 검증
	const FString TestSlot = TEXT("TestBBMBackupSlot");
	const FString SavPath = UBBMGameDataSubsystem::GetSaveFilePath(TestSlot);
	const FString BakPath = UBBMGameDataSubsystem::GetBackupFilePath(TestSlot);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// 사전 정리
	if (PlatformFile.FileExists(*SavPath)) { PlatformFile.DeleteFile(*SavPath); }
	if (PlatformFile.FileExists(*BakPath)) { PlatformFile.DeleteFile(*BakPath); }

	// 가상 .sav 파일 생성 (더미 세이브 데이터 저장)
	FFileHelper::SaveStringToFile(TEXT("OriginalSaveData"), *SavPath);
	TestTrue(TEXT(".sav 파일 생성 검증"), PlatformFile.FileExists(*SavPath));

	// 저장 전 백업 복제: 기존 파일이 있다면 .bak 파일로 복제
	const bool bBackupSuccess = UBBMGameDataSubsystem::PrepareBackupBeforeSave(TestSlot);
	TestTrue(TEXT("백업 복제 성공 검증"), bBackupSuccess);
	TestTrue(TEXT("기존 .sav는 그대로 보존되어야 함"), PlatformFile.FileExists(*SavPath));
	TestTrue(TEXT(".bak 백업 파일이 생성되어 있어야 함"), PlatformFile.FileExists(*BakPath));

	// 로드 시 복구 시뮬레이션: 비정상 종료 등으로 .sav 파일이 유실/손상된 상황 조성
	PlatformFile.DeleteFile(*SavPath);
	TestFalse(TEXT(".sav 파일 유실 상황 검증"), PlatformFile.FileExists(*SavPath));

	// 로드 시 복구 메커니즘: 기존 파일(.sav)이 없다면 .bak 파일을 복사 후 .sav로 복구
	const bool bRestoreSuccess = UBBMGameDataSubsystem::RestoreFromBackupIfNeeded(TestSlot);
	TestTrue(TEXT("백업 복구 성공 검증"), bRestoreSuccess);
	TestTrue(TEXT("복구 후 .sav 파일이 다시 생성되어 있어야 함"), PlatformFile.FileExists(*SavPath));
	TestTrue(TEXT("복사 복구이므로 .bak 파일도 보존되어야 함"), PlatformFile.FileExists(*BakPath));

	FString RestoredContent;
	FFileHelper::LoadFileToString(RestoredContent, *SavPath);
	TestEqual(TEXT("복구된 세이브 내용 무결성 검증"), RestoredContent, FString(TEXT("OriginalSaveData")));

	// 사후 정리
	PlatformFile.DeleteFile(*SavPath);
	PlatformFile.DeleteFile(*BakPath);

	return true;
}

// 5. UBBMStageSubsystem 5회 만남 순회 및 지역 완료 분기 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMFiveEncounterProgressionTest,
	"DIS.BBM.Stage.FiveEncounterProgression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMFiveEncounterProgressionTest::RunTest(const FString& Parameters)
{
	// 1. 만남 에셋 및 지역 에셋 구성
	UBBMCombatEncounterDefinition* EncCombat = NewObject<UBBMCombatEncounterDefinition>();
	EncCombat->EncounterType = EBBMEncounterType::Combat;

	UBBMCombatEncounterDefinition* EncBoss = NewObject<UBBMCombatEncounterDefinition>();
	EncBoss->EncounterType = EBBMEncounterType::BossCombat;

	UBBMRegionDefinition* RegionA = NewObject<UBBMRegionDefinition>();
	RegionA->RegionName = FText::FromString(TEXT("지역 1"));
	RegionA->CombatEncountersPool.Add(EncCombat);
	RegionA->BossEncountersPool.Add(EncBoss);

	UBBMRegionDefinition* RegionB = NewObject<UBBMRegionDefinition>();
	RegionB->RegionName = FText::FromString(TEXT("지역 2"));
	RegionB->CombatEncountersPool.Add(EncCombat);
	RegionB->BossEncountersPool.Add(EncBoss);

	RegionA->NextPossibleRegions.Add(RegionB);

	UBBMPlayerCharacterDefinition* PlayerDef = NewObject<UBBMPlayerCharacterDefinition>();
	PlayerDef->StartRegionDefinition = RegionA;

	// 2. UBBMStageSubsystem 인스턴스 생성 및 런 시작
	UGameInstance* GameInstanceB = NewObject<UGameInstance>();
	UBBMStageSubsystem* StageSubsystem = NewObject<UBBMStageSubsystem>(GameInstanceB);
	FSubsystemCollection<UGameInstanceSubsystem> SubsystemCollectionB;
	StageSubsystem->Initialize(SubsystemCollectionB);

	const bool bStartSuccess = StageSubsystem->StartNewRun(PlayerDef);
	TestTrue(TEXT("신규 런 시작 성공 검증"), bStartSuccess);
	TestTrue(TEXT("현재 지역은 RegionA여야 함"), StageSubsystem->GetCurrentRegion() == RegionA);
	TestEqual(TEXT("초기 만남 번호는 1이어야 함"), StageSubsystem->GetCurrentEncounterIndex(), 1);
	TestNotNull(TEXT("현재 만남 에셋 유효 검증"), StageSubsystem->GetCurrentEncounter());
	TestEqual(TEXT("계획된 만남 수량은 5개여야 함"), StageSubsystem->GetPlannedEncountersCount(), 5);

	// 3. 만남 순차 전진 (1 -> 2 -> 3 -> 4 -> 5)
	for (int32 Step = 1; Step <= 4; ++Step)
	{
		TestFalse(TEXT("5회 만남 전까지는 지역 미완료 상태여야 함"), StageSubsystem->IsCurrentRegionCompleted());
		StageSubsystem->AdvanceToNextEncounter(nullptr);
		TestEqual(TEXT("만남 번호 1씩 증가 검증"), StageSubsystem->GetCurrentEncounterIndex(), Step + 1);
	}

	TestEqual(TEXT("5회차 만남 도달 검증"), StageSubsystem->GetCurrentEncounterIndex(), 5);
	TestTrue(TEXT("5회차 만남은 보스여야 함"), StageSubsystem->GetCurrentEncounter() == EncBoss);
	TestFalse(TEXT("5회차 진행 중에는 여전히 지역 미완료"), StageSubsystem->IsCurrentRegionCompleted());

	// 4. 5회차 만남 완료 -> 지역 클리어 상태(CurrentEncounterIndex = 6)로 전환 검증
	// 5회차 완료 시에는 방 이동 트랜지션을 실행하지 않고 분기 선택 대기 상태로 전환됨
	StageSubsystem->AdvanceToNextEncounter(nullptr);

	TestEqual(TEXT("5회 완료 후 인덱스는 6이어야 함"), StageSubsystem->GetCurrentEncounterIndex(), 6);
	TestTrue(TEXT("5회 만남 완료 후 IsCurrentRegionCompleted는 true여야 함"), StageSubsystem->IsCurrentRegionCompleted());

	// 5. 다음 지역 분기 선택 및 진입 검증
	TArray<UBBMRegionDefinition*> NextOptions = StageSubsystem->GetNextRegionOptions(3);
	TestEqual(TEXT("다음 지역 후보 수량은 1개(RegionB)여야 함"), NextOptions.Num(), 1);
	TestTrue(TEXT("다음 지역 후보는 RegionB여야 함"), NextOptions[0] == RegionB);

	const bool bSelectRegionSuccess = StageSubsystem->SelectNextRegion(NextOptions[0], nullptr);
	TestTrue(TEXT("다음 지역 진입 성공 검증"), bSelectRegionSuccess);
	TestTrue(TEXT("현재 지역이 RegionB로 갱신되어야 함"), StageSubsystem->GetCurrentRegion() == RegionB);
	TestEqual(TEXT("새 지역 진입 시 만남 번호는 1로 리셋되어야 함"), StageSubsystem->GetCurrentEncounterIndex(), 1);
	TestFalse(TEXT("새 지역 진입 후에는 지역 미완료 상태여야 함"), StageSubsystem->IsCurrentRegionCompleted());
	TestEqual(TEXT("새 지역의 5회 만남 시퀀스도 5개로 재생성되어야 함"), StageSubsystem->GetPlannedEncountersCount(), 5);

	return true;
}

// 6. 다형성 인카운터 런타임 데이터 및 스테이지 아이템/NPC 추첨 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMEncounterRuntimeDataTest,
	"DIS.BBM.Stage.EncounterRuntimeData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMEncounterRuntimeDataTest::RunTest(const FString& Parameters)
{
	// 1. UBBMGlobalLogic 리프 태그 필터링 및 기본 희귀도 가중치 검증
	UBBMGlobalLogic* GlobalLogic = NewObject<UBBMGlobalLogic>();
	FDISItemTypeProfile WeaponParentProfile;
	FDISItemTypeProfile SwordLeafProfile;
	FDISItemTypeProfile ShieldLeafProfile;

	GlobalLogic->RegisterItemTypeProfile(BBMItemTypeTags::Item_Weapon, WeaponParentProfile);
	GlobalLogic->RegisterItemTypeProfile(BBMItemTypeTags::Item_Weapon_OneHandSword, SwordLeafProfile);
	GlobalLogic->RegisterItemTypeProfile(BBMItemTypeTags::Item_Weapon_Shield, ShieldLeafProfile);

	TArray<FGameplayTag> LeafTags = GlobalLogic->GetLeafItemTypeTags();
	TestEqual(TEXT("부모 태그가 제외되고 리프 태그 2개만 추출되어야 함"), LeafTags.Num(), 2);
	TestTrue(TEXT("Sword 태그 포함 검증"), LeafTags.Contains(BBMItemTypeTags::Item_Weapon_OneHandSword));
	TestTrue(TEXT("Shield 태그 포함 검증"), LeafTags.Contains(BBMItemTypeTags::Item_Weapon_Shield));
	TestFalse(TEXT("부모인 Item_Weapon 태그는 제외되어야 함"), LeafTags.Contains(BBMItemTypeTags::Item_Weapon));

	// 2. 아이템 정의 에셋 3종 구성 (Common, Rare, Epic)
	UBBMItemDefinition* CommonItem = NewObject<UBBMItemDefinition>();
	CommonItem->RarityTag = BBMItemRarityTags::Item_Rarity_Common;

	UBBMItemDefinition* RareItem = NewObject<UBBMItemDefinition>();
	RareItem->RarityTag = BBMItemRarityTags::Item_Rarity_Rare;

	UBBMItemDefinition* EpicItem = NewObject<UBBMItemDefinition>();
	EpicItem->RarityTag = BBMItemRarityTags::Item_Rarity_Epic;

	// 3. 지역 정의 에셋 및 희귀도 0% 배제 추첨 검증
	UBBMRegionDefinition* TestRegion = NewObject<UBBMRegionDefinition>();
	TestRegion->RegionName = FText::FromString(TEXT("테스트 숲"));

	// 미설정 시 기본 희귀도 4대 태그가 반환되는지 검증
	TMap<FGameplayTag, float> DefaultRarityWeights = TestRegion->GetEffectiveRarityWeights();
	TestTrue(TEXT("기본 희귀도 Common 태그 포함 검증"), DefaultRarityWeights.Contains(BBMItemRarityTags::Item_Rarity_Common));
	TestTrue(TEXT("기본 희귀도 Rare 태그 포함 검증"), DefaultRarityWeights.Contains(BBMItemRarityTags::Item_Rarity_Rare));
	TestTrue(TEXT("기본 희귀도 Epic 태그 포함 검증"), DefaultRarityWeights.Contains(BBMItemRarityTags::Item_Rarity_Epic));
	TestTrue(TEXT("기본 희귀도 Legendary 태그 포함 검증"), DefaultRarityWeights.Contains(BBMItemRarityTags::Item_Rarity_Legendary));
	TestRegion->AvailableItems.Add(CommonItem);
	TestRegion->AvailableItems.Add(RareItem);
	TestRegion->AvailableItems.Add(EpicItem);

	// Rare만 0%로 고의 설정
	TestRegion->RarityWeights.Add(BBMItemRarityTags::Item_Rarity_Common, 100.0f);
	TestRegion->RarityWeights.Add(BBMItemRarityTags::Item_Rarity_Rare, 0.0f);
	TestRegion->RarityWeights.Add(BBMItemRarityTags::Item_Rarity_Epic, 50.0f);

	FRandomStream FixedStream(777);
	for (int32 i = 0; i < 20; ++i)
	{
		UBBMItemDefinition* Picked = TestRegion->PickRandomItem(TestRegion->AvailableItems, TestRegion->RarityWeights, FixedStream);
		TestNotNull(TEXT("추첨된 아이템은 유효해야 함"), Picked);
		TestTrue(TEXT("가중치 0%인 Rare 아이템은 절대 뽑히지 않아야 함"), Picked != RareItem);
	}

	// 중복 없는 2개 추첨 검증
	TArray<UBBMItemDefinition*> MultiPicked = TestRegion->PickRandomItemsWithoutDuplicates(TestRegion->AvailableItems, TestRegion->RarityWeights, 2, FixedStream);
	TestEqual(TEXT("요청한 2개의 아이템이 반환되어야 함"), MultiPicked.Num(), 2);
	TestTrue(TEXT("중복 없는 2개의 아이템이어야 함"), MultiPicked[0] != MultiPicked[1]);

	// 4. 인카운터 정적 정의 에셋 3종 구성 (Treasure, Combat, Event)
	UBBMTreasureEncounterDefinition* EncTreasure = NewObject<UBBMTreasureEncounterDefinition>();
	EncTreasure->EncounterType = EBBMEncounterType::Treasure;

	UBBMCombatEncounterDefinition* EncCombat = NewObject<UBBMCombatEncounterDefinition>();
	EncCombat->EncounterType = EBBMEncounterType::Combat;

	UBBMAICombatantDefinition* MonsterDef = NewObject<UBBMAICombatantDefinition>();
	MonsterDef->DropItemPool.Add(CommonItem);
	MonsterDef->DropItemPool.Add(EpicItem);
	EncCombat->MonsterList.Add(MonsterDef);

	UBBMItemDefinition* MonsterDropped = MonsterDef->PickRandomDropItem(TestRegion->RarityWeights, FixedStream);
	TestNotNull(TEXT("몬스터 드롭 아이템 추첨은 유효해야 함"), MonsterDropped);
	TestTrue(TEXT("몬스터 드롭 풀에 존재하는 아이템이어야 함"), MonsterDropped == CommonItem || MonsterDropped == EpicItem);

	UBBMEventEncounterDefinition* EncEvent = NewObject<UBBMEventEncounterDefinition>();
	EncEvent->EncounterType = EBBMEncounterType::Event;
	EncEvent->NpcClasses.Add(ABBMEventNpcBase::StaticClass());
	EncEvent->NpcClasses.Add(ABBMEventNpcBase::StaticClass());
	EncEvent->NpcClasses.Add(ABBMEventNpcBase::StaticClass());

	TestRegion->CombatEncountersPool.Empty();
	TestRegion->CombatEncountersPool.Add(EncCombat);
	TestRegion->TreasureEncountersPool.Empty();
	TestRegion->TreasureEncountersPool.Add(EncTreasure);
	TestRegion->EventEncountersPool.Empty();
	TestRegion->EventEncountersPool.Add(EncEvent);
	TestRegion->BossEncountersPool.Empty();
	TestRegion->BossEncountersPool.Add(EncCombat);

	// 5. UBBMStageSubsystem에서 신규 런 시작 및 다형성 PlannedEncounters 검증
	UGameInstance* GI = NewObject<UGameInstance>();
	UBBMStageSubsystem* StageSubsystem = NewObject<UBBMStageSubsystem>(GI);
	FSubsystemCollection<UGameInstanceSubsystem> SubsystemCollection;
	StageSubsystem->Initialize(SubsystemCollection);

	UBBMPlayerCharacterDefinition* PlayerDef = NewObject<UBBMPlayerCharacterDefinition>();
	PlayerDef->StartRegionDefinition = TestRegion;

	const bool bStarted = StageSubsystem->StartNewRun(PlayerDef);
	TestTrue(TEXT("신규 런 시작 성공"), bStarted);

	const TArray<TInstancedStruct<FBBMRuntimeEncounterData>>& PlannedEncounters = StageSubsystem->GetPlannedEncounters();
	TestEqual(TEXT("계획된 만남 큐 수량은 5개여야 함"), PlannedEncounters.Num(), 5);

	// 각 만남의 다형성 데이터 검증
	bool bFoundTreasure = false;
	bool bFoundCombat = false;
	bool bFoundEvent = false;

	for (int32 EncIdx = 0; EncIdx < PlannedEncounters.Num(); ++EncIdx)
	{
		const TInstancedStruct<FBBMRuntimeEncounterData>& EncInst = PlannedEncounters[EncIdx];
		TestTrue(TEXT("다형성 인카운터 인스턴스 유효 검증"), EncInst.IsValid());

		const FBBMRuntimeEncounterData& BaseData = EncInst.Get<FBBMRuntimeEncounterData>();
		const EBBMEncounterType Type = BaseData.GetEncounterType();

		if (Type == EBBMEncounterType::Treasure)
		{
			bFoundTreasure = true;
			const FBBMRuntimeTreasureEncounterData* TreasureData = EncInst.GetPtr<FBBMRuntimeTreasureEncounterData>();
			TestNotNull(TEXT("보물 방 파생 데이터 캐스팅 성공 검증"), TreasureData);
			if (TreasureData != nullptr)
			{
				TestTrue(TEXT("보물 선택지는 1개 이상 3개 이하"), TreasureData->TreasureOptions.Num() >= 1 && TreasureData->TreasureOptions.Num() <= 3);
			}
		}
		else if (Type == EBBMEncounterType::Combat || Type == EBBMEncounterType::BossCombat)
		{
			bFoundCombat = true;
			const FBBMRuntimeCombatEncounterData* CombatData = EncInst.GetPtr<FBBMRuntimeCombatEncounterData>();
			TestNotNull(TEXT("전투 방 파생 데이터 캐스팅 성공 검증"), CombatData);
			if (CombatData != nullptr)
			{
				TestEqual(TEXT("몬스터 1마리로부터 1개의 드롭 보상이 사전 추첨되어야 함"), CombatData->CombatRewardItems.Num(), 1);
				if (CombatData->CombatRewardItems.Num() > 0)
				{
					TestTrue(TEXT("드롭 아이템은 MonsterDef 풀 소속이어야 함"),
						CombatData->CombatRewardItems[0] == CommonItem || CombatData->CombatRewardItems[0] == EpicItem);
				}
			}
		}
		else if (Type == EBBMEncounterType::Event)
		{
			bFoundEvent = true;
			const FBBMRuntimeEventEncounterData* EventData = EncInst.GetPtr<FBBMRuntimeEventEncounterData>();
			TestNotNull(TEXT("이벤트 방 파생 데이터 캐스팅 성공 검증"), EventData);
			if (EventData != nullptr)
			{
				TestEqual(TEXT("이벤트 방 NPC는 3인이어야 함"), EventData->EventNpcs.Num(), 3);
				if (EventData->EventNpcs.Num() == 3)
				{
					// NPC 3인의 요구 카테고리 태그 상호 중복 없음 검증
					const FGameplayTag TagA = EventData->EventNpcs[0].DesiredItemTypeTag;
					const FGameplayTag TagB = EventData->EventNpcs[1].DesiredItemTypeTag;
					const FGameplayTag TagC = EventData->EventNpcs[2].DesiredItemTypeTag;

					TestTrue(TEXT("요구 태그는 유효해야 함"), TagA.IsValid() && TagB.IsValid() && TagC.IsValid());
					TestTrue(TEXT("NPC A와 B의 요구 태그는 서로 달라야 함"), TagA != TagB);
					TestTrue(TEXT("NPC B와 C의 요구 태그는 서로 달라야 함"), TagB != TagC);
					TestTrue(TEXT("NPC A와 C의 요구 태그는 서로 달라야 함"), TagA != TagC);
				}
			}
		}

		// 6. 각 만남별 CollectPrimaryAssetIds(PAD 수집) 검증
		TArray<FPrimaryAssetId> EncounterPadList;
		BaseData.CollectPrimaryAssetIds(EncounterPadList);
		TestTrue(TEXT("각 만남 데이터에서 1개 이상의 PrimaryAssetId가 수집되어야 함"), EncounterPadList.Num() >= 1);
	}

	// 7. UBBMStageSubsystem의 PreloadEncounterAssets 실행 검증
	const bool bPreloadSuccess = StageSubsystem->PreloadEncounterAssets(1);
	TestTrue(TEXT("1번 만남 에셋 사전 프리로드 호출은 성공해야 함"), bPreloadSuccess);

	return true;
}

// 6. EBBMEncounterType별 확률에 따른 2단계 인카운터 추첨 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMEncounterTypeWeightSelectionTest,
	"DIS.BBM.Stage.EncounterTypeWeightSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMEncounterTypeWeightSelectionTest::RunTest(const FString& Parameters)
{
	UBBMRegionDefinition* Region = NewObject<UBBMRegionDefinition>();
	Region->RegionName = FText::FromString(TEXT("가중치 테스트 지역"));

	UBBMCombatEncounterDefinition* CombatEnc = NewObject<UBBMCombatEncounterDefinition>();
	CombatEnc->EncounterType = EBBMEncounterType::Combat;
	Region->CombatEncountersPool.Add(CombatEnc);

	UBBMEventEncounterDefinition* EventEnc = NewObject<UBBMEventEncounterDefinition>();
	EventEnc->EncounterType = EBBMEncounterType::Event;
	Region->EventEncountersPool.Add(EventEnc);

	UBBMTreasureEncounterDefinition* TreasureEnc = NewObject<UBBMTreasureEncounterDefinition>();
	TreasureEnc->EncounterType = EBBMEncounterType::Treasure;
	Region->TreasureEncountersPool.Add(TreasureEnc);

	UBBMCombatEncounterDefinition* BossEnc = NewObject<UBBMCombatEncounterDefinition>();
	BossEnc->EncounterType = EBBMEncounterType::BossCombat;
	Region->BossEncountersPool.Add(BossEnc);

	// 100% 보물 방만 등장하도록 설정
	Region->EncounterTypeWeights.Add(EBBMEncounterType::Treasure, 100.0f);
	Region->EncounterTypeWeights.Add(EBBMEncounterType::Combat, 0.0f);
	Region->EncounterTypeWeights.Add(EBBMEncounterType::Event, 0.0f);

	FRandomStream Stream(12345);
	TArray<UBBMEncounterDefinition*> Encounters = Region->GenerateFiveEncounters(Stream);

	TestEqual(TEXT("생성된 인카운터 총 5개 검증"), Encounters.Num(), 5);
	for (int32 i = 0; i < 4; ++i)
	{
		TestEqual(TEXT("1~4회차는 100% 가중치에 따라 보물 방이어야 함"), Encounters[i]->GetEncounterType(), EBBMEncounterType::Treasure);
		TestTrue(TEXT("선택된 에셋은 TreasureEnc여야 함"), Encounters[i] == TreasureEnc);
	}
	TestEqual(TEXT("5회차는 보스 만남이어야 함"), Encounters[4]->GetEncounterType(), EBBMEncounterType::BossCombat);

	// 100% 전투 방만 등장하도록 설정 변경
	Region->EncounterTypeWeights[EBBMEncounterType::Treasure] = 0.0f;
	Region->EncounterTypeWeights[EBBMEncounterType::Combat] = 100.0f;

	TArray<UBBMEncounterDefinition*> CombatOnlyEncounters = Region->GenerateFiveEncounters(Stream);
	for (int32 i = 0; i < 4; ++i)
	{
		TestEqual(TEXT("1~4회차는 100% 가중치에 따라 전투 방이어야 함"), CombatOnlyEncounters[i]->GetEncounterType(), EBBMEncounterType::Combat);
	}

	return true;
}

// 7. 2단계 계층적 희귀도 기반 아이템 추첨(Two-Step Tier Selection) 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMTwoStepRarityItemSelectionTest,
	"DIS.BBM.Stage.TwoStepRarityItemSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMTwoStepRarityItemSelectionTest::RunTest(const FString& Parameters)
{
	UBBMRegionDefinition* Region = NewObject<UBBMRegionDefinition>();

	// Common 아이템 10개 생성
	TArray<TSoftObjectPtr<UBBMItemDefinition>> ItemPool;
	for (int32 i = 0; i < 10; ++i)
	{
		UBBMItemDefinition* CommonItem = NewObject<UBBMItemDefinition>();
		CommonItem->RarityTag = BBMItemRarityTags::Item_Rarity_Common.GetTag();
		ItemPool.Add(CommonItem);
	}

	// Rare 아이템 1개 생성
	UBBMItemDefinition* RareItem = NewObject<UBBMItemDefinition>();
	RareItem->RarityTag = BBMItemRarityTags::Item_Rarity_Rare.GetTag();
	ItemPool.Add(RareItem);

	FRandomStream Stream(9999);

	// Case 1: Rare 100%, Common 0% 가중치 설정 시 Common 10개가 있어도 Rare만 당첨되어야 함
	TMap<FGameplayTag, float> RareOnlyWeights;
	RareOnlyWeights.Add(BBMItemRarityTags::Item_Rarity_Rare.GetTag(), 100.0f);
	RareOnlyWeights.Add(BBMItemRarityTags::Item_Rarity_Common.GetTag(), 0.0f);

	UBBMItemDefinition* PickedRare = Region->PickRandomItem(ItemPool, RareOnlyWeights, Stream);
	TestEqual(TEXT("Rare 100% 가중치 시 Rare 아이템이 반드시 뽑혀야 함"), PickedRare, RareItem);

	// Case 2: Common 100%, Rare 0% 가중치 설정 시 Common 아이템 중 하나가 뽑혀야 함
	TMap<FGameplayTag, float> CommonOnlyWeights;
	CommonOnlyWeights.Add(BBMItemRarityTags::Item_Rarity_Rare.GetTag(), 0.0f);
	CommonOnlyWeights.Add(BBMItemRarityTags::Item_Rarity_Common.GetTag(), 100.0f);

	UBBMItemDefinition* PickedCommon = Region->PickRandomItem(ItemPool, CommonOnlyWeights, Stream);
	TestNotNull(TEXT("Common 아이템이 유효하게 뽑혀야 함"), PickedCommon);
	TestTrue(TEXT("뽑힌 아이템의 희귀도는 Common이어야 함"), PickedCommon->RarityTag.MatchesTagExact(BBMItemRarityTags::Item_Rarity_Common.GetTag()));

	// Case 3: 중복 배제 추첨(PickRandomItemsWithoutDuplicates) 검증
	TArray<UBBMItemDefinition*> UniquePicked = Region->PickRandomItemsWithoutDuplicates(ItemPool, CommonOnlyWeights, 3, Stream);
	TestEqual(TEXT("중복 없이 3개 추출 검증"), UniquePicked.Num(), 3);
	TestTrue(TEXT("추출된 3개는 서로 다른 객체여야 함"),
		UniquePicked[0] != UniquePicked[1] &&
		UniquePicked[1] != UniquePicked[2] &&
		UniquePicked[0] != UniquePicked[2]);

	return true;
}

/**
 * 런 세이브 데이터(UBBMRunSaveGame) -> 상인 폰(ABBMPlayerMerchantBase) 복원(Restore) 및 역방향 추출(Extract) 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMRunSaveMerchantRestoreTest,
	"DIS.BBM.Save.RestoreMerchantFromRunSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMRunSaveMerchantRestoreTest::RunTest(const FString& Parameters)
{
	FBBMStageScopedTestWorld ScopedWorld;
	UWorld* World = ScopedWorld.World;
	TestNotNull(TEXT("테스트 월드 유효성 검증"), World);
	if (World == nullptr)
	{
		return false;
	}

	// 1. 테스트용 플레이어 상인 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABBMPlayerMerchantBase* Merchant = World->SpawnActor<ABBMPlayerMerchantBase>(
		ABBMPlayerMerchantBase::StaticClass(),
		FTransform::Identity,
		SpawnParams
	);
	TestNotNull(TEXT("플레이어 상인 스폰 검증"), Merchant);
	if (Merchant == nullptr)
	{
		return false;
	}

	// 2. 3x3 배낭 인벤토리 초기화
	UBBMInventoryDefinition* InvenDef = NewObject<UBBMInventoryDefinition>();
	InvenDef->DefaultCapacity = 9;
	InvenDef->LineBreakTileCount = 3;
	Merchant->GetInventoryComponent()->InitializeInventory(InvenDef);
	TestNotNull(TEXT("상인 인벤토리 인스턴스 유효성 검증"), Merchant->GetInventory());

	// 3. 테스트용 아이템 정의 에셋 생성 (1x1 단검)
	UBBMItemDefinition* DaggerDef = NewObject<UBBMItemDefinition>();
	DaggerDef->Shape.Points = { FIntPoint(0, 0) };

	// 4. 런 세이브 데이터 준비 (체력 42.0, 골드 350, 슬롯 4(중앙 칸) 단검 배치)
	UBBMRunSaveGame* RunSave = NewObject<UBBMRunSaveGame>();
	RunSave->bHasActiveRun = true;
	RunSave->CurrentHealth = 42.0f;
	RunSave->MaxHealth = 100.0f;
	RunSave->CurrentGold = 350;

	FBBMInventorySlotSaveRecord SlotRecord;
	SlotRecord.SlotIndex = 4;
	SlotRecord.ItemDefinitionPath = FSoftObjectPath(DaggerDef);
	SlotRecord.StackCount = 1;
	SlotRecord.Rotation = EDISItemRotation::Rotate0;
	RunSave->SavedInventorySlots.Add(SlotRecord);

	// 5. 세이브 -> 상인 데이터 복원 (ApplyRunSaveData)
	const bool bRestoreSuccess = Merchant->ApplyRunSaveData(RunSave);
	TestTrue(TEXT("세이브 데이터 상인 복원 성공 검증"), bRestoreSuccess);

	// 6. 복원된 스탯/골드/인벤토리 단언
	TestEqual(TEXT("체력 스탯 복원 값 검증 (42.0f)"), Merchant->GetAttributeSet()->GetHealth(), 42.0f);
	TestEqual(TEXT("골드 복원 값 검증 (350)"), Merchant->GetGold(), 350);
	TestFalse(TEXT("슬롯 4가 비어있지 않아야 함"), Merchant->GetInventory()->IsSlotEmpty(4));

	const UDISItemStack* RestoredStack = Merchant->GetInventory()->GetSlotItemStack(4);
	TestNotNull(TEXT("슬롯 4의 아이템 스택 유효성 검증"), RestoredStack);
	if (RestoredStack != nullptr)
	{
		TestEqual(TEXT("슬롯 4 아이템 정의 일치 검증"), RestoredStack->GetTopEntry()->GetItemDefinition(), static_cast<const UDISItemDefinition*>(DaggerDef));
		TestEqual(TEXT("슬롯 4 수량 검증 (1)"), RestoredStack->GetStackCount(), 1);
	}

	// 7. 상인 -> 세이브 역방향 추출(ExtractLevelData) 검증
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UBBMGameDataSubsystem* GameData = NewObject<UBBMGameDataSubsystem>(GameInstance);
	FSubsystemCollection<UGameInstanceSubsystem> SubsystemCollection;
	GameData->Initialize(SubsystemCollection);

	const FPrimaryAssetId DummyRegionId(TEXT("BBMRegionDefinition"), TEXT("DA_Region_Forest"));
	TArray<FPrimaryAssetId> DummyPlannedIds;

	GameData->ExtractLevelData(
		Merchant,
		DummyRegionId,
		2,
		DummyPlannedIds,
		12345
	);

	const UBBMRunSaveGame* ExtractedRun = GameData->GetCurrentRunData();
	TestNotNull(TEXT("추출된 런 세이브 데이터 유효성 검증"), ExtractedRun);
	if (ExtractedRun != nullptr)
	{
		TestEqual(TEXT("추출된 체력 검증 (42.0f)"), ExtractedRun->CurrentHealth, 42.0f);
		TestEqual(TEXT("추출된 골드 검증 (350)"), ExtractedRun->CurrentGold, 350);
		TestEqual(TEXT("추출된 다음 인카운터 인덱스 검증 (2)"), ExtractedRun->CurrentEncounterIndex, 2);
		TestEqual(TEXT("추출된 슬롯 레코드 수 검증 (1개)"), ExtractedRun->SavedInventorySlots.Num(), 1);
		if (ExtractedRun->SavedInventorySlots.Num() > 0)
		{
			TestEqual(TEXT("추출된 슬롯 번호 검증 (4)"), ExtractedRun->SavedInventorySlots[0].SlotIndex, 4);
		}
	}

	Merchant->Destroy();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
