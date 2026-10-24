import os, subprocess, struct, math

audio_dir = '/mnt/c/Users/amv31/Downloads/block_blast_apk_dump/all_audio'

files_to_check = [
    'res_931c6d88-86a0-4afa-b03a-faebc0bb45ef.39918.ogg',
    'res_6ce8f259-c755-4ea8-8890-20af17517251.57f4b.ogg',
    'res_9131ae8f-0d09-402e-9fa5-1fc709e9e64c.ca8bb.ogg',
    'res_b7501ba6-7a59-421f-98da-a5c045705797.a8934.ogg',
    'res_0ba0fa9e-1fad-40e4-b7d7-01b2d1073067.802cd.ogg',
    'res_243d8edf-0495-40ec-9a97-5fc4aeca0931.e291f.ogg',
    'res_041b204a-893c-4ac8-bab7-f638c5eb26dd.81cac.ogg',
    'res_626d19d5-ec20-4af1-8396-2889513c9de9.f2e80.ogg',
    'res_968fa0a9-a79e-4895-ad2d-3addf5adc173.4fd53.ogg',
    'res_bff95fcf-07f4-43da-9b40-3f23a9d32692.f2b7c.ogg',
    'res_daf3681c-01cd-4859-94e7-4c72d006178b.20536.ogg',
    'res_ee085d45-da61-43d0-8240-dacb69c7807c.472d9.ogg',
    'res_0a8e960c-7e21-46ae-b8c8-7afe97a248ef.9aaca.ogg',
    'res_3105bfa1-2c4f-43fb-a0c6-d089ba26c89a.15860.ogg'
]

for f in files_to_check:
    p = os.path.join(audio_dir, f)
    cmd = ['ffmpeg', '-v', 'error', '-i', p, '-f', 's16le', '-ac', '1', '-ar', '22050', '-']
    pcm = subprocess.run(cmd, capture_output=True).stdout
    count = len(pcm) // 2
    if count == 0: continue
    samples = struct.unpack(f'<{count}h', pcm)
    max_amp = max(abs(s) for s in samples)
    rms = math.sqrt(sum(s*s for s in samples) / count)
    
    # Zero-crossing rate gives approximate pitch/rough frequency
    zcr = sum(1 for i in range(1, count) if (samples[i] >= 0 and samples[i-1] < 0) or (samples[i] < 0 and samples[i-1] >= 0))
    approx_freq = (zcr * 22050) / (2 * count)
    
    print(f'{f[:26]}: dur={count/22050:5.2f}s | max={max_amp:5d} | rms={rms:5.0f} | approx_freq={approx_freq:5.0f}Hz')
