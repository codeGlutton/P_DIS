#include "Enhancement/DISEnhancement.h"

UDISEnhancement::UDISEnhancement() :
	PrefixPriority(0),
	SuffixPriority(0),
	Description(FText::GetEmpty()),
	DefaultMaxLevel(10)
{
}

void UDISEnhancement::OnEnhancementAdded_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const
{
	// 하위 파생 클래스 또는 블루프린트에서 강화 스탯 최초 생성/등록 시 초기화 로직 구현
}

void UDISEnhancement::OnEnhancementRemoved_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const
{
	// 하위 파생 클래스 또는 블루프린트에서 강화 스탯 완전 해제/제거 시 최종 정리 로직 구현
}

void UDISEnhancement::OnEnhancementActivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const
{
	// 하위 파생 클래스 또는 블루프린트에서 Target(캐릭터, 컴포넌트 등)을 대상으로 고유 효과 적용
}

void UDISEnhancement::OnEnhancementDeactivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const
{
	// 하위 파생 클래스 또는 블루프린트에서 Target을 대상으로 적용했던 효과 해제/정리
}

void UDISEnhancement::OnEnhancementLevelChanged_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement, int32 OldLevel, int32 NewLevel) const
{
	// 하위 파생 클래스 또는 블루프린트에서 레벨 변경(초기 할당 포함)에 따른 수치/스탯 보정 갱신 로직 구현
}

FText UDISEnhancement::GetPrefixText_Implementation(int32 Level) const
{
	// 기본 구현: 접두사가 없는 경우 빈 텍스트 반환 (하위 파생 클래스 또는 블루프린트에서 오버라이드)
	return FText::GetEmpty();
}

FText UDISEnhancement::GetSuffixText_Implementation(int32 Level) const
{
	// 기본 구현: 접미사가 없는 경우 빈 텍스트 반환 (하위 파생 클래스 또는 블루프린트에서 오버라이드)
	return FText::GetEmpty();
}

int32 UDISEnhancement::GetPrefixPriority() const
{
	return PrefixPriority;
}

int32 UDISEnhancement::GetSuffixPriority() const
{
	return SuffixPriority;
}

const FText& UDISEnhancement::GetDescription() const
{
	return Description;
}

int32 UDISEnhancement::GetDefaultMaxLevel() const
{
	return DefaultMaxLevel;
}
