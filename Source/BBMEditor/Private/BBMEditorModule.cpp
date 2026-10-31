#include "BBMEditorModule.h"
#include "PropertyEditorModule.h"
#include "Customizations/BBMTeamRelationCustomization.h"

#define LOCTEXT_NAMESPACE "FBBMEditorModule"

void FBBMEditorModule::StartupModule()
{
	// FBBMTeamRelation 프로퍼티 커스터마이제이션 등록
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"BBMTeamRelation",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBBMTeamRelationCustomization::MakeInstance)
	);
}

void FBBMEditorModule::ShutdownModule()
{
	// 프로퍼티 커스터마이제이션 해제
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor") == true)
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("BBMTeamRelation");
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBBMEditorModule, BBMEditor);

