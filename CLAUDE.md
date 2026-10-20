@AGENTS.md

# CLAUDE.md

Project rules are in AGENTS.md.

## Workflow

- Use Plan Mode before new features, significant design tradeoffs, and bugs that survive the first attempted fix.
- Delegate repository mapping to the `search` subagent, approved test runs to `qa-test`, and API lookups to `docs`.
- Specialised agents (see Docs/AI-Spilstudie-1864.html): `research`, `koncept`, `model3d`, `tekstur`, `blender`, `rigging-animation`, `qa-test`, `unreal-integration`, `asset-manager`, and `codex` for Codex CLI tasks. Only one GPU-heavy job at a time (Pinokio app, build or Codex).
