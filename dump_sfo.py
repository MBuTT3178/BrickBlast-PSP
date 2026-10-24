import struct

def dump(path):
    print("=== DUMPING:", path)
    with open(path, "rb") as f:
        data = f.read()
    magic, version, key_off, val_off, num_entries = struct.unpack("<4sIIII", data[:20])
    for i in range(num_entries):
        k_off, val_fmt, val_len, val_max, v_off = struct.unpack("<HHIII", data[20+i*16:20+(i+1)*16])
        key = data[key_off+k_off:].split(b"\0")[0].decode("ascii")
        val = data[val_off+v_off:val_off+v_off+val_len]
        print(f"  {key} = {val}")

dump("/home/abdul/ref_eboot/PARAM.SFO")
dump("/home/abdul/mcpsp/PARAM.SFO")
dump("/mnt/c/Users/amv31/Downloads/block_blast_psp/PARAM.SFO")
