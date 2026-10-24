import zipfile

base_apk = r"C:\Users\amv31\Downloads\block_blast_apk_dump\base.apk"
with zipfile.ZipFile(base_apk, "r") as bz:
    js_files = [f for f in bz.namelist() if f.endswith(".js")]
    print(f"JS files ({len(js_files)}):")
    for j in js_files[:20]:
        print(" ", j)
