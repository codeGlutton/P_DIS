#include "WorldUIModule.h"

DEFINE_LOG_CATEGORY(LogWorldUI);

#define LOCTEXT_NAMESPACE "FWorldUIModule"

void FWorldUIModule::StartupModule()
{
}

void FWorldUIModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWorldUIModule, WorldUI)