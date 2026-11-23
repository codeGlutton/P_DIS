#include "AssetLinterSettings.h"

#define LOCTEXT_NAMESPACE "AssetLinterSettings"

UAssetLinterSettings::UAssetLinterSettings()
	: bTreatAsWarning(false)
{
	// 기본 제외 디렉터리 등록
	FDirectoryPath EnginePath;
	EnginePath.Path = TEXT("/Engine");
	ExcludedDirectories.Add(EnginePath);

	FDirectoryPath DevPath;
	DevPath.Path = TEXT("/Game/Developers");
	ExcludedDirectories.Add(DevPath);
}

FName UAssetLinterSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FName UAssetLinterSettings::GetSectionName() const
{
	return TEXT("Asset Linter");
}

#if WITH_EDITOR
FText UAssetLinterSettings::GetSectionText() const
{
	return LOCTEXT("AssetLinterSettingsSectionText", "Asset Linter");
}

FText UAssetLinterSettings::GetSectionDescription() const
{
	return LOCTEXT("AssetLinterSettingsSectionDescription", "에셋 명명 규칙 및 디렉터리 보관 위치 규칙을 검사하는 Asset Linter 설정입니다.");
}
#endif

#undef LOCTEXT_NAMESPACE
