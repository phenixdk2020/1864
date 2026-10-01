#include "StrategyUniformPresetLibrary.h"

FStrategyUniformPreset
UStrategyUniformPresetLibrary::MakeNeutralQAPreset()
{
    FStrategyUniformPreset Preset;
    Preset.PresetId = TEXT("QA_NEUTRAL");
    Preset.NationId = TEXT("QA");
    Preset.UnitTypeId = TEXT("GENERIC");
    Preset.bLockHistoricalPalette = false;

    Preset.Colors.Coat =
        FLinearColor(0.22f, 0.22f, 0.22f, 1.0f);
    Preset.Colors.Trousers =
        FLinearColor(0.18f, 0.18f, 0.18f, 1.0f);
    Preset.Colors.Facings =
        FLinearColor(0.45f, 0.45f, 0.45f, 1.0f);
    Preset.Colors.HeadgearDetail =
        FLinearColor(0.08f, 0.08f, 0.08f, 1.0f);
    Preset.Colors.Leather =
        FLinearColor(0.07f, 0.035f, 0.02f, 1.0f);
    Preset.Colors.Accent =
        FLinearColor(0.50f, 0.50f, 0.50f, 1.0f);
    Preset.Colors.Metal =
        FLinearColor(0.55f, 0.48f, 0.30f, 1.0f);

    return Preset;
}

FStrategyUniformPreset
UStrategyUniformPresetLibrary::MakeDanishQAPreset()
{
    FStrategyUniformPreset Preset =
        MakeNeutralQAPreset();

    Preset.PresetId = TEXT("QA_DK");
    Preset.NationId = TEXT("DENMARK");
    Preset.UnitTypeId = TEXT("INFANTRY");

    Preset.Colors.Coat =
        FLinearColor(0.42f, 0.06f, 0.05f, 1.0f);
    Preset.Colors.Trousers =
        FLinearColor(0.12f, 0.18f, 0.28f, 1.0f);
    Preset.Colors.Facings =
        FLinearColor(0.75f, 0.72f, 0.64f, 1.0f);
    Preset.Colors.Accent =
        FLinearColor(0.80f, 0.80f, 0.80f, 1.0f);

    return Preset;
}

FStrategyUniformPreset
UStrategyUniformPresetLibrary::MakePrussianQAPreset()
{
    FStrategyUniformPreset Preset =
        MakeNeutralQAPreset();

    Preset.PresetId = TEXT("QA_PR");
    Preset.NationId = TEXT("PRUSSIA");
    Preset.UnitTypeId = TEXT("INFANTRY");

    Preset.Colors.Coat =
        FLinearColor(0.04f, 0.08f, 0.18f, 1.0f);
    Preset.Colors.Trousers =
        FLinearColor(0.14f, 0.14f, 0.16f, 1.0f);
    Preset.Colors.Facings =
        FLinearColor(0.42f, 0.08f, 0.06f, 1.0f);
    Preset.Colors.Accent =
        FLinearColor(0.72f, 0.72f, 0.72f, 1.0f);

    return Preset;
}

FStrategyUniformPreset
UStrategyUniformPresetLibrary::MakeArtilleryQAPreset()
{
    FStrategyUniformPreset Preset =
        MakeDanishQAPreset();

    Preset.PresetId = TEXT("QA_DK_ARTILLERY");
    Preset.UnitTypeId = TEXT("ARTILLERY");
    Preset.Colors.Facings =
        FLinearColor(0.10f, 0.10f, 0.10f, 1.0f);
    Preset.Colors.Accent =
        FLinearColor(0.88f, 0.65f, 0.10f, 1.0f);

    return Preset;
}
