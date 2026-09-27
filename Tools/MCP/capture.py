import base64, json, math, sys
from tool import call
# usage: capture.py out.png targetX targetY targetZ dist pitch yaw
out, tx, ty, tz, dist, pitch, yaw = sys.argv[1], *map(float, sys.argv[2:8])
p, y = math.radians(pitch), math.radians(yaw)
d = (math.cos(p)*math.cos(y), math.cos(p)*math.sin(y), math.sin(p))
loc = {"x": tx - d[0]*dist, "y": ty - d[1]*dist, "z": tz - d[2]*dist}
xf = {"location": loc, "rotation": {"pitch": pitch, "yaw": yaw, "roll": 0}}
call("EditorToolset.EditorAppToolset", "SetCameraTransform", {"transform": xf})
r = call("EditorToolset.EditorAppToolset", "CaptureViewport", {"captureTransform": xf, "annotations": {"gridSpacing":0,"gridExtent":0,"gridHeight":0,"maxLabelDistance":0,"classFilter":None,"maxLabels":0}, "bShowUI": False})
if "error" in r: print(r["error"]); sys.exit(1)
for c in r["result"].get("content", []):
    if c.get("type") == "image":
        open(out, "wb").write(base64.b64decode(c["data"])); print("saved", out, c.get("mimeType"))
    else:
        t = c.get("text", "")
        try:
            v = json.loads(t)
            img = v.get("returnValue", {}).get("image", {})
            if img.get("data"):
                open(out, "wb").write(base64.b64decode(img["data"])); print("saved", out); img["data"] = "..."
            s = json.dumps(v)
            print(s[:600])
        except Exception:
            print(t[:600])
