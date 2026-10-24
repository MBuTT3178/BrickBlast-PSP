import zipfile
import os

base_apk = r"C:\Users\amv31\Downloads\block_blast_apk_dump\base.apk"
out_dir = r"C:\Users\amv31\Downloads\block_blast_apk_dump\extracted"
os.makedirs(out_dir, exist_ok=True)

with zipfile.ZipFile(base_apk, "r") as bz:
    # Extract settings.json or project.json
    for name in bz.namelist():
        if "settings." in name and name.endswith(".json"):
            bz.extract(name, out_dir)
            print("Extracted settings:", name)
        if "project." in name and name.endswith(".json"):
            bz.extract(name, out_dir)
            print("Extracted project:", name)
        
        # Extract BrickSoundTrait and DynamicSoundTrait audio
        if any(trait in name for trait in ["BrickSoundTrait", "DynamicSoundTrait", "ElectronicSoundTrait", "BlockBackPlayAudioTrait"]) and name.endswith(".mp3"):
            bz.extract(name, out_dir)
            print("Extracted audio:", name)
