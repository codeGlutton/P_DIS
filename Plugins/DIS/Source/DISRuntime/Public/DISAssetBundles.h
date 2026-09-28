/*****************************************************************//**
 * @file   DISAssetBundles.h
 * @brief  DIS 플러그인 전역 에셋 번들(AssetBundles) 식별자 정의
 * @date   2026-09-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"

/**
 * DIS 플러그인 전역에서 사용하는 AssetBundle 식별자 모음
 */
namespace DISAssetBundles
{
	/* 실제 UI 표시에 필요한 에셋 번들 (아이콘, UI 텍스처 등) */
	constexpr const TCHAR* UI = TEXT("UI");
	/* 월드 액터 배치 및 시각적 표현에 필요한 에셋 번들 (메시, 머티리얼 등) */
	constexpr const TCHAR* World = TEXT("World");
	/* 타 DataAsset 등 데이터 참조용 에셋 번들 */
	constexpr const TCHAR* Data = TEXT("Data");
}
