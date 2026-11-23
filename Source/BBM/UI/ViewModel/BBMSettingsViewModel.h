/*****************************************************************//**
 * @file   BBMSettingsViewModel.h
 * @brief  BBM(비트 배낭 상인) 환경설정 화면 MVVM 뷰모델 클래스 정의
 * @details GameDataSubsystem과의 대리자 동기화, 즉각 런타임 적용, Apply 확정 저장 및 Cancel/닫힘 시 이전 설정 롤백 처리를 전담함
 * @date   2026-11-13
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "UI/ViewModel/BBMTabViewModelBase.h"
#include "Save/BBMSaveTypes.h"
#include "BBMSettingsViewModel.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMComboBoxTextViewModel;
class UBBMGameDataSubsystem;
class UBBMKeyMappingCategoryEntryViewModel;

/**
 * 환경 설정 패널 ViewModel
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Settings ViewModel"))
class BBM_API UBBMSettingsViewModel : public UBBMTabViewModelBase
{
	GENERATED_BODY()

	/* 생성자 및 수명 주기 */
public:
	UBBMSettingsViewModel();

protected:
	virtual void BeginViewModel() override;
	virtual void EndViewModel() override;
	virtual void OnOpen() override;
	virtual void OnClosed() override;

	/* 필드 노티파이 프로퍼티 (Getter / Setter) */
public:
	/* 마스터 볼륨 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	float GetMasterVolume() const;

	/* 마스터 볼륨 설정 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetMasterVolume(float InMasterVolume);

	/* BGM 볼륨 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	float GetBGMVolume() const;

	/* BGM 볼륨 설정 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetBGMVolume(float InBGMVolume);

	/* SFX 볼륨 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	float GetSFXVolume() const;

	/* SFX 볼륨 설정 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetSFXVolume(float InSFXVolume);

	/* 다이얼로그 볼륨 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	float GetDialogueVolume() const;

	/* 다이얼로그 볼륨 설정 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetDialogueVolume(float InDialogueVolume);

	/* 박자 시각 보조 체크박스 상태 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	ECheckBoxState GetEnableBeatVisualAssist() const;

	/* 박자 시각 보조 체크박스 상태 설정 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetEnableBeatVisualAssist(ECheckBoxState InEnableBeatVisualAssist);

	/* 언어 옵션 키 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	FName GetLanguageKey() const;

	/* 언어 옵션 키 설정 및 Settings 옵션 테이블 기반 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetLanguageKey(FName InLanguageKey);

	/* 창 모드 옵션 키 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	FName GetWindowModeKey() const;

	/* 창 모드 옵션 키 설정 및 Settings 옵션 테이블 기반 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetWindowModeKey(FName InWindowModeKey);

	/* 화면 해상도 옵션 키 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	FName GetScreenResolutionKey() const;

	/* 화면 해상도 옵션 키 설정 및 Settings 옵션 테이블 기반 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetScreenResolutionKey(FName InScreenResolutionKey);

	/* 수직 동기화(VSync) 체크박스 상태 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	ECheckBoxState GetVSyncEnabled() const;

	/* 수직 동기화(VSync) 체크박스 상태 설정 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetVSyncEnabled(ECheckBoxState InVSyncEnabled);

	/* 카테고리별 키 매핑 배열 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	const TArray<FBBMKeyCategoryMappingData>& GetKeyMappings() const { return KeyMappings; }

	/* 카테고리별 키 매핑 배열 일괄 설정 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetKeyMappings(const TArray<FBBMKeyCategoryMappingData>& InKeyMappings);

	/* ListView 바인딩용 키 매핑 카테고리 엔트리 ViewModel 배열 반환 */
	const TArray<TObjectPtr<UBBMKeyMappingCategoryEntryViewModel>>& GetKeyMappingCategoryEntries() const { return KeyMappingCategoryEntries; }

	/* 카테고리·엔트리 인덱스로 단일 키 조합을 갱신하고 엔트리 VM 동기화 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void RemapKey(int32 InCategoryIndex, int32 InEntryIndex, const FInputChord& InNewChord);

	/* 미확정 변경 사항 존재 여부 반환 (Apply 버튼 활성화 바인딩용) */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	bool GetHasPendingEdit() const { return bHasPendingEdit; }

	/* 텍스트 콤보박스 서브 뷰모델 조회 */
public:
	/* 언어 선택 콤보박스 모듈 뷰모델 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	UBBMComboBoxTextViewModel* GetLanguageComboBoxViewModel() const { return LanguageComboBoxViewModel; }

	/* 창 모드 선택 콤보박스 모듈 뷰모델 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	UBBMComboBoxTextViewModel* GetWindowModeComboBoxViewModel() const { return WindowModeComboBoxViewModel; }

	/* 화면 해상도 선택 콤보박스 모듈 뷰모델 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	UBBMComboBoxTextViewModel* GetScreenResolutionComboBoxViewModel() const { return ScreenResolutionComboBoxViewModel; }

	/* 환경설정 UI 커맨드 인터페이스 */
public:
	/* 현재 적용 중인 설정을 영구 유저 데이터로 확정 및 비동기 디스크 저장 후 창 닫기 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void ApplySettingsAndCloseWithAnim();

	/* 변경 사항을 취소하고 진입 시점에 캐싱해둔 이전 설정으로 복원 (디스크 저장 없음) 후 창 닫기  */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void CancelSettingsAndCloseWithAnim();

	/* 현재 적용 중인 설정을 영구 유저 데이터로 확정 및 비동기 디스크 저장 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void ApplySettings();

	/* 변경 사항을 취소하고 진입 시점에 캐싱해둔 이전 설정으로 복원 (디스크 저장 없음) */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void CancelSettings();

	/* 모든 설정을 기본값으로 복원 후 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void ResetToDefaults();

	/* 엔트리 초기화 헬퍼 */
private:
	void AddFixedTabEntries();
	void AddFixedComboBoxEntries();

	void RemoveAllKeyMappingCategoryEntries();
	void RemoveAllComboBoxEntries();

	/* 콤보박스 모듈 양방향 동기화 헬퍼 */
private:
	void RebindComboBoxViewModels();
	void UnbindComboBoxViewModels();
	void RefreshComboBoxSelections();
	void RefreshComboBoxSelection(UBBMComboBoxTextViewModel* InComboBoxVM, FName InSelectedKey);

	void HandleLanguageComboBoxSelectedKeyChanged(FName InSelectedKey);
	void HandleWindowModeComboBoxSelectedKeyChanged(FName InSelectedKey);
	void HandleScreenResolutionComboBoxSelectedKeyChanged(FName InSelectedKey);

	/* 내부 동기화 및 서브시스템 조회 헬퍼 */
private:
	/* GameDataSubsystem 캐싱 조회 및 반환 (미발견 시 에러 로그 출력) */
	UBBMGameDataSubsystem* GetGameDataSubsystem();

	/* 현재 ViewModel 값을 GameDataSubsystem 런타임 적용 API로 전달 */
	void PushCurrentValuesToSubsystem();

	/* 전달된 스냅샷 구조체로부터 ViewModel FieldNotify 프로퍼티 일괄 갱신 */
	void SyncFromSettingsData(const FBBMUserSettingsData& InSettings);

	/* GameDataSubsystem::OnUserSettingsChanged 수신 콜백 */
	void HandleSubsystemSettingsChanged(const FBBMUserSettingsData& NewSettings);

	/* 키 설정 동기화 헬퍼 */
private:
	void SetKeyMappingCategoryEntries(const TArray<TObjectPtr<UBBMKeyMappingCategoryEntryViewModel>>& InKeyMappingCategoryEntries);

	void RebindKeyMappingCategoryEntries();
	void UnbindKeyMappingCategoryEntries();
	void RefreshKeyMappingSingleEntry(int32 InCategoryIndex, int32 InEntryIndex, const FInputChord& InNewChord);
	void RefreshKeyMappingCategoryEntries();

	void HandleCategoryInputChordChanged(int32 InCategoryIndex, int32 InEntryIndex, FName InMappingName, const FInputChord& InNewChord);

	/* 필드 노티파이 프로퍼티 */
private:
	/* 마스터 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetMasterVolume, Getter = GetMasterVolume, meta = (AllowPrivateAccess = "true"))
	float MasterVolume = 1.0f;

	/* 배경음악(BGM) 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetBGMVolume, Getter = GetBGMVolume, meta = (AllowPrivateAccess = "true"))
	float BGMVolume = 0.8f;

	/* 효과음(SFX) 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetSFXVolume, Getter = GetSFXVolume, meta = (AllowPrivateAccess = "true"))
	float SFXVolume = 1.0f;

	/* 다이얼로그 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetDialogueVolume, Getter = GetDialogueVolume, meta = (AllowPrivateAccess = "true"))
	float DialogueVolume = 1.0f;

	/* 박자 시각 보조 체크박스 상태 */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetEnableBeatVisualAssist, Getter = GetEnableBeatVisualAssist, meta = (AllowPrivateAccess = "true"))
	ECheckBoxState EnableBeatVisualAssist = ECheckBoxState::Checked;

	/* 언어 옵션 식별 키 (예: "Korean", "English") */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetLanguageKey, Getter = GetLanguageKey, meta = (AllowPrivateAccess = "true"))
	FName LanguageKey = NAME_None;

	/* 창 모드 옵션 식별 키 (예: "Fullscreen", "WindowedFullscreen", "Windowed") */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetWindowModeKey, Getter = GetWindowModeKey, meta = (AllowPrivateAccess = "true"))
	FName WindowModeKey = NAME_None;

	/* 화면 해상도 옵션 식별 키 (예: "HD", "FHD", "QHD") */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetScreenResolutionKey, Getter = GetScreenResolutionKey, meta = (AllowPrivateAccess = "true"))
	FName ScreenResolutionKey = NAME_None;

	/* 수직 동기화(VSync) 체크박스 상태 */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetVSyncEnabled, Getter = GetVSyncEnabled, meta = (AllowPrivateAccess = "true"))
	ECheckBoxState VSyncEnabled = ECheckBoxState::Unchecked;

	/* 카테고리별 입력 키 조합 설정 배열 */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetKeyMappings, Getter = GetKeyMappings, meta = (AllowPrivateAccess = "true"))
	TArray<FBBMKeyCategoryMappingData> KeyMappings;

	/* 키 설정 카테고리 ListView에 바인딩할 엔트리 ViewModel 배열 */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadOnly, FieldNotify, Setter = SetKeyMappingCategoryEntries, Getter = GetKeyMappingCategoryEntries, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UBBMKeyMappingCategoryEntryViewModel>> KeyMappingCategoryEntries;

	/* 미확정 변경 사항 존재 여부 (Apply 버튼 활성화 바인딩 및 미확정 닫힘 롤백용) */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadOnly, FieldNotify, Getter = GetHasPendingEdit, meta = (AllowPrivateAccess = "true"))
	bool bHasPendingEdit = false;

	/* 언어·창 모드·화면 해상도 선택용 텍스트 콤보박스 서브 뷰모델 3종 */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadOnly, FieldNotify, Getter = GetLanguageComboBoxViewModel, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMComboBoxTextViewModel> LanguageComboBoxViewModel;

	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadOnly, FieldNotify, Getter = GetWindowModeComboBoxViewModel, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMComboBoxTextViewModel> WindowModeComboBoxViewModel;

	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadOnly, FieldNotify, Getter = GetScreenResolutionComboBoxViewModel, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMComboBoxTextViewModel> ScreenResolutionComboBoxViewModel;

	/* 내부 상태 및 대리자 핸들 */
private:
	struct FKeyMappingCategoryEntryBinding
	{
		TWeakObjectPtr<UBBMKeyMappingCategoryEntryViewModel> Entry;
		FDelegateHandle ChordChangedHandle;
	};

	/* 현재 설정 값 */
	FBBMUserSettingsData CachedCurrentSettings;

	/* 바인딩된 GameDataSubsystem 약참조 */
	TWeakObjectPtr<UBBMGameDataSubsystem> CachedGameDataSubsystem;

	/* OnUserSettingsChanged 대리자 등록 핸들 */
	FDelegateHandle UserSettingsChangedHandle;

	/* 콤보박스 서브 뷰모델 SelectedKey 변경 대리자 핸들 */
	FDelegateHandle LanguageComboChangedHandle;
	FDelegateHandle WindowModeComboChangedHandle;
	FDelegateHandle ScreenResolutionComboChangedHandle;

	/* 키 매핑 카테고리 엔트리 VM 대리자 등록 핸들 목록 */
	TArray<FKeyMappingCategoryEntryBinding> KeyMappingCategoryEntryBindings;

	/* 설정 창 진입 시점 스냅샷 (Cancel 또는 미확정 닫힘 시 복원용) */
	FBBMUserSettingsData CachedSettingsBeforeEdit;

	/* 서브시스템 대리자 콜백/푸시 간 재진입 방지 가드 */
	bool bIsSyncingFromSubsystem = false;

	/* 키 매핑 양방향 동기화 재진입 방지 가드 */
	bool bIsSyncingKeyMappings = false;

	/* SelectedKey 양방향 동기화 재진입 방지 가드 */
	bool bIsSyncingComboBoxes = false;
};
