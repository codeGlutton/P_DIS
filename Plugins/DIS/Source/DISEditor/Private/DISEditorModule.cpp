#include "DISEditorModule.h"
#include "PropertyEditorModule.h"
#include "Customizations/DISItemShapeCustomization.h"

#define LOCTEXT_NAMESPACE "FDISEditorModule"

void FDISEditorModule::StartupModule()
{
	// FDISItemShape 프로퍼티 커스터마이제이션 등록
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"DISItemShape",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDISItemShapeCustomization::MakeInstance)
	);
}

void FDISEditorModule::ShutdownModule()
{
	// 프로퍼티 커스터마이제이션 해제
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor") == true)
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("DISItemShape");
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDISEditorModule, DISEditor);
