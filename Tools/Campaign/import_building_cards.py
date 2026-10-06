# Imports the building cards (Reference/Campaign1851/Buildings/T_Bld_*.png) into /Game/Campaign1851/Buildings as UI
# textures (no mips, uncompressed alpha), replacing older versions.
# Run: UnrealEditor-Cmd Game1864.uproject -run=pythonscript -script=<this file>
import os, unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tools = unreal.AssetToolsHelpers.get_asset_tools()
JOBS = [(os.path.join(root, 'Reference', 'Campaign1851', 'Buildings'), '/Game/Campaign1851/Buildings', 'T_Bld_'),
        (os.path.join(root, 'Reference', 'Campaign1851', 'Uniforms'), '/Game/Campaign1851/Uniforms', 'T_Uniform_'),
        (os.path.join(root, 'Reference', 'Campaign1851', 'Portraits'), '/Game/Campaign1851/Portraits', 'T_Portrait_')]
for src, DEST, prefix in JOBS:
  for f in (sorted(os.listdir(src)) if os.path.isdir(src) else []):
    if not (f.startswith(prefix) and f.endswith('.png')):
        continue
    name = os.path.splitext(f)[0]
    task = unreal.AssetImportTask()
    for k, v in [('filename', os.path.join(src, f)), ('destination_path', DEST), ('destination_name', name),
                 ('automated', True), ('replace_existing', True), ('save', False)]:
        task.set_editor_property(k, v)
    tools.import_asset_tasks([task])
    t = unreal.load_asset(DEST + '/' + name)
    if t:
        t.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        t.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
        t.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        unreal.EditorAssetLibrary.save_loaded_asset(t)
        unreal.log('CARD ' + name)
unreal.log('CARDS DONE')
