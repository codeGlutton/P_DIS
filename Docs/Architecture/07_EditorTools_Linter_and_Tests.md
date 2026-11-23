# 07. 에디터 도구 · 에셋 린터 · 자동화 테스트 (`Editor`, `AssetLinter`, `DISTest`)

에디터 디테일 커스터마이징, 에셋 네이밍/경로 검증 린터, 단위 및 스트레스 테스트 모듈 명세임.

## 1. 에디터 커스터마이징 및 유틸리티
- **DIS 에디터 (`Plugins/DIS/Source/DISEditor/`)**: 인벤토리 데이터 에셋 유효성 검사, 에셋 액션, 디테일 커스터마이제이션
- **BBM 에디터 (`Source/BBMEditor/`)**:
  - `Customizations/`: `FBBMTeamRelation` 누락 팀 자동 채우기, `FBBMBeatSequence` 4단 타임라인 시각화, `UBBMInventoryDefinition` 배낭 2D 프리뷰 커스터마이제이션
  - `AssetActions/`: 일괄 리네임 액션 유틸리티(`UBBMAssetRenameActionUtility`)
- **ExtensionWidget 에디터 (`Plugins/ExtensionWidget/Source/ExtensionWidgetEditor/`)**: `URichComboBoxKey` ViewModel Extension 토글 및 Content/Item 위젯·엔트리 ViewModel 드롭다운 선택(`FRichComboBoxKeyCustomizationExtender`)

## 2. 에셋 자동 검사 린트 플러그인 (`Plugins/AssetLinter/`)
- **검증기**: DataValidation 서브시스템 + `UEditorValidatorBase` 기반 `UAssetLinterValidator`, `UAssetLinterSettings`([DefaultEditor.ini](file:///e:/Unreal/P_AI_Inven/DIS/Config/DefaultEditor.ini)에서 Prefix/Suffix·대소문자·스코프·재귀 설정), Most-Derived 상속 거리 판정
- **일괄 수정 UI**: `UAssetActionUtility` 기반 선택 에셋 검사 및 `SAssetLinterBatchFixDialog` 모달 확인 후 원자적 일괄 수정, 내장 유틸리티 `Content/Util/BP_AssetLint`

## 3. 자동화 · 스트레스 · 성능 테스트 (`Source/DISTest/Private/Tests/`)
- **DIS 코어 테스트**: `DISInventoryTests`, `DISSharedInventoryTests`, `DISItemTypeHierarchyTests`, `DISRarityTests`, `DISEnhancementTests`, `DISGlobalSubsystemCustomizationTests`, `DISStressTests`
- **BBM 게임플레이 테스트**: `BBMInventoryTests`, `BBMItemGroupTests`, `BBMItemProfileTests`, `BBMCombatManagerTests`, `BBMStageTests`
- **UI·위젯 테스트**: `BBMWorldWidgetTests`, `BBMTitleViewModelTests`, `RichWidgetTests` (`RichWidgetTestHelpers`)
