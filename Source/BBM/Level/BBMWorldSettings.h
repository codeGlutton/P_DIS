/*****************************************************************//**
 * @file   BBMWorldSettings.h
 * @brief  BBM(비트 배낭 상인) 커스텀 월드 세팅 클래스 정의
 * @details 레벨 내 룸/만남별 메인 카메라 포인트 및 플레이어 시작 스폰 포인트를 Key-Value로 관리함
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "Level/BBMSpawnTypes.h"
#include "BBMWorldSettings.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class AActor;

/**
 * BBM 전용 월드 세팅 클래스
 * 레벨 내 다양한 만남(Room)별 메인 카메라 포인트 및 플레이어 시작 포인트를 관리하고 질의 기능을 제공함
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM World Settings"))
class BBM_API ABBMWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMWorldSettings();

	/* 스폰 세팅 질의 API */
public:
	/**
	 * 등록된 스폰 설정 중 난수 스트림 기반으로 무작위 Key 이름 반환
	 * @param Stream 난수 추출에 사용할 랜덤 스트림
	 * @return 선정된 스폰 세팅 Key (비어있는 경우 NAME_None)
	 */
	UFUNCTION(Category = "BBM|Spawn", BlueprintCallable, BlueprintPure)
	FName GetRandomRoomSpawnSettingName(const FRandomStream& Stream) const;

	/* 특정 Key에 해당하는 메인 카메라 액터 반환 */
	UFUNCTION(Category = "BBM|Spawn", BlueprintCallable, BlueprintPure)
	AActor* GetMainCameraPoint(const FName& InSettingName) const;

	/* 특정 Key에 해당하는 룸 시작 기준 포인트 액터 반환 */
	UFUNCTION(Category = "BBM|Spawn", BlueprintCallable, BlueprintPure)
	AActor* GetRoomStartPoint(const FName& InSettingName) const;

	/* 특정 Key에 해당하는 스폰 설정 구조체 포인터 반환 */
	const FBBMRoomSpawnSetting* FindSpawnSetting(const FName& InSettingName) const;

	/* 특정 Key의 스폰 설정 존재 여부 확인 */
	UFUNCTION(Category = "BBM|Spawn", BlueprintCallable, BlueprintPure)
	bool HasSpawnSetting(const FName& InSettingName) const;

	/* 등록된 모든 스폰 세팅 Key 목록 반환 */
	UFUNCTION(Category = "BBM|Spawn", BlueprintCallable, BlueprintPure)
	TArray<FName> GetAllSpawnSettingNames() const;

	/* 특정 Key에 스폰 설정 등록 또는 갱신 (에디터/테스트/런타임 지원) */
	UFUNCTION(Category = "BBM|Spawn", BlueprintCallable)
	void SetSpawnSetting(const FName& InSettingName, const FBBMRoomSpawnSetting& InSetting);

	/* 정적 헬퍼: 현재 UWorld로부터 ABBMWorldSettings 획득 */
	UFUNCTION(Category = "BBM|Spawn", BlueprintCallable, BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static ABBMWorldSettings* GetBBMWorldSettings(const UObject* WorldContextObject);

	/* 멤버 프로퍼티 */
protected:
	/* 룸/만남 식별자 Key별 스폰 및 카메라 세팅 매핑 */
	UPROPERTY(Category = "BBM|Spawn", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SpawnSettings"))
	TMap<FName, FBBMRoomSpawnSetting> SpawnSettings;
};

