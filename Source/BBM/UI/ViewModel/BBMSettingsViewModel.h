/*****************************************************************//**
 * @file   BBMSettingsViewModel.h
 * @brief  BBM(비트 배낭 상인) 환경설정 화면 MVVM 뷰모델 클래스 정의
 * @details GameDataSubsystem과의 대리자 동기화, 즉각 런타임 적용, Apply 확정 저장 및 Cancel/닫힘 시 이전 설정 롤백 처리를 전담함
 * @date   2026-11-13
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/BBMTabViewModelBase.h"
#include "Save/BBMSaveTypes.h"
#include "BBMSettingsViewModel.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMGameDataSubsystem;
class UBBMKeyMappingCategoryEntryViewModel;

/**
 * 환경 설정 패널 ViewModel
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Settings ViewModel"))
class BBM_API UBBMSettingsViewModel : public UBBMTabViewModelBase
{
	GENERATED_BODY()

	/* 수명 주기 */
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

	/* 박자 시각 보조 활성화 여부 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	bool GetEnableBeatVisualAssist() const;

	/* 박자 시각 보조 활성화 여부 설정 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetEnableBeatVisualAssist(bool bInEnableBeatVisualAssist);

	/* 창 모드 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	EWindowMode::Type GetWindowMode() const;

	/* 창 모드 설정 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetWindowMode(EWindowMode::Type InWindowMode);

	/* 화면 해상도 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	FIntPoint GetScreenResolution() const;

	/* 화면 해상도 설정 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetScreenResolution(FIntPoint InScreenResolution);

	/* 수직 동기화(VSync) 활성화 여부 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	bool GetVSyncEnabled() const;

	/* 수직 동기화(VSync) 활성화 여부 설정 및 즉각 런타임 적용 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintCallable)
	void SetVSyncEnabled(bool bInVSyncEnabled);

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
	void RefreshKeyMappingCategoryEntries();

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

	/* 키 설정 헬퍼 */
private:
	/* 키 맵 카테고리 배열을 새 목록으로 교체하고 대리자 재바인딩을 수행함 */
	void SetKeyMappingCategoryEntries(const TArray<TObjectPtr<UBBMKeyMappingCategoryEntryViewModel>>& InKeyMappingCategoryEntries);

	/* 모든 키 맵 카테고리를 제거 */
	void RemoveAllKeyMappingCategoryEntries();

	/* 모든 KeyMappingCategoryEntries 카테고리 VM의 커스텀 대리자에 바인딩함 */
	void RebindKeyMappingCategoryEntries();

	/* 모든 KeyMappingCategoryEntries 카테고리 VM의 커스텀 대리자 바인딩을 해제함 */
	void UnbindKeyMappingCategoryEntries();

	/* 카테고리 엔트리 VM InputChord 변경 시 RemapKey로 위임하여 상태를 일관 동기화함 */
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

	/* 박자 시각 보조 활성화 여부 */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetEnableBeatVisualAssist, Getter = GetEnableBeatVisualAssist, meta = (AllowPrivateAccess = "true"))
	bool bEnableBeatVisualAssist = true;

	/* 창 모드 */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetWindowMode, Getter = GetWindowMode, meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<EWindowMode::Type> WindowMode = EWindowMode::WindowedFullscreen;

	/* 화면 해상도 */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetScreenResolution, Getter = GetScreenResolution, meta = (AllowPrivateAccess = "true"))
	FIntPoint ScreenResolution = FIntPoint(1920, 1080);

	/* 수직 동기화(VSync) 활성화 여부 */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetVSyncEnabled, Getter = GetVSyncEnabled, meta = (AllowPrivateAccess = "true"))
	bool bVSyncEnabled = false;

	/* 카테고리별 입력 키 조합 설정 배열 */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadWrite, FieldNotify, Setter = SetKeyMappings, Getter = GetKeyMappings, meta = (AllowPrivateAccess = "true"))
	TArray<FBBMKeyCategoryMappingData> KeyMappings;

	/* 키 설정 카테고리 ListView에 바인딩할 엔트리 ViewModel 배열 */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadOnly, FieldNotify, Setter = SetKeyMappingCategoryEntries, Getter = GetKeyMappingCategoryEntries, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UBBMKeyMappingCategoryEntryViewModel>> KeyMappingCategoryEntries;

	/* 미확정 변경 사항 존재 여부 (Apply 버튼 활성화 바인딩 및 미확정 닫힘 롤백용) */
	UPROPERTY(Category = "BBM|UI|Settings", BlueprintReadOnly, FieldNotify, Getter = GetHasPendingEdit, meta = (AllowPrivateAccess = "true"))
	bool bHasPendingEdit = false;

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

	/* 키 매핑 카테고리 엔트리 VM 대리자 등록 핸들 목록 */
	TArray<FKeyMappingCategoryEntryBinding> KeyMappingCategoryEntryBindings;

	/* 설정 창 진입 시점 스냅샷 (Cancel 또는 미확정 닫힘 시 복원용) */
	FBBMUserSettingsData CachedSettingsBeforeEdit;

	/* 서브시스템 대리자 콜백/푸시 간 재진입 방지 가드 */
	bool bIsSyncingFromSubsystem = false;
};
