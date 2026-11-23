#include "AssetLinterValidator.h"
#include "AssetLinterSettings.h"
#include "Engine/Blueprint.h"
#include "Misc/DataValidation.h"
#include "UObject/Interface.h"

#define LOCTEXT_NAMESPACE "AssetLinterValidator"

UAssetLinterValidator::UAssetLinterValidator()
{
	bIsEnabled = true;
}

bool UAssetLinterValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	const UAssetLinterSettings* Settings = GetDefault<UAssetLinterSettings>();
	if (Settings == nullptr)
	{
		return false;
	}

	// 1. 검사 대상 스코프가 하나도 정의되어 있지 않으면 검사하지 않음
	if (Settings->TargetScopes.Num() == 0)
	{
		return false;
	}

	const FString PackagePath = InAssetData.PackagePath.ToString();

	// 2. 제외 대상 디렉터리(Blacklist) 확인
	for (const FDirectoryPath& ExcludedDir : Settings->ExcludedDirectories)
	{
		const FString& ExcludedPath = ExcludedDir.Path;
		if (ExcludedPath.IsEmpty() == true)
		{
			continue;
		}

		if (PackagePath == ExcludedPath || PackagePath.StartsWith(ExcludedPath + TEXT("/")))
		{
			return false;
		}
	}

	// 3. 대상 스코프(Whitelist) 일치 여부 확인
	for (const FAssetLinterTargetScope& Scope : Settings->TargetScopes)
	{
		const FString& ScopePath = Scope.TargetDirectory.Path;
		if (ScopePath.IsEmpty() == true)
		{
			continue;
		}

		if (Scope.bIncludeSubdirectories == true)
		{
			if (PackagePath == ScopePath || PackagePath.StartsWith(ScopePath + TEXT("/")))
			{
				return true;
			}
		}
		else
		{
			if (PackagePath == ScopePath)
			{
				return true;
			}
		}
	}

	return false;
}

EDataValidationResult UAssetLinterValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	if (InAsset == nullptr)
	{
		return EDataValidationResult::NotValidated;
	}

	const UAssetLinterSettings* Settings = GetDefault<UAssetLinterSettings>();
	if (Settings == nullptr)
	{
		return EDataValidationResult::NotValidated;
	}

	// 1. 에셋의 클래스 후보군 수집 (일반 클래스 및 블루프린트 생성/부모 클래스 포함)
	TArray<const UClass*> CandidateClasses;
	GatherCandidateClasses(InAssetData, InAsset, CandidateClasses);

	if (CandidateClasses.Num() == 0)
	{
		AssetPasses(InAsset);
		return EDataValidationResult::Valid;
	}

	bool bHasViolation = false;
	const FString AssetName = InAsset->GetName();
	const FString PackagePath = InAssetData.PackagePath.ToString();

	// 2. Most-Derived 네이밍 규칙 검사
	const FAssetNamingRule* NamingRule = FindMostDerivedNamingRule(CandidateClasses, Settings);
	if (NamingRule != nullptr)
	{
		const ESearchCase::Type SearchCase = NamingRule->bMatchCase ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;

		// 2-1. Prefix 검사
		if (NamingRule->RequiredPrefix.IsEmpty() == false)
		{
			if (AssetName.StartsWith(NamingRule->RequiredPrefix, SearchCase) == false)
			{
				const FText ErrorMsg = FText::Format(
					LOCTEXT("PrefixViolation", "에셋 명칭 '{0}'이 필수 접두사 '{1}'(으)로 시작하지 않습니다. (요구 클래스: {2})"),
					FText::FromString(AssetName),
					FText::FromString(NamingRule->RequiredPrefix),
					FText::FromString(NamingRule->TargetClass.GetAssetName())
				);

				if (Settings->bTreatAsWarning == true)
				{
					AssetWarning(InAsset, ErrorMsg);
				}
				else
				{
					AssetFails(InAsset, ErrorMsg);
				}
				bHasViolation = true;
			}
		}

		// 2-2. Suffix 검사
		if (NamingRule->RequiredSuffix.IsEmpty() == false)
		{
			if (AssetName.EndsWith(NamingRule->RequiredSuffix, SearchCase) == false)
			{
				const FText ErrorMsg = FText::Format(
					LOCTEXT("SuffixViolation", "에셋 명칭 '{0}'이 필수 접미사 '{1}'(으)로 끝나지 않습니다. (요구 클래스: {2})"),
					FText::FromString(AssetName),
					FText::FromString(NamingRule->RequiredSuffix),
					FText::FromString(NamingRule->TargetClass.GetAssetName())
				);

				if (Settings->bTreatAsWarning == true)
				{
					AssetWarning(InAsset, ErrorMsg);
				}
				else
				{
					AssetFails(InAsset, ErrorMsg);
				}
				bHasViolation = true;
			}
		}
	}

	// 3. Most-Derived 위치 규칙 검사
	const FAssetLocationRule* LocationRule = FindMostDerivedLocationRule(CandidateClasses, Settings);
	if (LocationRule != nullptr && LocationRule->AllowedDirectories.Num() > 0)
	{
		if (IsInAllowedDirectory(PackagePath, *LocationRule) == false)
		{
			TArray<FString> AllowedPathStrings;
			for (const FDirectoryPath& Dir : LocationRule->AllowedDirectories)
			{
				if (Dir.Path.IsEmpty() == false)
				{
					AllowedPathStrings.Add(Dir.Path);
				}
			}

			const FString CombinedAllowedPaths = FString::Join(AllowedPathStrings, TEXT(", "));
			const FText ErrorMsg = FText::Format(
				LOCTEXT("LocationViolation", "에셋 위치 '{0}'이 허용된 디렉터리 [{1}] 하위에 존재하지 않습니다. (요구 클래스: {2})"),
				FText::FromString(PackagePath),
				FText::FromString(CombinedAllowedPaths),
				FText::FromString(LocationRule->TargetClass.GetAssetName())
			);

			if (Settings->bTreatAsWarning == true)
			{
				AssetWarning(InAsset, ErrorMsg);
			}
			else
			{
				AssetFails(InAsset, ErrorMsg);
			}
			bHasViolation = true;
		}
	}

	// 4. 위반 사항이 없으면 성공 처리
	if (bHasViolation == false)
	{
		AssetPasses(InAsset);
		return EDataValidationResult::Valid;
	}

	return Settings->bTreatAsWarning ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}

int32 UAssetLinterValidator::CalculateClassDistance(const UClass* InAssetClass, const UClass* InRuleClass)
{
	if (InAssetClass == nullptr || InRuleClass == nullptr)
	{
		return -1;
	}

	if (InAssetClass == InRuleClass)
	{
		return 0;
	}

	if (InAssetClass->IsChildOf(InRuleClass) == false)
	{
		return -1;
	}

	int32 Distance = 0;
	for (const UClass* Current = InAssetClass; Current != nullptr; Current = Current->GetSuperClass())
	{
		if (Current == InRuleClass)
		{
			return Distance;
		}
		Distance++;
	}

	return -1;
}

void UAssetLinterValidator::GatherCandidateClasses(const FAssetData& InAssetData, const UObject* InAsset, TArray<const UClass*>& OutClasses)
{
	OutClasses.Reset();

	if (InAsset == nullptr)
	{
		return;
	}

	// 블루프린트인 경우 논리적 부모 및 생성 클래스 추가
	if (const UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		// 블루프린트 인터페이스인 경우 UInterface를 최우선 후보로 추가
		if (Blueprint->BlueprintType == BPTYPE_Interface)
		{
			OutClasses.AddUnique(UInterface::StaticClass());
		}
		else
		{
			if (Blueprint->GeneratedClass != nullptr)
			{
				OutClasses.AddUnique(Blueprint->GeneratedClass);
			}

			if (Blueprint->ParentClass != nullptr)
			{
				OutClasses.AddUnique(Blueprint->ParentClass);
			}

			// 블루프린트 에셋 자체 클래스 (예: UWidgetBlueprint, UAnimBlueprint, UBlueprint 등) 추가
			OutClasses.AddUnique(Blueprint->GetClass());
		}
	}
	else
	{
		// 일반 C++ 에셋의 경우 해당 객체의 런타임 클래스 추가
		OutClasses.AddUnique(InAsset->GetClass());
	}
}

const FAssetNamingRule* UAssetLinterValidator::FindMostDerivedNamingRule(const TArray<const UClass*>& InCandidateClasses, const UAssetLinterSettings* InSettings)
{
	if (InSettings == nullptr)
	{
		return nullptr;
	}

	const FAssetNamingRule* BestRule = nullptr;
	int32 MinDistance = MAX_int32;

	for (const FAssetNamingRule& Rule : InSettings->NamingRules)
	{
		const UClass* RuleClass = Rule.TargetClass.Get();
		if (RuleClass == nullptr)
		{
			RuleClass = Rule.TargetClass.LoadSynchronous();
		}

		if (RuleClass == nullptr)
		{
			continue;
		}

		for (const UClass* CandidateClass : InCandidateClasses)
		{
			if (CandidateClass == nullptr)
			{
				continue;
			}

			const int32 Distance = CalculateClassDistance(CandidateClass, RuleClass);
			if (Distance >= 0 && Distance < MinDistance)
			{
				MinDistance = Distance;
				BestRule = &Rule;
			}
		}
	}

	return BestRule;
}

const FAssetLocationRule* UAssetLinterValidator::FindMostDerivedLocationRule(const TArray<const UClass*>& InCandidateClasses, const UAssetLinterSettings* InSettings)
{
	if (InSettings == nullptr)
	{
		return nullptr;
	}

	const FAssetLocationRule* BestRule = nullptr;
	int32 MinDistance = MAX_int32;

	for (const FAssetLocationRule& Rule : InSettings->LocationRules)
	{
		const UClass* RuleClass = Rule.TargetClass.Get();
		if (RuleClass == nullptr)
		{
			RuleClass = Rule.TargetClass.LoadSynchronous();
		}

		if (RuleClass == nullptr)
		{
			continue;
		}

		for (const UClass* CandidateClass : InCandidateClasses)
		{
			if (CandidateClass == nullptr)
			{
				continue;
			}

			const int32 Distance = CalculateClassDistance(CandidateClass, RuleClass);
			if (Distance >= 0 && Distance < MinDistance)
			{
				MinDistance = Distance;
				BestRule = &Rule;
			}
		}
	}

	return BestRule;
}

bool UAssetLinterValidator::IsInAllowedDirectory(const FString& InPackagePath, const FAssetLocationRule& InRule)
{
	for (const FDirectoryPath& AllowedDir : InRule.AllowedDirectories)
	{
		const FString& AllowedPath = AllowedDir.Path;
		if (AllowedPath.IsEmpty() == true)
		{
			continue;
		}

		if (InRule.bAllowSubdirectories == true)
		{
			if (InPackagePath == AllowedPath || InPackagePath.StartsWith(AllowedPath + TEXT("/")))
			{
				return true;
			}
		}
		else
		{
			if (InPackagePath == AllowedPath)
			{
				return true;
			}
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
