/*****************************************************************//**
 * @file   DISGlobalSubsystem.cpp
 * @brief  DIS 플러그인 전역 런타임 호스팅 서브시스템 구현부
 * @date   2026-09-28
 *********************************************************************/

#include "Global/DISGlobalSubsystem.h"
#include "Global/DISGlobalLogic.h"
#include "Global/DISGlobalSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UDISGlobalSubsystem::UDISGlobalSubsystem() :
	LogicInstance(nullptr)
{
}

void UDISGlobalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UDISGlobalSettings* Settings = GetDefault<UDISGlobalSettings>();
	UClass* LogicClass = (Settings != nullptr) ? Settings->GetGlobalLogicClass() : UDISGlobalLogic::StaticClass();

	// 미지정 / 계층 밖의 클래스 / 추상 클래스인 경우 기본 UDISGlobalLogic으로 안전 폴백
	if (LogicClass == nullptr || LogicClass->IsChildOf(UDISGlobalLogic::StaticClass()) == false || LogicClass->HasAnyClassFlags(CLASS_Abstract) == true)
	{
		LogicClass = UDISGlobalLogic::StaticClass();
	}

	LogicInstance = NewObject<UDISGlobalLogic>(this, LogicClass);
	if (LogicInstance != nullptr)
	{
		LogicInstance->Initialize(this);
	}
}

void UDISGlobalSubsystem::Deinitialize()
{
	if (LogicInstance != nullptr)
	{
		LogicInstance->Deinitialize();
		LogicInstance = nullptr;
	}

	Super::Deinitialize();
}

UDISGlobalLogic* UDISGlobalSubsystem::GetLogic() const
{
	return LogicInstance;
}

UDISGlobalLogic* UDISGlobalSubsystem::GetDISGlobalLogic(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	// 컨텍스트 객체가 이미 UGameInstance인 경우 즉시 서브시스템 조회
	if (const UGameInstance* GI = Cast<UGameInstance>(WorldContextObject))
	{
		UDISGlobalSubsystem* Subsystem = GI->GetSubsystem<UDISGlobalSubsystem>();
		return (Subsystem != nullptr) ? Subsystem->GetLogic() : nullptr;
	}

	if (GEngine == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		return nullptr;
	}

	UDISGlobalSubsystem* Subsystem = GameInstance->GetSubsystem<UDISGlobalSubsystem>();
	return (Subsystem != nullptr) ? Subsystem->GetLogic() : nullptr;
}
