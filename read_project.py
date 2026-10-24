import json

proj_path = r"C:\Users\amv31\Downloads\block_blast_apk_dump\extracted\assets\project.json"
with open(proj_path, "r", encoding="utf-8") as f:
    data = json.load(f)

print("project.json keys:", list(data.keys()))
if "types" in data:
    print("types:", data["types"])
if "uuids" in data:
    print("total uuids:", len(data["uuids"]))
    block_uuids = [u for u in data["uuids"] if "block" in str(u).lower()]
    print("sample uuids:", data["uuids"][:10])
