import zipfile
import os
import subprocess

base_apk = r"C:\Users\amv31\Downloads\block_blast_apk_dump\base.apk"
out_dir = r"C:\Users\amv31\Downloads\block_blast_apk_dump\all_audio"
os.makedirs(out_dir, exist_ok=True)

with zipfile.ZipFile(base_apk, "r") as bz:
    audio_files = [f for f in bz.namelist() if any(f.endswith(ext) for ext in [".ogg", ".mp3", ".wav"])]
    print(f"Total audio files found: {len(audio_files)}")
    
    for a in audio_files:
        base_name = os.path.basename(a)
        # prefix with folder hint
        folder_hint = "res" if "resources" in a else ("brick" if "Brick" in a else ("dyn" if "Dynamic" in a else "other"))
        out_name = f"{folder_hint}_{base_name}"
        target = os.path.join(out_dir, out_name)
        with open(target, "wb") as f:
            f.write(bz.read(a))

extracted = os.listdir(out_dir)
print(f"Extracted {len(extracted)} audio files to {out_dir}")
for f in sorted(extracted):
    sz = os.path.getsize(os.path.join(out_dir, f))
    print(f"  {f} ({sz} bytes)")
