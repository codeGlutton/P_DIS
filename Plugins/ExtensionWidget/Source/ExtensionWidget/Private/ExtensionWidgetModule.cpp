#include "ExtensionWidgetModule.h"

DEFINE_LOG_CATEGORY(LogExtensionWidget);

#define LOCTEXT_NAMESPACE "FExtensionWidgetModule"

void FExtensionWidgetModule::StartupModule()
{
}

void FExtensionWidgetModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FExtensionWidgetModule, ExtensionWidget)
