#!/usr/bin/env python3
import os
import subprocess
import struct

audio_dir = "/mnt/c/Users/amv31/Downloads/block_blast_apk_dump/all_audio"
out_dir = "/mnt/c/Users/amv31/Downloads/block_blast_psp/real_sounds"
os.makedirs(out_dir, exist_ok=True)

# 1. Place / drop click (0.10s)
place_src = f"{audio_dir}/res_931c6d88-86a0-4afa-b03a-faebc0bb45ef.39918.ogg"

# 2. Snap / pickup (0.17s)
snap_src = f"{audio_dir}/res_6ce8f259-c755-4ea8-8890-20af17517251.57f4b.ogg"

# 3. Line clear laser sweep (0.34s)
clear_src = f"{audio_dir}/res_b7501ba6-7a59-421f-98da-a5c045705797.a8934.ogg"

# 4. Game Over defeat sound (2.85s)
gameover_src = f"{audio_dir}/res_0a8e960c-7e21-46ae-b8c8-7afe97a248ef.9aaca.ogg"

# 5. Authentic 10 Ascending Musical Scale Notes for STREAKS / COMBOS
scale_srcs = [
    f"{audio_dir}/res_ee085d45-da61-43d0-8240-dacb69c7807c.472d9.ogg",  # Note 1 (124 Hz)
    f"{audio_dir}/res_626d19d5-ec20-4af1-8396-2889513c9de9.f2e80.ogg",  # Note 2 (139 Hz)
    f"{audio_dir}/res_daf3681c-01cd-4859-94e7-4c72d006178b.20536.ogg",  # Note 3 (147 Hz)
    f"{audio_dir}/res_041b204a-893c-4ac8-bab7-f638c5eb26dd.81cac.ogg",  # Note 4 (156 Hz)
    f"{audio_dir}/res_41701843-6eed-4cb5-93fe-c0d982d6c941.23ef2.ogg",  # Note 5 (185 Hz)
    f"{audio_dir}/res_d712293f-48f6-403b-af36-c99239d38acc.443b7.ogg",  # Note 6 (197 Hz)
    f"{audio_dir}/res_a3f506ef-6764-48aa-97a6-e4ad965fc45d.80ab5.ogg",  # Note 7 (208 Hz)
    f"{audio_dir}/res_968fa0a9-a79e-4895-ad2d-3addf5adc173.4fd53.ogg",  # Note 8 (394 Hz)
    f"{audio_dir}/res_bcbf9f22-56f2-45f6-8d77-6da9ea33abc4.f6fb8.ogg",  # Note 9 (490 Hz)
    f"{audio_dir}/res_bff95fcf-07f4-43da-9b40-3f23a9d32692.f2b7c.ogg",  # Note 10 (760 Hz)
]

# 6. Background Music loop (30s)
bgm_src = f"{audio_dir}/res_3105bfa1-2c4f-43fb-a0c6-d089ba26c89a.15860.ogg"

def to_pcm(src, dst_name, rate=22050):
    dst = os.path.join(out_dir, dst_name)
    cmd = [
        "ffmpeg", "-y", "-i", src,
        "-ar", str(rate), "-ac", "1", "-f", "s16le", dst
    ]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    with open(dst, "rb") as f:
        return f.read()

pcm_place = to_pcm(place_src, "place.pcm")
pcm_snap  = to_pcm(snap_src, "snap.pcm")
pcm_clear = to_pcm(clear_src, "clear.pcm")
pcm_gameover = to_pcm(gameover_src, "gameover.pcm")

scale_pcms = []
for i, cs in enumerate(scale_srcs):
    scale_pcms.append(to_pcm(cs, f"scale_{i}.pcm"))

pcm_bgm = to_pcm(bgm_src, "bgm.pcm", rate=22050)
print(f"BGM converted: {len(pcm_bgm)} bytes ({len(pcm_bgm)//2} samples, {len(pcm_bgm)//2/22050:.1f}s)")

# Generate real_sounds.h
header_path = "/mnt/c/Users/amv31/Downloads/block_blast_psp/real_sounds.h"
with open(header_path, "w") as f:
    f.write("/* Official Block Blast In-Game Sounds & Music — 22050Hz 16-bit Mono PCM */\n\n")
    f.write("#ifndef REAL_SOUNDS_H\n#define REAL_SOUNDS_H\n\n#include <psptypes.h>\n\n")

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
    write_array("snd_snap", pcm_snap)
    write_array("snd_clear", pcm_clear)
    write_array("snd_gameover", pcm_gameover)

    for i, c in enumerate(scale_pcms):
        write_array(f"snd_scale_{i}", c)

    f.write("#define NUM_SCALE_NOTES 10\n")
    f.write("static const s16 *snd_scale[NUM_SCALE_NOTES] = {\n")
    for i in range(10):
        f.write(f"    snd_scale_{i},\n")
    f.write("};\n\n")
    f.write("static const int snd_scale_samples[NUM_SCALE_NOTES] = {\n")
    for i in range(10):
        f.write(f"    SND_SCALE_{i}_SAMPLES,\n")
    f.write("};\n\n")

    # Write BGM array
    write_array("snd_bgm", pcm_bgm)

    f.write("#endif\n")

print(f"Generated {header_path} successfully!")
