import os, subprocess, struct, math

audio_dir = '/mnt/c/Users/amv31/Downloads/block_blast_apk_dump/all_audio'

files = sorted([f for f in os.listdir(audio_dir) if f.startswith('res_')])

for f in files:
    p = os.path.join(audio_dir, f)
    cmd = ['ffprobe', '-v', 'error', '-show_entries', 'format=duration', '-of', 'default=noprint_wrappers=1:nokey=1', p]
    dur = float(subprocess.run(cmd, capture_output=True, text=True).stdout.strip() or 0)
    if 1.35 <= dur <= 1.70:
        c2 = ['ffmpeg', '-v', 'error', '-i', p, '-f', 's16le', '-ac', '1', '-ar', '22050', '-']
        pcm = subprocess.run(c2, capture_output=True).stdout
        count = len(pcm) // 2
        samples = struct.unpack(f'<{count}h', pcm)
        # Find dominant frequency by autocorrelation
        corr = [sum(samples[j] * samples[j+lag] for j in range(0, min(2000, count - lag))) for lag in range(20, 200)]
        max_lag = corr.index(max(corr)) + 20
        pitch = 22050 / max_lag
        print(f'{f[:25]} | dur={dur:.2f}s | pitch={pitch:.1f}Hz')
