import zipfile
import os

base_apk = r"C:\Users\amv31\Downloads\block_blast_apk_dump\base.apk"
out_dir = r"C:\Users\amv31\Downloads\block_blast_apk_dump\sprites"
os.makedirs(out_dir, exist_ok=True)

with zipfile.ZipFile(base_apk, "r") as bz:
    pngs = [f for f in bz.namelist() if f.endswith(".png")]
    print(f"Total PNGs in base.apk: {len(pngs)}")
    
    # Extract textures from resources and main traits
    for p in pngs:
        # Check size or name
        info = bz.getinfo(p)
        # Gem / block textures are usually between 1KB and 50KB
        if any(k in p for k in ["resources", "Brick", "Block", "Theme", "Skin"]) and (500 < info.file_size < 100000):
            target = os.path.join(out_dir, os.path.basename(p))
            with open(target, "wb") as f:
                f.write(bz.read(p))

print(f"Extracted candidate textures to {out_dir}")
extracted_files = os.listdir(out_dir)
print(f"Extracted count: {len(extracted_files)}")
for f in extracted_files[:30]:
    print(" ", f)
