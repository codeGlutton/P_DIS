#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "BBMCharacterSelectViewModel.generated.h"

/**
 * 캐릭터 선택 패널 ViewModel
 */
UCLASS(BlueprintType, Blueprintable)
class BBM_API UBBMCharacterSelectViewModel : public UBBMViewModelBase
{
	GENERATED_BODY()

public:
	/**
	 * 선택한 캐릭터로 새 게임을 시작하는 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|CharacterSelect")
	void StartNewGame();
};

