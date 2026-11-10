/*****************************************************************//**
 * @file   BBMWorldSettings.cpp
 * @brief  BBM(비트 배낭 상인) 커스텀 월드 세팅 클래스 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Level/BBMWorldSettings.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

ABBMWorldSettings::ABBMWorldSettings()
{
}

FName ABBMWorldSettings::GetRandomRoomSpawnSettingName(const FRandomStream& Stream) const
{
	if (SpawnSettings.Num() == 0)
	{
		return NAME_None;
	}

	TArray<FName> Keys;
	SpawnSettings.GetKeys(Keys);

	const int32 RandomIndex = Stream.RandRange(0, Keys.Num() - 1);
	return Keys[RandomIndex];
}

AActor* ABBMWorldSettings::GetMainCameraPoint(const FName& InSettingName) const
{
	const FBBMRoomSpawnSetting* Found = FindSpawnSetting(InSettingName);
	if (Found != nullptr)
	{
		return Found->MainCameraPoint.Get();
	}
	return nullptr;
}

AActor* ABBMWorldSettings::GetRoomStartPoint(const FName& InSettingName) const
{
	const FBBMRoomSpawnSetting* Found = FindSpawnSetting(InSettingName);
	if (Found != nullptr)
	{
		return Found->RoomStartPoint.Get();
	}
	return nullptr;
}

const FBBMRoomSpawnSetting* ABBMWorldSettings::FindSpawnSetting(const FName& InSettingName) const
{
	return SpawnSettings.Find(InSettingName);
}

bool ABBMWorldSettings::HasSpawnSetting(const FName& InSettingName) const
{
	return SpawnSettings.Contains(InSettingName);
}

TArray<FName> ABBMWorldSettings::GetAllSpawnSettingNames() const
{
	TArray<FName> Keys;
	SpawnSettings.GetKeys(Keys);
	return Keys;
}

void ABBMWorldSettings::SetSpawnSetting(const FName& InSettingName, const FBBMRoomSpawnSetting& InSetting)
{
	if (InSettingName.IsNone() == false)
	{
		SpawnSettings.Add(InSettingName, InSetting);
	}
}

ABBMWorldSettings* ABBMWorldSettings::GetBBMWorldSettings(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (World != nullptr)
	{
		return Cast<ABBMWorldSettings>(World->GetWorldSettings());
	}
	return nullptr;
}

