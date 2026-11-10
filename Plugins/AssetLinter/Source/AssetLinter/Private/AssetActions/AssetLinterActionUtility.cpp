#include "AssetActions/AssetLinterActionUtility.h"
#include "AssetLinterSettings.h"
#include "AssetLinterValidator.h"
#include "UI/SAssetLinterBatchFixDialog.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "EditorUtilityLibrary.h"
#include "Misc/MessageDialog.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "AssetLinterActionUtility"

UAssetLinterActionUtility::UAssetLinterActionUtility() : Super()
{
	// 모든 에셋 타입에 대해 우클릭 컨텍스트 메뉴가 노출되도록 기본 UObject를 지원 클래스로 지정
	SupportedClasses.Add(UObject::StaticClass());
}

void UAssetLinterActionUtility::ValidateSelectedAssets()
{
	const TArray<FAssetData> SelectedAssets = UEditorUtilityLibrary::GetSelectedAssetData();
	if (SelectedAssets.Num() == 0)
	{
		return;
	}

	const UAssetLinterSettings* Settings = GetDefault<UAssetLinterSettings>();
	if (Settings == nullptr)
	{
		return;
	}

	int32 TotalChecked = 0;
	int32 ViolationCount = 0;
	FMessageLog MessageLog("AssetValidation");

	for (const FAssetData& AssetData : SelectedAssets)
	{
		UObject* AssetObj = AssetData.GetAsset();
		if (AssetObj == nullptr)
		{
			continue;
		}

		TotalChecked++;

		TArray<const UClass*> CandidateClasses;
		UAssetLinterValidator::GatherCandidateClasses(AssetData, AssetObj, CandidateClasses);
		if (CandidateClasses.Num() == 0)
		{
			continue;
		}

		const FString AssetName = AssetData.AssetName.ToString();
		const FString PackagePath = AssetData.PackagePath.ToString();
		bool bAssetHasViolation = false;

		// 1. 네이밍 룰 검증
		const FAssetNamingRule* NamingRule = UAssetLinterValidator::FindMostDerivedNamingRule(CandidateClasses, Settings);
		if (NamingRule != nullptr)
		{
			const ESearchCase::Type SearchCase = NamingRule->bMatchCase ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;

			if (NamingRule->RequiredPrefix.IsEmpty() == false && AssetName.StartsWith(NamingRule->RequiredPrefix, SearchCase) == false)
			{
				const FText ErrorText = FText::Format(
					LOCTEXT("PrefixFailLog", "[AssetLinter] 에셋 '{0}'이 필수 접두사 '{1}'(으)로 시작하지 않습니다. (요구 클래스: {2})"),
					FText::FromString(AssetName),
					FText::FromString(NamingRule->RequiredPrefix),
					FText::FromString(NamingRule->TargetClass.GetAssetName())
				);
				MessageLog.Error(ErrorText);
				bAssetHasViolation = true;
			}

			if (NamingRule->RequiredSuffix.IsEmpty() == false && AssetName.EndsWith(NamingRule->RequiredSuffix, SearchCase) == false)
			{
				const FText ErrorText = FText::Format(
					LOCTEXT("SuffixFailLog", "[AssetLinter] 에셋 '{0}'이 필수 접미사 '{1}'(으)로 끝나지 않습니다. (요구 클래스: {2})"),
					FText::FromString(AssetName),
					FText::FromString(NamingRule->RequiredSuffix),
					FText::FromString(NamingRule->TargetClass.GetAssetName())
				);
				MessageLog.Error(ErrorText);
				bAssetHasViolation = true;
			}
		}

		// 2. 위치 룰 검증
		const FAssetLocationRule* LocationRule = UAssetLinterValidator::FindMostDerivedLocationRule(CandidateClasses, Settings);
		if (LocationRule != nullptr && LocationRule->AllowedDirectories.Num() > 0)
		{
			if (UAssetLinterValidator::IsInAllowedDirectory(PackagePath, *LocationRule) == false)
			{
				const FText ErrorText = FText::Format(
					LOCTEXT("LocationFailLog", "[AssetLinter] 에셋 '{0}'의 위치 '{1}'이 허용된 디렉터리 하위에 존재하지 않습니다. (요구 클래스: {2})"),
					FText::FromString(AssetName),
					FText::FromString(PackagePath),
					FText::FromString(LocationRule->TargetClass.GetAssetName())
				);
				MessageLog.Error(ErrorText);
				bAssetHasViolation = true;
			}
		}

		if (bAssetHasViolation == true)
		{
			ViolationCount++;
		}
	}

	// 알림 토스트 표시
	FNotificationInfo Info(FText::GetEmpty());
	Info.FadeInDuration = 0.2f;
	Info.FadeOutDuration = 0.5f;
	Info.ExpireDuration = 4.0f;

	if (ViolationCount == 0)
	{
		Info.Text = FText::Format(
			LOCTEXT("ValidationSuccessToast", "선택한 {0}개 에셋이 모두 린트 규격을 준수하고 있습니다."),
			FText::AsNumber(TotalChecked)
		);
		Info.Image = FAppStyle::GetBrush("Icons.SuccessWithColor");
	}
	else
	{
		Info.Text = FText::Format(
			LOCTEXT("ValidationFailToast", "선택한 {0}개 에셋 중 {1}개에서 린트 규칙 위반이 발견되었습니다."),
			FText::AsNumber(TotalChecked),
			FText::AsNumber(ViolationCount)
		);
		Info.Image = FAppStyle::GetBrush("Icons.WarningWithColor");
		MessageLog.Open(EMessageSeverity::Warning, true);
	}

	FSlateNotificationManager::Get().AddNotification(Info);
}

void UAssetLinterActionUtility::BatchFixSelectedAssets()
{
	const TArray<FAssetData> SelectedAssets = UEditorUtilityLibrary::GetSelectedAssetData();
	if (SelectedAssets.Num() == 0)
	{
		return;
	}

	TArray<TSharedPtr<FAssetLintFixItem>> FixItems;
	FText AtomicErrorMessage;

	// 1. 사전 무결성 평가 및 원자적 검증
	const bool bEvaluationPassed = BuildFixItems(SelectedAssets, FixItems, AtomicErrorMessage);
	if (bEvaluationPassed == false)
	{
		// 위치 규칙 미비 등으로 원자적 실패 발생 시 즉시 중단하고 안내 다이얼로그 표시
		FMessageDialog::Open(
			EAppMsgType::Ok,
			AtomicErrorMessage,
			LOCTEXT("AtomicFailureDialogTitle", "Asset Linter - 일괄 수정 실패 (원자적 중단)")
		);
		return;
	}

	// 2. 위반 항목 유무 확인
	if (FixItems.Num() == 0)
	{
		FNotificationInfo Info(LOCTEXT("NoFixNeededToast", "선택한 에셋 중 수정이 필요한 린트 위반 항목이 없습니다."));
		Info.FadeInDuration = 0.2f;
		Info.FadeOutDuration = 0.5f;
		Info.ExpireDuration = 3.0f;
		Info.Image = FAppStyle::GetBrush("Icons.SuccessWithColor");
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// 3. 에디터 현재 테마 기반 확인 팝업 UI 표시
	const bool bUserConfirmed = SAssetLinterBatchFixDialog::OpenDialog(FixItems);
	if (bUserConfirmed == false)
	{
		// 사용자가 취소한 경우 변경 없이 종료
		return;
	}

	// 4. 일괄 수정 실행
	ExecuteBatchFix(FixItems);
}

bool UAssetLinterActionUtility::BuildFixItems(const TArray<FAssetData>& InSelectedAssets, TArray<TSharedPtr<FAssetLintFixItem>>& OutFixItems, FText& OutAtomicErrorMessage)
{
	OutFixItems.Reset();
	OutAtomicErrorMessage = FText::GetEmpty();

	const UAssetLinterSettings* Settings = GetDefault<UAssetLinterSettings>();
	if (Settings == nullptr)
	{
		return true;
	}

	for (const FAssetData& AssetData : InSelectedAssets)
	{
		UObject* AssetObj = AssetData.GetAsset();
		if (AssetObj == nullptr)
		{
			continue;
		}

		TArray<const UClass*> CandidateClasses;
		UAssetLinterValidator::GatherCandidateClasses(AssetData, AssetObj, CandidateClasses);
		if (CandidateClasses.Num() == 0)
		{
			continue;
		}

		const FString CurrentName = AssetData.AssetName.ToString();
		const FString CurrentPath = AssetData.PackagePath.ToString();

		FString ProposedName = CurrentName;
		bool bNeedsRename = false;
		TArray<FString> Violations;

		// 1. 네이밍 보정 산출 (칼같은 처리)
		const FAssetNamingRule* NamingRule = UAssetLinterValidator::FindMostDerivedNamingRule(CandidateClasses, Settings);
		if (NamingRule != nullptr)
		{
			const ESearchCase::Type SearchCase = NamingRule->bMatchCase ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;

			if (NamingRule->RequiredPrefix.IsEmpty() == false && ProposedName.StartsWith(NamingRule->RequiredPrefix, SearchCase) == false)
			{
				ProposedName = NamingRule->RequiredPrefix + ProposedName;
				bNeedsRename = true;
				Violations.Add(FString::Printf(TEXT("접두사 '%s' 누락"), *NamingRule->RequiredPrefix));
			}

			if (NamingRule->RequiredSuffix.IsEmpty() == false && ProposedName.EndsWith(NamingRule->RequiredSuffix, SearchCase) == false)
			{
				ProposedName = ProposedName + NamingRule->RequiredSuffix;
				bNeedsRename = true;
				Violations.Add(FString::Printf(TEXT("접미사 '%s' 누락"), *NamingRule->RequiredSuffix));
			}
		}

		// 2. 위치 보정 산출 및 원자적 검증
		FString ProposedPath = CurrentPath;
		bool bNeedsMove = false;

		const FAssetLocationRule* LocationRule = UAssetLinterValidator::FindMostDerivedLocationRule(CandidateClasses, Settings);
		if (LocationRule != nullptr)
		{
			const bool bInAllowed = UAssetLinterValidator::IsInAllowedDirectory(CurrentPath, *LocationRule);
			if (bInAllowed == false)
			{
				// 위치 이동이 필요한데 AllowedDirectories가 비어 있는 경우 원자적 일괄 실패 처리
				if (LocationRule->AllowedDirectories.Num() == 0 || LocationRule->AllowedDirectories[0].Path.IsEmpty() == true)
				{
					OutAtomicErrorMessage = FText::Format(
						LOCTEXT("AtomicFailureAllowedDirsMissing",
							"일괄 수정 실패 (원자적 중단): 에셋 '{0}'의 위치 규칙(대상 클래스: {1})에 허용 디렉터리(AllowedDirectories)가 등록되어 있지 않아 이동할 대상을 특정할 수 없습니다.\nProject Settings에서 위치 규칙의 허용 디렉터리를 먼저 설정하십시오."),
						FText::FromString(CurrentName),
						FText::FromString(LocationRule->TargetClass.GetAssetName())
					);
					OutFixItems.Reset();
					return false;
				}

				ProposedPath = LocationRule->AllowedDirectories[0].Path;
				if (CurrentPath != ProposedPath)
				{
					bNeedsMove = true;
					Violations.Add(FString::Printf(TEXT("허용 디렉터리 외 위치 ➔ '%s' 이동 필요"), *ProposedPath));
				}
			}
		}

		// 위반 사항이 하나라도 있는 경우 수정 제안 항목으로 등록
		if (bNeedsRename == true || bNeedsMove == true)
		{
			TSharedPtr<FAssetLintFixItem> FixItem = MakeShared<FAssetLintFixItem>();
			FixItem->AssetData = AssetData;
			FixItem->CurrentName = CurrentName;
			FixItem->ProposedName = ProposedName;
			FixItem->bNeedsRename = bNeedsRename;
			FixItem->CurrentPath = CurrentPath;
			FixItem->ProposedPath = ProposedPath;
			FixItem->bNeedsMove = bNeedsMove;
			FixItem->ViolationReason = FText::FromString(FString::Join(Violations, TEXT(", ")));
			FixItem->bIsSelected = true;

			OutFixItems.Add(FixItem);
		}
	}

	return true;
}

void UAssetLinterActionUtility::ExecuteBatchFix(const TArray<TSharedPtr<FAssetLintFixItem>>& InFixItems)
{
	TArray<FAssetRenameData> RenameDataList;

	for (const TSharedPtr<FAssetLintFixItem>& Item : InFixItems)
	{
		if (Item.IsValid() == false || Item->bIsSelected == false)
		{
			continue;
		}

		UObject* AssetObj = Item->AssetData.GetAsset();
		if (AssetObj == nullptr)
		{
			continue;
		}

		const FString TargetPath = Item->bNeedsMove ? Item->ProposedPath : Item->CurrentPath;
		const FString TargetName = Item->bNeedsRename ? Item->ProposedName : Item->CurrentName;

		RenameDataList.Emplace(AssetObj, TargetPath, TargetName);
	}

	if (RenameDataList.Num() == 0)
	{
		FNotificationInfo Info(LOCTEXT("NoItemSelectedToast", "선택된 수정 항목이 없습니다."));
		Info.ExpireDuration = 2.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// 언리얼 엔진 공식 IAssetTools를 통해 안전한 일괄 리네임 및 이동 실행
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const bool bSuccess = AssetTools.RenameAssets(RenameDataList);

	FNotificationInfo Info(FText::GetEmpty());
	Info.FadeInDuration = 0.2f;
	Info.FadeOutDuration = 0.5f;
	Info.ExpireDuration = 4.0f;

	if (bSuccess == true)
	{
		Info.Text = FText::Format(
			LOCTEXT("BatchFixSuccessToast", "총 {0}개 에셋의 린트 일괄 보정(리네임 및 폴더 이동)이 성공적으로 완료되었습니다."),
			FText::AsNumber(RenameDataList.Num())
		);
		Info.Image = FAppStyle::GetBrush("Icons.SuccessWithColor");
	}
	else
	{
		Info.Text = LOCTEXT("BatchFixFailToast", "일부 에셋의 일괄 보정 중 오류가 발생했습니다. 로그를 확인하십시오.");
		Info.Image = FAppStyle::GetBrush("Icons.ErrorWithColor");
	}

	FSlateNotificationManager::Get().AddNotification(Info);
}

#undef LOCTEXT_NAMESPACE

