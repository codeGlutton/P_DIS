# 06. UI 프레임워크 · 위젯 · MVVM 뷰모델 (`WorldUI`, `ExtensionWidget`, `BBM/UI`)

월드 UI 레이어 프레임워크, 범용 Slate/UMG 확장 위젯, BBM 호스트 위젯·뷰모델 및 UI 콘텐츠 에셋 명세임.

## 1. 월드 UI 수명주기 및 전환 프레임워크 (`Plugins/WorldUI/`)
- **공용 타입 (`WorldUITypes`)**: `EWorldWidgetZOrder`(0~7), `EWorldWidgetLifecyclePolicy`, `EWorldWidgetAnimPlayMode`, `FWorldWidgetAnimConfig`, `ESingleAnimState`, `EDualAnimState`, `FWorldWidgetConfig`, `FWorldViewModelConfig`
- **위젯 계층 (`Widget/`)**: `UWorldUISubsystem`(월드 전역 위젯/ViewModel 수명주기 및 ZOrder 레이어 관리), `UWorldLayerRootWidget`(`FWorldUILayerState`), `UWorldUserWidgetBase`(단일 ViewModel 프레젠테이션 베이스)
- **뷰모델 계층 (`ViewModel/`)**: `UWorldViewModelBase`, 양방향 MVVM 전환 모듈(`USingleAnimTransition`, `UDualAnimTransition`)

## 2. 범용 Slate/UMG 확장 위젯 (`Plugins/ExtensionWidget/`)
- **런타임 (`Widgets/`, `Components/`, `Types/`)**: `SBorder`→`SRichScrollBar`/`URichScrollBar`, `SPanel`→`SRichScrollTrack`, `URichVerticalBox`/`URichHorizontalBox`, `URichSlider`, `URichComboBoxKey`, `URichInputKeySelector`, `URichListView`(`UListView` 브릿지) — 동적/고정 썸 크기, 양단 화살표, 역순 렌더링·HitTest 패널, FieldNotify 선 대입 양방향 바인딩 지원

## 3. BBM 호스트 위젯 및 MVVM 뷰모델 (`Source/BBM/UI/`)
- **공용 UI 타입 (`BBMUITypes.h`)**: `EBBMWidgetCreationPhase`, `FBBMWorldUIConfig`, `FBBMCreditCategory`
- **위젯 (`Widget/`)**: `UWorldUserWidgetBase` 파생 화면(`UBBMTitleWidget`, `UBBMSettingsWidget`, `UBBMCharacterSelectWidget`, `UBBMCreditsWidget`, `UBBMLoadingWidget`, `UBBMSaveWidget`), 탭 엔트리(`UBBMTabEntryWidget`, `UBBMSettingsTabEntryWidget`), 콤보박스 엔트리(`UBBMComboBoxTextEntryWidget`), ListView 엔트리(`UBBMCreditEntryWidget`, `UBBMKeyMappingEntryWidget`, `UBBMKeyMappingCategoryEntryWidget`)
- **뷰모델 (`ViewModel/`)**: `UBBMTabViewModelBase`, `UBBMTabEntryViewModel`, `UBBMSettingsViewModel`(오디오·그래픽·언어/창모드/해상도 콤보 3종, 키매핑 스냅샷/Apply/Reset), `UBBMCharacterSelectViewModel`(`StartNewGame`), `UBBMCreditsViewModel`, `UBBMTitleViewModel`, 서브 엔트리 뷰모델(`UBBMComboBoxText*`, `UBBMCreditEntryViewModel`, `UBBMKeyMapping*`)

## 4. BBM UI 콘텐츠 및 리소스 (`Plugins/GameFeatures/BBMGameFeatureCore/Content/`)
- **폰트 (`Asset/Font/`)**: `Font_Pretendard`, `FF_Pretendard_Regular/Medium/Bold`, `FF_BM_JUA`, `FF_LilitaOne_Regular`
- **UI 텍스처·머티리얼 (`Asset/UI/`)**: `Background/`, `Buttons/`, `Frames/`, `FX/`, `Icons/`, `Logo/`, `Portraits/`, `ProgressBar/`, `Props/`, `Scrollbar/`, `Slider/`, `Materials/M_UI_PortraitRing`
- **위젯 블루프린트 (`UI/`)**: `WBP_Title`, `WBP_CharacterSelect`, `Entry/WBP_CharSelectRosterEntry`, `WBP_Settings`, `WBP_Credits`, `WBP_Loading`, `WBP_Save`, `ST_BBMGameFeatureCore_UI`
