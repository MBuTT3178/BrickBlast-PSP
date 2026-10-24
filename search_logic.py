import zipfile
import re

base_apk = r"C:\Users\amv31\Downloads\block_blast_apk_dump\base.apk"
with zipfile.ZipFile(base_apk, "r") as bz:
    main_bundles = [f for f in bz.namelist() if any(k in f for k in ["bundle", "index", "main"]) and f.endswith(".js")]
    print(f"Candidate JS bundles: {len(main_bundles)}")
    
    # Search for shape matrices or block configurations
    for f in main_bundles:
        content = bz.read(f).decode("utf-8", errors="ignore")
        if "combo" in content.lower() and ("score" in content.lower()):
            matches = re.findall(r"combo.{0,30}score", content, re.IGNORECASE)
            if matches:
                print(f"Match in {f}: {matches[:3]}")
