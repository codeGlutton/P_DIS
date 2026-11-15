#include "UI/BBMWorldWidgetTypes.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "MovieScene.h"

namespace
{
	UWidgetAnimation* FindAnimationInWidgetClass(const UUserWidget* InWidget, const FName& InAnimationName)
	{
		if (InWidget == nullptr || InAnimationName == NAME_None)
		{
			return nullptr;
		}

		const UClass* CurrentClass = InWidget->GetClass();
		while (const UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(CurrentClass))
		{
			for (UWidgetAnimation* Anim : WBGC->Animations)
			{
				if (Anim != nullptr && Anim->GetMovieScene() != nullptr && Anim->GetMovieScene()->GetFName() == InAnimationName)
				{
					return Anim;
				}
			}

			CurrentClass = CurrentClass->GetSuperClass();
		}

		return nullptr;
	}

	bool FindAnimationRecursive(
		UUserWidget* InCurrentWidget,
		const FName& InAnimationName,
		TSet<const UUserWidget*>& InOutVisitedWidgets,
		UWidgetAnimation*& OutAnimation,
		UUserWidget*& OutOwnerWidget
	)
	{
		if (InCurrentWidget == nullptr || InOutVisitedWidgets.Contains(InCurrentWidget) == true)
		{
			return false;
		}

		InOutVisitedWidgets.Add(InCurrentWidget);

		// 1. 현재 위젯(자신)의 WBGC 상속 계층에서 먼저 탐색
		UWidgetAnimation* DirectAnim = FindAnimationInWidgetClass(InCurrentWidget, InAnimationName);
		if (DirectAnim != nullptr)
		{
			OutAnimation = DirectAnim;
			OutOwnerWidget = InCurrentWidget;
			return true;
		}

		// 2. 자신에게 없으면 위젯 트리의 하위 UUserWidget들을 재귀 탐색
		if (InCurrentWidget->WidgetTree == nullptr || InCurrentWidget->WidgetTree->RootWidget == nullptr)
		{
			return false;
		}

		UWidget* RootWidget = InCurrentWidget->WidgetTree->RootWidget;

		// ForEachWidgetAndChildrenUntil 규격: 계속 탐색하려면 true, 탐색 성공으로 즉시 중단(Early-Out)하려면 false 반환
		const auto ShouldContinueSearch = [&](UWidget* InTargetWidget) -> bool
		{
			UUserWidget* ChildUserWidget = Cast<UUserWidget>(InTargetWidget);
			if (ChildUserWidget != nullptr)
			{
				if (FindAnimationRecursive(ChildUserWidget, InAnimationName, InOutVisitedWidgets, OutAnimation, OutOwnerWidget) == true)
				{
					return false;
				}
			}

			return true;
		};

		// 2-1. 직속 하위 위젯인 RootWidget 자체가 또 다른 UserWidget인 경우 내부의 WidgetTree를 검사
		if (ShouldContinueSearch(RootWidget) == false)
		{
			return true;
		}

		// 2-2. 하위 자식 및 NamedSlot 위젯들을 조기 종료 순회
		if (UWidgetTree::ForEachWidgetAndChildrenUntil(RootWidget, ShouldContinueSearch) == false)
		{
			return true;
		}

		return false;
	}
}

UWidgetAnimation* FBBMWidgetAnimConfig::GetAnimation(UUserWidget* InWidget) const
{
	if (ResolveAnimation(InWidget) == false)
	{
		return nullptr;
	}

	return CachedAnimation.Get();
}

UUserWidget* FBBMWidgetAnimConfig::GetOwnerWidget(UUserWidget* InWidget) const
{
	if (ResolveAnimation(InWidget) == false)
	{
		return nullptr;
	}

	return CachedOwnerWidget.Get();
}

void FBBMWidgetAnimConfig::InvalidateCachedAnimation() const
{
	CachedTargetWidget.Reset();
	CachedOwnerWidget.Reset();
	CachedAnimation.Reset();
}

bool FBBMWidgetAnimConfig::ResolveAnimation(UUserWidget* InWidget) const
{
	if (InWidget == nullptr || AnimationName == NAME_None)
	{
		return false;
	}

	// 탐색 대상(TargetWidget)과 소유 위젯·애니메이션·이름이 모두 유효하고 일치하면 캐시 히트
	UUserWidget* CachedTargetPtr = CachedTargetWidget.Get();
	UUserWidget* CachedOwnerPtr = CachedOwnerWidget.Get();
	UWidgetAnimation* CachedAnimPtr = CachedAnimation.Get();
	if (CachedTargetPtr != nullptr
		&& CachedTargetPtr == InWidget
		&& CachedOwnerPtr != nullptr
		&& CachedAnimPtr != nullptr)
	{
		const UMovieScene* CachedMovieScene = CachedAnimPtr->GetMovieScene();
		if (CachedMovieScene != nullptr && CachedMovieScene->GetFName() == AnimationName)
		{
			return true;
		}
	}

	// TargetWidget이 다르거나 캐시가 무효하면 자기 자신 및 하위 위젯 트리를 재귀 탐색
	TSet<const UUserWidget*> VisitedWidgets;
	UWidgetAnimation* FoundAnim = nullptr;
	UUserWidget* FoundOwner = nullptr;
	if (FindAnimationRecursive(InWidget, AnimationName, VisitedWidgets, FoundAnim, FoundOwner) == true
		&& FoundAnim != nullptr
		&& FoundOwner != nullptr)
	{
		CachedTargetWidget = InWidget;
		CachedOwnerWidget = FoundOwner;
		CachedAnimation = FoundAnim;
		return true;
	}

	InvalidateCachedAnimation();
	return false;
}

