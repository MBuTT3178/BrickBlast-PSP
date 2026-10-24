import os, subprocess, struct

audio_dir = '/mnt/c/Users/amv31/Downloads/block_blast_apk_dump/all_audio'
files = [
    'res_ee085d45-da61-43d0-8240-dacb69c7807c.472d9.ogg',
    'res_626d19d5-ec20-4af1-8396-2889513c9de9.f2e80.ogg',
    'res_daf3681c-01cd-4859-94e7-4c72d006178b.20536.ogg',
    'res_041b204a-893c-4ac8-bab7-f638c5eb26dd.81cac.ogg',
    'res_41701843-6eed-4cb5-93fe-c0d982d6c941.23ef2.ogg',
    'res_4f5fb60f-3b65-47e1-b5c8-a68a081bde0d.4c061.ogg',
    'res_d712293f-48f6-403b-af36-c99239d38acc.443b7.ogg',
    'res_a3f506ef-6764-48aa-97a6-e4ad965fc45d.80ab5.ogg',
    'res_968fa0a9-a79e-4895-ad2d-3addf5adc173.4fd53.ogg',
    'res_bcbf9f22-56f2-45f6-8d77-6da9ea33abc4.f6fb8.ogg',
    'res_bff95fcf-07f4-43da-9b40-3f23a9d32692.f2b7c.ogg'
]

results = []
for f in files:
    p = os.path.join(audio_dir, f)
    cmd = ['ffmpeg', '-v', 'error', '-i', p, '-f', 's16le', '-ac', '1', '-ar', '22050', '-']
    pcm = subprocess.run(cmd, capture_output=True).stdout
    count = len(pcm) // 2
    samples = struct.unpack(f'<{count}h', pcm)
    corr = [sum(samples[j] * samples[j+lag] for j in range(0, min(2000, count - lag))) for lag in range(20, 200)]
    max_lag = corr.index(max(corr)) + 20
    pitch = 22050 / max_lag
    results.append((pitch, f))

results.sort()
print('Sorted ascending scale notes:')
for idx, (pitch, f) in enumerate(results):
    print(f'Note {idx+1}: {pitch:6.1f}Hz -> {f}')
