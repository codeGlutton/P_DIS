/*****************************************************************//**
 * @file   SRichListView.h
 * @brief  SListView를 확장하여 SRichScrollBar를 내장 연동하는 Slate 리스트 뷰 템플릿
 * @details ITypedUMGListView::ConstructListView와 100% 호환되도록 SListView::FArguments를 승계하고,
 *          ExternalScrollbar에 SRichScrollBarBridge를 주입한 뒤 ChildSlot을 래핑하여
 *          UListViewBase::RebuildWidget() final 제약을 위반하지 않고 SRichScrollBar를 결합함.
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Types/RichScrollBarTypes.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SRichScrollBar.h"
#include "Widgets/SRichScrollBarBridge.h"
#include "Widgets/Views/SListView.h"

/**
 * SListView 파생 확장 Slate 리스트 뷰 템플릿
 */
template <typename ItemType>
class SRichListView : public SListView<ItemType>
{
	/* 타입 별칭 */
public:
	using FSuper = SListView<ItemType>;
	using FArguments = typename FSuper::FArguments;

	/* 위젯 생성 및 스크롤바 구성 API */
public:
	void Construct(const FArguments& InArgs)
	{
		TSharedRef<SRichScrollBarBridge> CreatedBridge = SNew(SRichScrollBarBridge);
		BridgeScrollBar = CreatedBridge;
		Placement = ERichScrollBarPlacement::End;
		RichOrientation = InArgs._Orientation;
		ScrollBarSlotPadding = InArgs._ScrollBarPadding;

		FArguments ModifiedArgs = InArgs;
		ModifiedArgs.ExternalScrollbar(CreatedBridge);
		FSuper::Construct(ModifiedArgs);

		// STableViewBase가 ChildSlot에 배치한 순수 리스트 컨텐츠를 보존하고 SRichScrollBar와 함께 래핑
		OriginalListContent = this->ChildSlot.GetWidget();

		RichScrollBar = SNew(SRichScrollBar)
			.Style(InArgs._ScrollBarStyle)
			.Orientation(RichOrientation)
			.OnUserScrolled(FOnUserScrolled::CreateSP(this, &SRichListView::HandleRichScrollBarUserScrolled))
			.OnArrowScrolled(FOnRichArrowScrolled::CreateSP(this, &SRichListView::HandleRichScrollBarArrowScrolled));

		CreatedBridge->BindRichScrollBar(RichScrollBar);
		RebuildChildSlotWrapper();
	}

	void ConfigureRichScrollBar(
		const FScrollBarStyle* InStyle,
		EOrientation InOrientation,
		bool bInAlwaysShow,
		const FMargin& InSlotPadding,
		const FRichScrollBarConfig& InConfig,
		ERichScrollBarPlacement InPlacement)
	{
		const bool bLayoutChanged = (RichOrientation != InOrientation) || (Placement != InPlacement);
		RichOrientation = InOrientation;
		Placement = InPlacement;
		ScrollBarSlotPadding = InSlotPadding;

		if (RichScrollBar.IsValid() == true)
		{
			RichScrollBar->SetStyle(InStyle);
			RichScrollBar->ClearThicknessOverride();
			RichScrollBar->SetOrientation(RichOrientation);
			RichScrollBar->SetAlwaysShowScrollbar(bInAlwaysShow);
			RichScrollBar->SetConfig(InConfig);
		}

		if (bLayoutChanged == true)
		{
			RebuildChildSlotWrapper();
		}

		this->Invalidate(EInvalidateWidgetReason::Layout);
	}

	TSharedPtr<SRichScrollBar> GetRichScrollBar() const
	{
		return RichScrollBar;
	}

	TSharedPtr<SRichScrollBarBridge> GetBridgeScrollBar() const
	{
		return BridgeScrollBar;
	}

	/* 내부 레이아웃 및 스크롤 핸들러 */
private:
	void RebuildChildSlotWrapper()
	{
		if (OriginalListContent.IsValid() == false || RichScrollBar.IsValid() == false)
		{
			return;
		}

		const TSharedRef<SWidget> ListWidgetRef = OriginalListContent.ToSharedRef();
		const TSharedRef<SRichScrollBar> ScrollBarRef = RichScrollBar.ToSharedRef();
		const TAttribute<FMargin> ScrollBarPaddingAttr = TAttribute<FMargin>::CreateSP(this, &SRichListView::GetScrollBarSlotPadding);
		const bool bPlaceStart = (Placement == ERichScrollBarPlacement::Start);

		if (RichOrientation == Orient_Vertical)
		{
			TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);
			if (bPlaceStart == true)
			{
				HorizontalBox->AddSlot().AutoWidth().Padding(ScrollBarPaddingAttr)[ScrollBarRef];
				HorizontalBox->AddSlot().FillWidth(1.0f)[ListWidgetRef];
			}
			else
			{
				HorizontalBox->AddSlot().FillWidth(1.0f)[ListWidgetRef];
				HorizontalBox->AddSlot().AutoWidth().Padding(ScrollBarPaddingAttr)[ScrollBarRef];
			}

			this->ChildSlot[HorizontalBox];
		}
		else
		{
			TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);
			if (bPlaceStart == true)
			{
				VerticalBox->AddSlot().AutoHeight().Padding(ScrollBarPaddingAttr)[ScrollBarRef];
				VerticalBox->AddSlot().FillHeight(1.0f)[ListWidgetRef];
			}
			else
			{
				VerticalBox->AddSlot().FillHeight(1.0f)[ListWidgetRef];
				VerticalBox->AddSlot().AutoHeight().Padding(ScrollBarPaddingAttr)[ScrollBarRef];
			}

			this->ChildSlot[VerticalBox];
		}
	}

	FMargin GetScrollBarSlotPadding() const
	{
		return ScrollBarSlotPadding;
	}

	void HandleRichScrollBarUserScrolled(float InNewOffsetFraction)
	{
		if (BridgeScrollBar.IsValid() == true)
		{
			BridgeScrollBar->ForwardUserScrollToOwner(InNewOffsetFraction);
		}
	}

	void HandleRichScrollBarArrowScrolled(float InSignedPixelDelta)
	{
		this->ScrollBy(this->GetTickSpaceGeometry(), InSignedPixelDelta, EAllowOverscroll::No);
	}

	/* 멤버 변수 */
private:
	TSharedPtr<SWidget> OriginalListContent;
	TSharedPtr<SRichScrollBarBridge> BridgeScrollBar;
	TSharedPtr<SRichScrollBar> RichScrollBar;
	EOrientation RichOrientation;
	ERichScrollBarPlacement Placement;
	FMargin ScrollBarSlotPadding;
};
