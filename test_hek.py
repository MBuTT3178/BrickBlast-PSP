import zipfile, os

with zipfile.ZipFile('/mnt/c/Users/amv31/Downloads/block_blast_apk_dump/base.apk') as z:
    for name in z.namelist():
        if name.endswith('.json') and 'config' in name:
            d = z.read(name)
            if d.startswith(b'HEK:'):
                print('Testing:', name)
                payload = d[8:]
                # Check if payload is compressed or encrypted
                # Test zlib / gzip / lzma / brotli
                for mod, desc in [('zlib', 'zlib'), ('bz2', 'bz2'), ('lzma', 'lzma')]:
                    try:
                        import importlib
                        m = importlib.import_module(mod)
                        dec = m.decompress(payload)
                        print(f'SUCCESS with {desc}:', dec[:50])
                        break
                    except:
                        pass
                break
