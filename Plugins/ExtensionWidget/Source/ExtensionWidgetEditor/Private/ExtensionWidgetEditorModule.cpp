#include "ExtensionWidgetEditorModule.h"

#include "Customizations/RichComboBoxKeyCustomizationExtender.h"
#include "Modules/ModuleManager.h"
#include "UMGEditorModule.h"

#define LOCTEXT_NAMESPACE "FExtensionWidgetEditorModule"

void FExtensionWidgetEditorModule::StartupModule()
{
	IUMGEditorModule& UMGEditorModule = FModuleManager::LoadModuleChecked<IUMGEditorModule>("UMGEditor");

	ComboBoxKeyCustomizationExtender = FRichComboBoxKeyCustomizationExtender::MakeInstance();
	if (ComboBoxKeyCustomizationExtender.IsValid() == true)
	{
		UMGEditorModule.AddWidgetCustomizationExtender(ComboBoxKeyCustomizationExtender.ToSharedRef());
	}
}

void FExtensionWidgetEditorModule::ShutdownModule()
{
	if (ComboBoxKeyCustomizationExtender.IsValid() == true)
	{
		if (FModuleManager::Get().IsModuleLoaded("UMGEditor") == true)
		{
			IUMGEditorModule& UMGEditorModule = FModuleManager::GetModuleChecked<IUMGEditorModule>("UMGEditor");
			UMGEditorModule.RemoveWidgetCustomizationExtender(ComboBoxKeyCustomizationExtender.ToSharedRef());
		}

		ComboBoxKeyCustomizationExtender.Reset();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FExtensionWidgetEditorModule, ExtensionWidgetEditor)
