import os, subprocess, struct, math

audio_dir = '/mnt/c/Users/amv31/Downloads/block_blast_apk_dump/all_audio'

combo_srcs = [
    'res_243d8edf-0495-40ec-9a97-5fc4aeca0931.e291f.ogg',  # 0.70s
    'res_ddaebabe-d002-42bc-991a-fdb113dcd186.46a7e.ogg',  # 0.94s
    'res_6165ad39-7845-47d4-ae7b-488c1579ba62.1c3cb.ogg',  # 0.95s
    'res_ac6e3347-43b8-4caa-9296-396770c0a45d.1fac6.ogg',  # 1.01s
    'res_0c34a7df-1c71-4447-a799-7134b5cd089c.d18dc.ogg',  # 1.04s
    'res_1f37155c-c28b-4d8f-8a18-fa74409387c9.e270d.ogg',  # 1.09s
    'res_67e795d7-0ddb-4e09-88de-febdef0eefcc.ac934.ogg',  # 1.09s
    'res_7cf3c039-952d-4374-b179-87678f971986.5fe1d.ogg',  # 1.10s
]

for idx, f in enumerate(combo_srcs):
    p = os.path.join(audio_dir, f)
    cmd = ['ffmpeg', '-v', 'error', '-i', p, '-f', 's16le', '-ac', '1', '-ar', '22050', '-']
    pcm = subprocess.run(cmd, capture_output=True).stdout
    count = len(pcm) // 2
    samples = struct.unpack(f'<{count}h', pcm)
    zcr = sum(1 for i in range(1, count) if (samples[i] >= 0 and samples[i-1] < 0) or (samples[i] < 0 and samples[i-1] >= 0))
    approx_freq = (zcr * 22050) / (2 * count)
    print(f'Combo {idx+1} ({f[:15]}): dur={count/22050:.2f}s, approx_freq={approx_freq:.0f}Hz')
