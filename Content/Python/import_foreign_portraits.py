"""Importerer SE-, NO- og PR-portrætter som UI-teksturer.

Køres manuelt i Unreal, når import ønskes. Scriptet er ikke kørt ved oprettelsen.
Indstillinger følger Tools/Campaign/import_building_cards.py (de danske portrætter).
Eksisterende danske assets ændres ikke.
"""
from pathlib import Path
import struct
import unreal

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Reference" / "Campaign1851" / "Portraits"
DESTINATION = "/Game/Campaign1851/Portraits"
NATIONS = ("SE", "NO", "PR")
EXPECTED_PER_NATION = 42
SETS = (("Officer_Ung", 10), ("Officer_Aeldre", 10),
        ("Officer_Gammel", 6), ("General", 6), ("Minister", 10))


def run():
    # Kontroller alle 126 filer før første import, så et ufuldstændigt såt opdages tidligt.
    jobs = []
    for nation in NATIONS:
        expected_names = {
            f"T_Portrait_{nation}_{group}_{index:02d}.png"
            for group, count in SETS for index in range(count)
        }
        actual_names = {png.name for png in (SOURCE / nation).glob("*.png")}
        if len(expected_names) != EXPECTED_PER_NATION or actual_names != expected_names:
            raise ValueError(
                f"{nation}: forventer præcis {EXPECTED_PER_NATION} PNG-filer; "
                f"mangler={sorted(expected_names - actual_names)}, "
                f"ekstra={sorted(actual_names - expected_names)}")
        for group, count in SETS:
            for index in range(count):
                name = f"T_Portrait_{nation}_{group}_{index:02d}"
                png = SOURCE / nation / (name + ".png")
                with png.open("rb") as source_file:
                    header = source_file.read(33)
                if (len(header) != 33 or header[:8] != b"\x89PNG\r\n\x1a\n"
                        or header[12:16] != b"IHDR"
                        or struct.unpack(">II", header[16:24]) != (512, 640)
                        or header[24:26] != bytes((8, 2))):
                    raise ValueError(f"Forventer 512x640, 8-bit RGB PNG: {png}")
                jobs.append((nation, name, png))

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    imported_count = 0
    for nation, name, png in jobs:
        destination = f"{DESTINATION}/{nation}"
        unreal.EditorAssetLibrary.make_directory(destination)
        task = unreal.AssetImportTask()
        for key, value in (("filename", str(png)), ("destination_path", destination),
                           ("destination_name", name), ("automated", True),
                           ("replace_existing", True), ("save", False)):
            task.set_editor_property(key, value)
        asset_tools.import_asset_tasks([task])
        texture = unreal.load_asset(f"{destination}/{name}")
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError(f"Teksturimport mislykkedes: {destination}/{name}")
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("srgb", True)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
            raise RuntimeError(f"Kunne ikke gemme tekstur: {destination}/{name}")
        imported_count += 1
        unreal.log(f"PROJECT1864-PORTRAET: {destination}/{name}")
    unreal.log(f"PROJECT1864-PORTRAETTER: {imported_count} teksturer importeret")


run()
