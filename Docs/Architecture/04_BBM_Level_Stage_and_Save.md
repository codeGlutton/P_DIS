# 04. BBM 레벨 · 스테이지 진행 · 세이브 (`Source/BBM/{Level,Stage,Save}/`)

게임모드 흐름, 룸·만남 추첨 스테이지 진행, 영속 세이브 및 유저 옵션 복원 기능의 상세 명세임.

## 1. 레벨 및 게임플레이 진입 (`Source/BBM/Level/`)
- **게임모드**: `ABBMGameModeBase`(인카운터 전/후 UI 자동 생성 `CreateWorldUIsByPhase`, `AutoCreateUIs`, 8초 준비/시뮬레이션 흐름 제어), `ABBMTitleGameMode`, `ABBMStageGameMode`
- **컨트롤러·카메라·월드**: `ABBMPlayerController`(`SetupInputComponent` 내 `RegisterIMCs` 호출, F1~F5/`bbm.*` 치트), `ABBMCameraActorBase`, `ABBMWorldSettings`(`FBBMRoomSpawnSetting`)

## 2. 스테이지 만남 및 지역 추첨 (`Source/BBM/Stage/`)
- **서브시스템**: `UBBMStageSubsystem` — 5회 만남 순회·방 전환, 다형성 런타임 인카운터 큐(`TInstancedStruct<FBBMRuntimeEncounterData>`) 관리
- **정의 에셋 & 액터**: `UBBMEncounterDefinition` 계열(`Combat`, `Treasure`, `Event`), `UBBMRegionDefinition`(2단계 계층 추첨 풀, `TileMapActorClass`), `ABBMChestActorBase`, `ABBMEventNpcBase`

## 3. 영속 세이브 및 유저 설정 (`Source/BBM/Save/`)
- **세이브 오브젝트**: 단일 런 슬롯(`UBBMRunSaveGame`, 소지 골드 `CurrentGold` 포함), 영구 유저 설정(`UBBMUserSaveGame`: 통합 `FBBMUserSettingsData`, `GetSettingsData`/`SetSettingsData`, `FBBMKeyMappingData`, `FBBMKeyCategoryMappingData`, `KeyMappings`)
- **데이터 서브시스템 (`UBBMGameDataSubsystem`)**: 런 초기화(`ResetAndInitializeNewRun`), 상인 역방향 복원(`ApplyRunDataToMerchant`), 오디오/디스플레이/입력(`RuntimeIMC` 복제, `RemapByName`, `RegisterIMCs`, `GetDefaultKeyMappings`) 런타임 적용·복원, `OnUserSettingsChanged` 통지, 비동기 저장/로드
