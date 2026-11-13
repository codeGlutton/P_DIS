#include "UI/BBMWorldWidgetTypes.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "MovieScene.h"

UWidgetAnimation* FBBMWidgetAnimConfig::GetAnimation(const UUserWidget* InWidget) const
{
	if (InWidget == nullptr || AnimationName == NAME_None)
	{
		return nullptr;
	}

	const UClass* TargetClass = InWidget->GetClass();
	if (TargetClass == nullptr)
	{
		return nullptr;
	}

	// 캐시된 애니메이션의 소유 클래스(Outer)가 현재 위젯 계층에 속하고 이름도 동일하면 재탐색 생략
	UWidgetAnimation* CachedPtr = CachedAnimation.Get();
	if (CachedPtr != nullptr)
	{
		const UClass* CachedOwnerClass = Cast<UClass>(CachedPtr->GetOuter());
		const UMovieScene* CachedMovieScene = CachedPtr->GetMovieScene();
		if (CachedOwnerClass != nullptr
			&& TargetClass->IsChildOf(CachedOwnerClass) == true
			&& CachedMovieScene != nullptr
			&& CachedMovieScene->GetFName() == AnimationName)
		{
			return CachedPtr;
		}
	}

	// 자식 위젯 클래스부터 부모 WBGC 체인을 순회하며 이름이 일치하는 애니메이션 탐색
	UWidgetAnimation* FoundAnim = nullptr;
	const UClass* CurrentClass = TargetClass;
	while (const UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(CurrentClass))
	{
		for (UWidgetAnimation* Anim : WBGC->Animations)
		{
			if (Anim != nullptr && Anim->GetMovieScene() != nullptr && Anim->GetMovieScene()->GetFName() == AnimationName)
			{
				FoundAnim = Anim;
				break;
			}
		}

		if (FoundAnim != nullptr)
		{
			break;
		}

		CurrentClass = CurrentClass->GetSuperClass();
	}

	CachedAnimation = FoundAnim;
	return FoundAnim;
}

void FBBMWidgetAnimConfig::InvalidateCachedAnimation() const
{
	CachedAnimation.Reset();
}

