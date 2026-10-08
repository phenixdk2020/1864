#include "Campaign1851Map.h"

bool ACampaign1851Map::RenameUnit(int32 RegimentIndex, const FString& NewName)
{
	if (!Regiments.IsValidIndex(RegimentIndex)) { return false; }
	FString CustomClean = NewName.Left(80).TrimStartAndEnd();
	CustomClean.ReplaceInline(TEXT("\n"), TEXT(" "));
	CustomClean.ReplaceInline(TEXT("\r"), TEXT(" "));
	if (CustomClean.IsEmpty()) { return false; }
	if (Regiments[RegimentIndex].OriginalName.IsEmpty()) { Regiments[RegimentIndex].OriginalName = Regiments[RegimentIndex].Name; }
	Regiments[RegimentIndex].CustomName = CustomClean;
	Regiments[RegimentIndex].Name = CustomClean;
	return true;
}

FLinearColor ACampaign1851Map::UnitPaletteColor(int32 PaletteIndex)
{
	static const FColor CustomPalette[] = {
		FColor(25, 35, 65), FColor(55, 75, 105), FColor(115, 28, 35), FColor(170, 45, 40),
		FColor(35, 65, 45), FColor(75, 85, 55), FColor(105, 80, 55), FColor(165, 145, 110),
		FColor(200, 190, 165), FColor(105, 110, 115), FColor(45, 45, 45), FColor(18, 18, 22)
	};
	return FLinearColor::FromSRGBColor(CustomPalette[FMath::Clamp(PaletteIndex, 0, 11)]);
}

FLinearColor ACampaign1851Map::UnitUniformColor(const FCampaign1851Regiment& Regiment, int32 Piece) const
{
	const int32 CustomPiece = FMath::Clamp(Piece, 0, 2);
	if (Regiment.UniformPalette[CustomPiece] >= 0) { return UnitPaletteColor(Regiment.UniformPalette[CustomPiece]); }
	// Representative national/arm palettes; estimates rather than verified dye specifications.
	if (CustomPiece == 2) { return UnitPaletteColor(11); }
	if (CustomPiece == 1) { return UnitPaletteColor(Regiment.Nation == TEXT("AT") ? 8 : 1); }
	if (Regiment.Nation == TEXT("AT")) { return UnitPaletteColor(8); }
	if (Regiment.Arm == ECampaign1851Arm::Jager) { return UnitPaletteColor(4); }
	return UnitPaletteColor(Regiment.Nation == TEXT("DK") && ActiveScenario().Id == TEXT("1825") ? 2 : 0);
}

bool ACampaign1851Map::SetUnitUniform(int32 RegimentIndex, int32 Piece, int32 PaletteIndex)
{
	if (!Regiments.IsValidIndex(RegimentIndex) || Piece < 0 || Piece > 2 || PaletteIndex < -1 || PaletteIndex > 11) { return false; }
	Regiments[RegimentIndex].UniformPalette[Piece] = PaletteIndex;
	return true;
}

int32 ACampaign1851Map::UnitWeaponLevel(const FCampaign1851Regiment& Regiment) const
{
	if (Regiment.WeaponLevel >= 0) { return FMath::Clamp(Regiment.WeaponLevel, 0, (Regiment.Arm == ECampaign1851Arm::Artillery || Regiment.Arm == ECampaign1851Arm::HorseArtillery) ? 1 : 3); }
	return (Regiment.Arm == ECampaign1851Arm::Artillery || Regiment.Arm == ECampaign1851Arm::HorseArtillery) ? 0 : ActiveScenario().Id == TEXT("1825") ? 0 : 1;
}

FString ACampaign1851Map::UnitWeaponName(const FCampaign1851Regiment& Regiment) const
{
	if (Regiment.Mortars > 0) { return TEXT("Glatløbet morter"); }
	if ((Regiment.Arm == ECampaign1851Arm::Artillery || Regiment.Arm == ECampaign1851Arm::HorseArtillery)) { return UnitWeaponLevel(Regiment) == 1 ? TEXT("Riflet kanon") : TEXT("Glatløbet kanon"); }
	static const TCHAR* CustomNames[] = { TEXT("Flintlåsmusket"), TEXT("Perkussionsmusket"), TEXT("Minié-riffel"), TEXT("Bagladegevær") };
	return Regiment.Arm == ECampaign1851Arm::Cavalry ? FString(CustomNames[UnitWeaponLevel(Regiment)]).Replace(TEXT("musket"), TEXT("karabin")) : FString(CustomNames[UnitWeaponLevel(Regiment)]);
}


bool ACampaign1851Map::CompatibleUnitWeapons(const FCampaign1851Regiment& First, const FCampaign1851Regiment& Second) const
{
	return First.PendingWeaponLevel < 0 && Second.PendingWeaponLevel < 0 && UnitWeaponLevel(First) == UnitWeaponLevel(Second);
}
