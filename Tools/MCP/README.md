# Unreal MCP helpers

Talk to the editor's built-in MCP server (http://localhost:8000/mcp) over plain HTTP.
Use them when the Claude session has not connected the "unreal" server from .mcp.json.

- `python tool.py <toolset|-> <tool> '<json args>'` calls one tool (`-` = top-level: list_toolsets, describe_toolset).
- `mat.py` holds material graph helpers: create, expr, wire, out, setp, compile.
- `python capture.py out.png <tx> <ty> <tz> <dist> <pitch> <yaw>` sets the viewport camera to look at a target and saves a PNG.

Delete `sid` if the editor has been restarted; it caches the MCP session id.
