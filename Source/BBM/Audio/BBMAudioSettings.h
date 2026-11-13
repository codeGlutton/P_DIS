/*****************************************************************//**
 * @file   BBMAudioSettings.h
 * @brief  BBM 오디오 사운드 믹스 및 채널별 사운드 클래스 전역 개발자 환경설정 정의
 * @details Project Settings를 통해 옵션 볼륨 제어를 위한 USoundMix 및 채널별 USoundClass 에셋 참조를 관리함
 * @date   2026-11-13
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BBMAudioSettings.generated.h"

/* 독립 전방 선언 */
class USoundMix;
class USoundClass;

/**
 * BBM 오디오 믹스 및 채널별 사운드 클래스 매핑 설정 클래스
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "BBM Audio Settings"))
class BBM_API UBBMAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMAudioSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/* 오디오 에셋 참조 프로퍼티 */
public:
	/* 옵션 볼륨 오버라이드 적용 대상 전역 사운드 믹스 */
	UPROPERTY(Category = "Audio", Config, EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USoundMix> UserSettingsSoundMix;

	/* 전체 마스터 볼륨 제어용 루트 사운드 클래스 */
	UPROPERTY(Category = "Audio", Config, EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	/* 배경음악(BGM) 볼륨 제어용 사운드 클래스 */
	UPROPERTY(Category = "Audio", Config, EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USoundClass> BGMSoundClass;

	/* 효과음(SFX) 볼륨 제어용 사운드 클래스 */
	UPROPERTY(Category = "Audio", Config, EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USoundClass> SFXSoundClass;

	/* 다이얼로그/음성 볼륨 제어용 사운드 클래스 */
	UPROPERTY(Category = "Audio", Config, EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USoundClass> DialogueSoundClass;
};

