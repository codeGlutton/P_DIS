#include "Global/DISGlobalDefinition.h"

UDISGlobalDefinition::UDISGlobalDefinition()
{
}

FPrimaryAssetId UDISGlobalDefinition::GetPrimaryAssetId() const
{
	// 에셋 매니저에서 식별 가능한 PrimaryAssetType 반환
	return FPrimaryAssetId(DISPrimaryAssetTypes::Global, GetFName());
}
