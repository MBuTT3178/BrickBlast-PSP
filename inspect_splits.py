import zipfile
import os

out_dir = r"C:\Users\amv31\Downloads\block_blast_apk_dump"
gamedata_apk = os.path.join(out_dir, "split_gamedatalib.apk")
xxhdpi_apk = os.path.join(out_dir, "split_config.xxhdpi.apk")

for apk in [gamedata_apk, xxhdpi_apk]:
    if os.path.exists(apk):
        with zipfile.ZipFile(apk, "r") as z:
            names = z.namelist()
            print(f"=== {os.path.basename(apk)} ({len(names)} files) ===")
            blocks = [f for f in names if any(k in f.lower() for k in ["block", "brick", "tile", "gem", "cell"])]
            print(f"Block-related files: {len(blocks)}")
            for b in blocks[:15]:
                print("  ", b)
