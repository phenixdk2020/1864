import json
from tool import call
MT = "editor_toolset.toolsets.material.MaterialTools"
OT = "editor_toolset.toolsets.object.ObjectTools"

def rv(r):
    if "error" in r: raise RuntimeError(json.dumps(r["error"])[:500])
    txt = " ".join(c.get("text", "") for c in r["result"].get("content", []))
    if r["result"].get("isError"): raise RuntimeError(txt[:800])
    try: return json.loads(txt).get("returnValue")
    except Exception: return txt

def ref(p): return {"refPath": p}

def create(folder, name):
    return rv(call(MT, "create_material", {"folder_path": folder, "asset_name": name}))["refPath"]

def expr(mat, cls, x=0, y=0, **props):
    e = rv(call(MT, "add_expression", {"material_or_function": ref(mat), "expression_class": ref("/Script/Engine." + cls), "x": x, "y": y}))["refPath"]
    if props: setp(e, props)
    return e

def setp(obj, props):
    return rv(call(OT, "set_properties", {"instance": ref(obj), "values": json.dumps(props)}))

def wire(a, b, pin, out=""):
    return rv(call(MT, "connect_expressions", {"from_expression": ref(a), "from_output_name": out, "to_expression": ref(b), "to_input_name": pin}))

def out(e, prop, pin=""):
    return rv(call(MT, "connect_to_output", {"expression": ref(e), "output_name": pin, "material_property": prop}))

def compile(mat):
    return rv(call(MT, "recompile", {"material_or_function": ref(mat)}))
