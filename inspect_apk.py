import zipfile
import os

apkm_path = r"C:\Users\amv31\Downloads\com.block.juggle_10.9.1-10910_4arch_7dpi_24lang_1feat_433d3633a479cba9edcc44403b3fcc56_apkmirror.com.apkm"
out_dir = r"C:\Users\amv31\Downloads\block_blast_apk_dump"

os.makedirs(out_dir, exist_ok=True)

with zipfile.ZipFile(apkm_path, "r") as z:
    for name in z.namelist():
        print("APKM Entry:", name)
        if name.endswith(".apk"):
            z.extract(name, out_dir)

print("\nExtracted split APKs.")
base_apk = os.path.join(out_dir, "base.apk")
if os.path.exists(base_apk):
    with zipfile.ZipFile(base_apk, "r") as bz:
        assets = [f for f in bz.namelist() if any(f.endswith(ext) for ext in [".ogg", ".wav", ".mp3", ".json", ".png"])]
        print(f"Total matching assets in base.apk: {len(assets)}")
        audio = [f for f in assets if any(f.endswith(ext) for ext in [".ogg", ".wav", ".mp3"])]
        print("\n--- AUDIO FILES ---")
        for a in audio:
            print(a)
        
        # Look for game config / piece jsons
        jsons = [f for f in assets if f.endswith(".json")]
        print(f"\n--- JSON FILES ({len(jsons)}) ---")
        for j in jsons[:25]:
            print(j)
