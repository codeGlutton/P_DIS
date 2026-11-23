# 05. BBM 전역 설정 · GameFeature · 로컬라이제이션

호스트 전역 설정, GameFeature 모듈 콘텐츠 및 타깃별 다국어 로컬라이제이션 파이프라인 명세임.

## 1. BBM 전역 설정 및 GameFeature (`Source/BBM/{Global,GameFeatures}/`)
- **모듈 진입점**: `BBM.Build.cs`, `BBM.h/.cpp`
- **전역 설정 (`UBBMGlobalSettings`, `BBMGlobalTypes`)**:
  - 오디오 믹스/사운드 클래스, 팀 관계 태도 솔버, 크레딧 카테고리 목록
  - 입력 매핑(`DefaultInputMappingContext`, `ModifierInputMappingContext`, `Shift`/`Ctrl`/`Alt` Chord Action)
  - 선택 옵션 테이블(`LanguageOptions`, `WindowModeOptions`, `ScreenResolutionOptions` 및 공용 타입 `FBBMStringSelectionData`, `FBBMWindowModeSelectionData`, `FBBMIntPointSelectionData`)
- **GameFeature 액션 (`Source/BBM/GameFeatures/`)**: 모듈형 피처 마운트 커스텀 액션 구현부

## 2. GameFeature 코어 비-UI 콘텐츠 (`Plugins/GameFeatures/BBMGameFeatureCore/Content/`)
- **전역·오디오·입력**: `Globals/`(`DA_GlobalDefinitionCore`, `ST_BBMGameFeatureCore_Global`), `Audio/`(`Mix_BBM_UserSettings`, `SC_BBM_Master` → `BGM`/`SFX`/`Dialogue`), `Input/`(`IMC_Default`, `IMC_Modifier`, `ST_BBMGameFeatureCore_Input`), `Input/Actions/`(8종 Enhanced Input 액션)
- **인게임 데이터·레벨**: `Item/`(6종 `DA_Item_*`, `ST_BBMGameFeatureCore_Item`), `Inventory/`(`DA_Player_Inventory`, `ST_BBMGameFeatureCore_Inventory`), `Combat/`(몬스터 4종 `DA_Monster_*`, `BP_BBMCombatant_Monster`, `DA_Player_DefaultMerchant`, `BP_BBMPlayerMerchant`), `Stage/`(만남 5종, `BP_BBMChestActor`, `BP_BBMEventNpc`, 8종 `DA_Region_*`), `Level/`(`BP_GameMode_Title`, `BP_GameMode_InGame`, `BP_BBMCameraActor`, `L_Title.umap`, `L_Test.umap`)

## 3. 호스트 일반 콘텐츠·설정 (`Content/`, `Config/`)
- `Content/Level/`(테스트 맵), `Content/Collections/`, `Content/Developers/`, `Content/UI/`, `Content/Util/`(`BP_Rename`), `Config/`(전역 `.ini`)

## 4. 독립 로컬라이제이션 파이프라인
- **호스트 (`Config/Localization/`, `Content/Localization/`)**: `Game`, `GameEditor` 타깃
- **플러그인·피처 (`Plugins/*/Config/Localization/`, `Plugins/*/Content/Localization/`)**: `.uplugin` `"LocalizationTargets"`(`ConfigGenerationPolicy: "User"`) 기반 `DIS`, `DISEditor`, `AssetLinter`, `BBMGameFeatureCore` 독립 타깃 4종 `.ini` 및 `.locres`/`.po` 산출물
