# Usage: python tool.py <toolset|-> <tool> ['<args-json>' | @file.json]
import json, os, sys, urllib.request

URL = "http://localhost:8000/mcp"
S = os.path.dirname(os.path.abspath(__file__))
SID_FILE = os.path.join(S, "sid")


def post(payload, sid=None):
    headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
    if sid:
        headers["Mcp-Session-Id"] = sid
    req = urllib.request.Request(URL, json.dumps(payload).encode("utf-8"), headers)
    with urllib.request.urlopen(req, timeout=900) as resp:
        return resp.headers.get("Mcp-Session-Id"), resp.read().decode("utf-8")


def session():
    if os.path.exists(SID_FILE):
        return open(SID_FILE).read().strip() or None
    sid, _ = post({"jsonrpc": "2.0", "id": 0, "method": "initialize", "params": {
        "protocolVersion": "2025-11-25", "capabilities": {}, "clientInfo": {"name": "claude-http", "version": "1"}}})
    post({"jsonrpc": "2.0", "method": "notifications/initialized"}, sid)
    open(SID_FILE, "w").write(sid or "")
    return sid


def call(toolset, tool, args):
    a = {"tool_name": tool, "arguments": args}
    if toolset != "-":
        a["toolset_name"] = toolset
    _, body = post({"jsonrpc": "2.0", "id": 1, "method": "tools/call", "params": {"name": "call_tool", "arguments": a}}, session())
    return json.loads(body)


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raw = sys.argv[3] if len(sys.argv) > 3 else "{}"
    if raw.startswith("@"):
        raw = open(raw[1:], encoding="utf-8").read()
    r = call(sys.argv[1], sys.argv[2], json.loads(raw))
    if "error" in r:
        print("ERROR:", json.dumps(r["error"], ensure_ascii=False))
        sys.exit(1)
    res = r["result"]
    for c in res.get("content", []):
        print(c.get("text", c))
    if res.get("isError"):
        sys.exit(1)
