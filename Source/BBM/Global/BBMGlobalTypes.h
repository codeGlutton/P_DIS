/*****************************************************************//**
 * @file   BBMGlobalTypes.h
 * @brief  BBM 전역 환경설정 및 옵션 선택 테이블용 공용 구조체 정의
 * @details 공통 부모 FBBMSelectionDataBase(Key·DisplayName·키/값 비교 인터페이스)와
 *          실제 값(FString·EWindowMode::Type·FIntPoint)을 보관하는 파생 SelectionData 구조체들을 선언함
 * @date   2026-11-18
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericWindow.h"
#include "BBMGlobalTypes.generated.h"

/**
 * 옵션 식별 키(Key) 및 UI 표기 텍스트(DisplayName)를 공통 보유하는 SelectionData 부모 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMSelectionDataBase
{
	GENERATED_BODY()

	/* 생성자 및 가상 소멸자 */
public:
	FBBMSelectionDataBase() = default;
	FBBMSelectionDataBase(const FBBMSelectionDataBase&) = default;
	FBBMSelectionDataBase(FBBMSelectionDataBase&&) = default;
	FBBMSelectionDataBase& operator=(const FBBMSelectionDataBase&) = default;
	FBBMSelectionDataBase& operator=(FBBMSelectionDataBase&&) = default;
	virtual ~FBBMSelectionDataBase() = default;

	/* 키·값 비교 인터페이스 */
public:
	/* 옵션 식별 키 일치 여부 반환 (NAME_None 입력 시 항상 false) */
	bool IsSameKey(FName InKey) const
	{
		return InKey.IsNone() == false && Key == InKey;
	}

	/* 런타임 리플렉션 타입 식별용 UScriptStruct 반환 (파생 구조체에서 오버라이드) */
	virtual const UScriptStruct* GetSelectionStruct() const
	{
		return StaticStruct();
	}

	/* 파생 구조체의 실제 값 일치 여부 비교 (파생 구조체에서 오버라이드) */
	virtual bool IsSameValue(const FBBMSelectionDataBase& InOther) const
	{
		return false;
	}

	/* 공통 프로퍼티 */
public:
	/* 옵션 식별 키 */
	UPROPERTY(Category = "Selection", EditAnywhere, BlueprintReadWrite)
	FName Key = NAME_None;

	/* UI 표시용 텍스트 */
	UPROPERTY(Category = "Selection", EditAnywhere, BlueprintReadWrite)
	FText DisplayName;
};

/**
 * 문자열(FString) 실제 값을 보관하는 범용 선택 옵션 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMStringSelectionData : public FBBMSelectionDataBase
{
	GENERATED_BODY()

	/* 생성자 및 가상 소멸자 */
public:
	FBBMStringSelectionData() = default;
	virtual ~FBBMStringSelectionData() override = default;

	/* FBBMSelectionDataBase 가상 함수 구현 */
public:
	virtual const UScriptStruct* GetSelectionStruct() const override
	{
		return StaticStruct();
	}

	virtual bool IsSameValue(const FBBMSelectionDataBase& InOther) const override
	{
		if (InOther.GetSelectionStruct() != StaticStruct())
		{
			return false;
		}

		const FBBMStringSelectionData& OtherData = static_cast<const FBBMStringSelectionData&>(InOther);
		return Value.Equals(OtherData.Value, ESearchCase::IgnoreCase) == true;
	}

	/* 프로퍼티 */
public:
	/* 실제 적용 문자열 값 */
	UPROPERTY(Category = "Selection", EditAnywhere, BlueprintReadWrite)
	FString Value;
};

/**
 * 창 모드(EWindowMode::Type) 실제 값을 보관하는 범용 선택 옵션 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMWindowModeSelectionData : public FBBMSelectionDataBase
{
	GENERATED_BODY()

	/* 생성자 및 가상 소멸자 */
public:
	FBBMWindowModeSelectionData() = default;
	virtual ~FBBMWindowModeSelectionData() override = default;

	/* FBBMSelectionDataBase 가상 함수 구현 */
public:
	virtual const UScriptStruct* GetSelectionStruct() const override
	{
		return StaticStruct();
	}

	virtual bool IsSameValue(const FBBMSelectionDataBase& InOther) const override
	{
		if (InOther.GetSelectionStruct() != StaticStruct())
		{
			return false;
		}

		const FBBMWindowModeSelectionData& OtherData = static_cast<const FBBMWindowModeSelectionData&>(InOther);
		return Value == OtherData.Value;
	}

	/* 프로퍼티 */
public:
	/* 실제 적용 창 모드 값 */
	UPROPERTY(Category = "Selection", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EWindowMode::Type> Value = EWindowMode::WindowedFullscreen;
};

/**
 * 2D 정수(FIntPoint) 실제 값을 보관하는 범용 선택 옵션 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMIntPointSelectionData : public FBBMSelectionDataBase
{
	GENERATED_BODY()

	/* 생성자 및 가상 소멸자 */
public:
	FBBMIntPointSelectionData() = default;
	virtual ~FBBMIntPointSelectionData() override = default;

	/* FBBMSelectionDataBase 가상 함수 구현 */
public:
	virtual const UScriptStruct* GetSelectionStruct() const override
	{
		return StaticStruct();
	}

	virtual bool IsSameValue(const FBBMSelectionDataBase& InOther) const override
	{
		if (InOther.GetSelectionStruct() != StaticStruct())
		{
			return false;
		}

		const FBBMIntPointSelectionData& OtherData = static_cast<const FBBMIntPointSelectionData&>(InOther);
		return Value == OtherData.Value;
	}

	/* 프로퍼티 */
public:
	/* 실제 적용 2D 정수 값 */
	UPROPERTY(Category = "Selection", EditAnywhere, BlueprintReadWrite)
	FIntPoint Value = FIntPoint(1920, 1080);
};
