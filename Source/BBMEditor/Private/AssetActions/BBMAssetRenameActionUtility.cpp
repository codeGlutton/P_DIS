#include "AssetActions/BBMAssetRenameActionUtility.h"
#include "EditorUtilityLibrary.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"

UBBMAssetRenameActionUtility::UBBMAssetRenameActionUtility() :
	Super()
{
	// 모든 에셋 타입에 대해 공통으로 컨텍스트 메뉴가 노출되도록 기본 UObject를 지원 클래스로 설정
	SupportedClasses.Add(UObject::StaticClass());
}

void UBBMAssetRenameActionUtility::AddPrefixToAssets(const FString& Prefix)
{
	// 접두사가 비어 있는 경우 조기 반환
	if (Prefix.IsEmpty() == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BBM Asset Tools] 접두사(Prefix)가 비어 있어 리네임 작업을 중단함."));
		return;
	}

	// 람다 변환기를 통해 기존 이름 앞에 Prefix 추가
	ExecuteBatchRename(
		TEXT("Add Prefix"),
		[&Prefix](const FString& OldName) -> FString
		{
			// 이미 해당 접두사로 시작하는 경우 중복 추가 방지
			if (OldName.StartsWith(Prefix) == true)
			{
				return OldName;
			}
			return Prefix + OldName;
		}
	);
}

void UBBMAssetRenameActionUtility::AddSuffixToAssets(const FString& Suffix)
{
	// 접미사가 비어 있는 경우 조기 반환
	if (Suffix.IsEmpty() == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BBM Asset Tools] 접미사(Suffix)가 비어 있어 리네임 작업을 중단함."));
		return;
	}

	// 람다 변환기를 통해 기존 이름 뒤에 Suffix 추가
	ExecuteBatchRename(
		TEXT("Add Suffix"),
		[&Suffix](const FString& OldName) -> FString
		{
			// 이미 해당 접미사로 끝나는 경우 중복 추가 방지
			if (OldName.EndsWith(Suffix) == true)
			{
				return OldName;
			}
			return OldName + Suffix;
		}
	);
}

void UBBMAssetRenameActionUtility::ReplaceCharactersInAssets(const FString& SearchText, const FString& ReplacementText, bool bIgnoreCase)
{
	// 검색 대상 문자열이 비어 있는 경우 조기 반환
	if (SearchText.IsEmpty() == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BBM Asset Tools] 검색할 문자열(SearchText)이 비어 있어 리네임 작업을 중단함."));
		return;
	}

	const ESearchCase::Type SearchCase = (bIgnoreCase == true) ? ESearchCase::IgnoreCase : ESearchCase::CaseSensitive;

	// 람다 변환기를 통해 검색 문자열 치환
	ExecuteBatchRename(
		TEXT("Replace Characters"),
		[&SearchText, &ReplacementText, SearchCase](const FString& OldName) -> FString
		{
			if (OldName.Contains(SearchText, SearchCase) == true)
			{
				return OldName.Replace(*SearchText, *ReplacementText, SearchCase);
			}
			return OldName;
		}
	);
}

void UBBMAssetRenameActionUtility::ExecuteBatchRename(const FString& ActionName, TFunctionRef<FString(const FString&)> NameTransformFunc)
{
	// 에디터 콘텐츠 브라우저에서 현재 선택된 에셋 목록 조회
	const TArray<UObject*> SelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();
	if (SelectedAssets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BBM Asset Tools] [%s] 선택된 에셋이 없어 작업을 수행할 수 없음."), *ActionName);
		return;
	}

	// 일괄 리네임 대상 데이터 컨테이너 준비
	TArray<FAssetRenameData> RenameDataList;
	RenameDataList.Reserve(SelectedAssets.Num());

	for (UObject* TargetAsset : SelectedAssets)
	{
		// 유효하지 않은 포인터 방어
		if (TargetAsset == nullptr)
		{
			continue;
		}

		const FString OldName = TargetAsset->GetName();
		const FString NewName = NameTransformFunc(OldName);

		// 이름 변동이 없거나 빈 문자열인 경우 스킵
		if (NewName == OldName || NewName.IsEmpty() == true)
		{
			continue;
		}

		// 에셋의 부모 패키지 경로 추출
		UPackage* AssetPackage = TargetAsset->GetOutermost();
		if (AssetPackage == nullptr)
		{
			continue;
		}

		const FString PackagePath = FPackageName::GetLongPackagePath(AssetPackage->GetName());
		RenameDataList.Emplace(TargetAsset, PackagePath, NewName);
	}

	// 실제 이름 변경 대상이 없는 경우 조기 반환
	if (RenameDataList.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[BBM Asset Tools] [%s] 이름 변경이 필요한 에셋이 없음 (모두 동일하거나 대상 문자열 미포함)."), *ActionName);
		return;
	}

	// AssetTools 모듈을 통해 일괄 리네임 실행
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	const bool bRenameSuccess = AssetToolsModule.Get().RenameAssets(RenameDataList);
	if (bRenameSuccess == true)
	{
		UE_LOG(LogTemp, Log, TEXT("[BBM Asset Tools] [%s] %d개 에셋 일괄 리네임 즉시 적용 성공."), *ActionName, RenameDataList.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[BBM Asset Tools] [%s] 에셋 일괄 리네임 처리 중 오류 발생."), *ActionName);
	}
}

