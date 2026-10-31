#include "BBMEditorModule.h"
#include "PropertyEditorModule.h"
#include "Customizations/BBMTeamRelationCustomization.h"
#include "Customizations/BBMBeatSequenceCustomization.h"

#define LOCTEXT_NAMESPACE "FBBMEditorModule"

void FBBMEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// FBBMTeamRelation 프로퍼티 커스터마이제이션 등록
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"BBMTeamRelation",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBBMTeamRelationCustomization::MakeInstance)
	);

	// FBBMBeatSequence 프로퍼티 커스터마이제이션 등록 (8비트 3단 타임라인 시각화 바)
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"BBMBeatSequence",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBBMBeatSequenceCustomization::MakeInstance)
	);
}

void FBBMEditorModule::ShutdownModule()
{
	// 프로퍼티 커스터마이제이션 해제
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor") == true)
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("BBMBeatSequence");
		PropertyModule.UnregisterCustomPropertyTypeLayout("BBMTeamRelation");
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBBMEditorModule, BBMEditor);

