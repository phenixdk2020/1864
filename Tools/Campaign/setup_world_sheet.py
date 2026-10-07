# Imports the map textures and builds only the world sheet material (M_Campaign1851World); the level is left alone.
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=R:/.../Tools/Campaign/setup_world_sheet.py
import os as _o2, unreal as _u2
_src = open(_o2.path.join(_u2.Paths.project_dir(), "Tools/Campaign/setup_campaign1851.py"), encoding="utf-8").read()
_cut = _src.index('# Map (unlit: the painting already carries its shading).')
exec(compile(_src[:_cut], "setup_campaign1851_head", "exec"))
_block = _src[_src.index("# The coarse world sheet under the detailed map"):_src.index('log("material M_Campaign1851World")') + len('log("material M_Campaign1851World")')]
exec(compile(_block, "setup_world_block", "exec"))
