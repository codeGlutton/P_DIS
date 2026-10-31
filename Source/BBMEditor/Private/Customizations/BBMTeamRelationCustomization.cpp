#include "Customizations/BBMTeamRelationCustomization.h"
#include "Combat/Team/BBMTeamTypes.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FBBMTeamRelationCustomization"

TSharedRef<IPropertyTypeCustomization> FBBMTeamRelationCustomization::MakeInstance()
{
	return MakeShared<FBBMTeamRelationCustomization>();
}

void FBBMTeamRelationCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> InPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = InPropertyHandle;
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	HeaderRow
	.NameContent()
	[
		InPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(260.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(this, &FBBMTeamRelationCustomization::GetSummaryText)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(LOCTEXT("FillMissingTeamsText", "누락된 팀 자동 채우기"))
			.ToolTipText(LOCTEXT("FillMissingTeamsToolTip", "EBBMTeamType 열거형 중 현재 목록에 누락된 모든 팀을 기본 태도로 일괄 추가합니다."))
			.OnClicked(this, &FBBMTeamRelationCustomization::OnFillMissingTeamsClicked)
		]
	];
}

void FBBMTeamRelationCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> InPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> AttitudesHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBBMTeamRelation, Attitudes));
	if (AttitudesHandle.IsValid() == true)
	{
		ChildBuilder.AddProperty(AttitudesHandle.ToSharedRef());
	}
}

FReply FBBMTeamRelationCustomization::OnFillMissingTeamsClicked()
{
	if (StructPropertyHandle.IsValid() == false)
	{
		return FReply::Unhandled();
	}

	const FScopedTransaction Transaction(LOCTEXT("FillMissingTeamsTransaction", "BBM: 누락된 팀 관계 일괄 생성"));
	StructPropertyHandle->NotifyPreChange();

	TArray<void*> RawData;
	StructPropertyHandle->AccessRawData(RawData);

	const EBBMTeamType ContextTeam = GetContextTeam();

	for (void* DataPtr : RawData)
	{
		if (DataPtr != nullptr)
		{
			FBBMTeamRelation* Relation = static_cast<FBBMTeamRelation*>(DataPtr);
			Relation->FillMissingTeams(ContextTeam);
		}
	}

	StructPropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	StructPropertyHandle->NotifyFinishedChangingProperties();

	if (PropertyUtilities.IsValid() == true)
	{
		PropertyUtilities->ForceRefresh();
	}

	return FReply::Handled();
}

FText FBBMTeamRelationCustomization::GetSummaryText() const
{
	if (StructPropertyHandle.IsValid() == false)
	{
		return FText::GetEmpty();
	}

	TSharedPtr<IPropertyHandle> AttitudesHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBBMTeamRelation, Attitudes));
	uint32 NumChildren = 0;
	if (AttitudesHandle.IsValid() == true && AttitudesHandle->GetNumChildren(NumChildren) == FPropertyAccess::Success)
	{
		return FText::Format(LOCTEXT("SummaryFormat", "{0}개 태도 설정됨"), FText::AsNumber(NumChildren));
	}

	return FText::GetEmpty();
}

EBBMTeamType FBBMTeamRelationCustomization::GetContextTeam() const
{
	if (StructPropertyHandle.IsValid() == true)
	{
		TSharedPtr<IPropertyHandle> KeyHandle = StructPropertyHandle->GetKeyHandle();
		if (KeyHandle.IsValid() == true)
		{
			uint8 KeyByte = 0;
			if (KeyHandle->GetValue(KeyByte) == FPropertyAccess::Success)
			{
				return static_cast<EBBMTeamType>(KeyByte);
			}
		}
	}

	return EBBMTeamType::Neutral;
}

#undef LOCTEXT_NAMESPACE

