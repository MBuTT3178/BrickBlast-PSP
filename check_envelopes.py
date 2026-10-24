import os, subprocess, struct, math

audio_dir = '/mnt/c/Users/amv31/Downloads/block_blast_apk_dump/all_audio'
files = [
    'res_931c6d88-86a0-4afa-b03a-faebc0bb45ef.39918.ogg',
    'res_6ce8f259-c755-4ea8-8890-20af17517251.57f4b.ogg',
    'res_9131ae8f-0d09-402e-9fa5-1fc709e9e64c.ca8bb.ogg',
    'res_b7501ba6-7a59-421f-98da-a5c045705797.a8934.ogg',
    'res_0ba0fa9e-1fad-40e4-b7d7-01b2d1073067.802cd.ogg',
    'res_0a8e960c-7e21-46ae-b8c8-7afe97a248ef.9aaca.ogg',
    'res_c59e2c71-131e-4521-a721-d9c14373a41c.2296a.ogg',
    'res_e44a494b-cd59-497c-9ec8-0d0870dd2f53.bd9e7.ogg',
    'res_cfb47fa7-9ffb-44ae-9f6d-2bad4bd99555.5eb64.ogg',
    'res_3105bfa1-2c4f-43fb-a0c6-d089ba26c89a.15860.ogg'
]

for f in files:
    p = os.path.join(audio_dir, f)
    cmd = ['ffmpeg', '-v', 'error', '-i', p, '-f', 's16le', '-ac', '1', '-ar', '22050', '-']
    pcm = subprocess.run(cmd, capture_output=True).stdout
    count = len(pcm) // 2
    samples = struct.unpack(f'<{count}h', pcm)
    dur = count / 22050
    step = count // 4
    rms_parts = [int(math.sqrt(sum(s*s for s in samples[i*step:(i+1)*step])/step)) for i in range(4)]
    print(f'{f[:26]} ({dur:.2f}s): envelope={rms_parts}')
