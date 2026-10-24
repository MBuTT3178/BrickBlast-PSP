import os
import subprocess
import json

audio_dir = "/mnt/c/Users/amv31/Downloads/block_blast_apk_dump/all_audio"
files = [f for f in os.listdir(audio_dir) if f.startswith("res_") or "3105b" in f or "2fac" in f]

for f in sorted(files):
    p = os.path.join(audio_dir, f)
    cmd = [
        "ffprobe", "-v", "error", "-show_entries",
        "format=duration:stream=channels,sample_rate",
        "-of", "json", p
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.stdout:
        try:
            info = json.loads(res.stdout)
            dur = float(info.get("format", {}).get("duration", 0))
            print(f"{f}: {dur:.2f}s")
        except:
            pass
