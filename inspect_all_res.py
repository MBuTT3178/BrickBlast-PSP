import os, subprocess

audio_dir = '/mnt/c/Users/amv31/Downloads/block_blast_apk_dump/all_audio'
res_files = sorted([f for f in os.listdir(audio_dir) if f.startswith('res_')])

print('Inspecting all 39 res_ files:')
for f in res_files:
    p = os.path.join(audio_dir, f)
    # Get duration, channels, sample rate
    c = ['ffprobe', '-v', 'error', '-show_entries', 'format=duration,size', '-of', 'csv=p=0', p]
    res = subprocess.run(c, capture_output=True, text=True).stdout.strip().split(',')
    dur = float(res[0]) if res and res[0] else 0
    sz = int(res[1]) if len(res) > 1 else 0
    
    # Analyze frequency: where is the spectral peak?
    # e.g. low pitch (thump/drop), high pitch (chime/sweep), or complex (music)
    print(f'{f:55s} | dur={dur:6.2f}s | sz={sz:6d}')
