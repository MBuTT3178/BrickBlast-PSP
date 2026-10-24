#!/usr/bin/env python3
import os
import subprocess
import struct

extracted_dir = "/mnt/c/Users/amv31/Downloads/block_blast_apk_dump/extracted/assets/assets"
out_dir = "/mnt/c/Users/amv31/Downloads/block_blast_psp/pcm_sounds"
os.makedirs(out_dir, exist_ok=True)

place_src = f"{extracted_dir}/BrickSoundTrait/native/4d/4d72f8fc-b88e-47b2-9896-b29a94a45966.e75c1.mp3"
clear_src = f"{extracted_dir}/BrickSoundTrait/native/6c/6ccde367-2a08-46b5-b263-9649af952df2.6a76d.mp3"
fail_src = f"{extracted_dir}/BrickSoundTrait/native/f1/f13469e1-1c8d-4262-bad4-f558b0a5724e.67298.mp3"

dynamic_dir = f"{extracted_dir}/DynamicSoundTrait/native"
combo_files = []
for root, dirs, files in os.walk(dynamic_dir):
    for f in files:
        if f.endswith(".mp3"):
            combo_files.append(os.path.join(root, f))

combo_files.sort()

def to_pcm(src, dst_name):
    dst = os.path.join(out_dir, dst_name)
    cmd = [
        "ffmpeg", "-y", "-i", src,
        "-ar", "44100", "-ac", "1", "-f", "s16le", dst
    ]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    with open(dst, "rb") as f:
        return f.read()

pcm_place = to_pcm(place_src, "place.pcm")
pcm_clear = to_pcm(clear_src, "clear.pcm")
pcm_fail = to_pcm(fail_src, "fail.pcm")

combo_pcms = []
for i, cf in enumerate(combo_files[:8]):
    pcm = to_pcm(cf, f"combo_{i}.pcm")
    combo_pcms.append(pcm)

header_path = "/mnt/c/Users/amv31/Downloads/block_blast_psp/sounds.h"
with open(header_path, "w") as f:
    f.write("/* Official Block Blast Extracted Audio — 44100Hz 16-bit Mono PCM */\n\n")
    f.write("#ifndef SOUNDS_H\n#define SOUNDS_H\n\n#include <psptypes.h>\n\n")
    
    def write_array(name, data):
        samples = struct.unpack(f"<{len(data)//2}h", data)
        f.write(f"static const s16 {name}[] __attribute__((aligned(16))) = {{\n")
        for idx, s in enumerate(samples):
            f.write(f"{s},")
            if (idx + 1) % 16 == 0:
                f.write("\n")
        f.write("\n};\n")
        f.write(f"#define {name.upper()}_SAMPLES {len(samples)}\n\n")
    
    write_array("snd_place", pcm_place)
    write_array("snd_clear", pcm_clear)
    write_array("snd_fail", pcm_fail)
    
    for i, c in enumerate(combo_pcms):
        write_array(f"snd_combo_{i}", c)
    
    f.write("static const s16 *snd_combos[8] = {\n")
    for i in range(8):
        f.write(f"    snd_combo_{i},\n")
    f.write("};\n\n")
    f.write("static const int snd_combo_samples[8] = {\n")
    for i in range(8):
        f.write(f"    SND_COMBO_{i}_SAMPLES,\n")
    f.write("};\n\n")
    f.write("#endif\n")

print("Generated 44100Hz sounds.h successfully!")
