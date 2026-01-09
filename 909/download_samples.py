#!/usr/bin/env python3
"""
TribeX - 909 Sample Downloader
Downloads high-quality open-source TR-909 samples from Archive.org
"""

import os
import subprocess
import sys
import zipfile
import shutil

# High-quality TR-909 sample pack from Archive.org (Public Domain)
SAMPLE_PACK_URL = "https://archive.org/download/RolandTR909SamplePack/Roland_TR-909_Samples.zip"

# Alternative: Use wget to download a verified sample pack
ALTERNATIVE_URL = "https://www.drummachines.be/download/tr-909-sample-pack.zip"

# Target samples mapping (TribeX has 8 drum parts)
SAMPLE_MAPPING = [
    ("kick", "Part 0 - Kick"),
    ("snare", "Part 1 - Snare"),
    ("clap", "Part 2 - Clap"),
    ("clhat", "Part 3 - Closed HH"),
    ("ohhat", "Part 4 - Open HH"),
    ("lotom", "Part 5 - Low Tom"),
    ("hitom", "Part 6 - High Tom"),
    ("crash", "Part 7 - Crash/Cymbal")
]

def download_pack():
    """Download sample pack"""
    print("📦 Downloading TR-909 sample pack...")
    
    # Try with wget first
    try:
        result = subprocess.run(
            ["wget", "-q", "--show-progress", "-O", "tr909_pack.zip", 
             "https://www.sampleradar.com/samples/0/8/2/2/0/909-sample-pack.zip"],
            capture_output=False,
            timeout=60
        )
        if os.path.exists("tr909_pack.zip") and os.path.getsize("tr909_pack.zip") > 1000:
            return True
    except:
        pass
    
    # Fallback: Clone GitHub repo with samples
    print("\n🔄 Trying alternative: Cloning sample repository...")
    try:
        subprocess.run(
            ["git", "clone", "--depth=1", 
             "https://github.com/tiagolr/drum-machine.git",
             "drum-machine-repo"],
            check=True
        )
        return "git"
    except:
        return False

def extract_from_git():
    """Extract samples from cloned git repo"""
    print("\n📂 Extracting samples from repository...")
    
    repo_path = "drum-machine-repo/TR909"
    if not os.path.exists(repo_path):
        print("❌ TR909 folder not found in repo")
        return False
    
    samples_found = []
    for filename in os.listdir(repo_path):
        if filename.endswith('.wav'):
            src = os.path.join(repo_path, filename)
            # Map to TribeX naming
            if 'BD' in filename or 'Kick' in filename:
                dst = "909_kick.wav"
            elif 'SD' in filename or 'Snare' in filename:
                dst = "909_snare.wav"
            elif 'CP' in filename or 'Clap' in filename:
                dst = "909_clap.wav"
            elif 'CH' in filename or 'CHH' in filename:
                dst = "909_clhat.wav"
            elif 'OH' in filename or 'OHH' in filename:
                dst = "909_ohhat.wav"
            elif 'LT' in filename or 'Tom' in filename and 'Low' in filename:
                dst = "909_lotom.wav"
            elif 'HT' in filename or 'Tom' in filename and 'High' in filename:
                dst = "909_hitom.wav"
            elif 'CY' in filename or 'Crash' in filename or 'Cymbal' in filename:
                dst = "909_crash.wav"
            else:
                continue
            
            shutil.copy2(src, dst)
            samples_found.append(dst)
            print(f"   ✅ {dst}")
    
    # Cleanup
    shutil.rmtree("drum-machine-repo", ignore_errors=True)
    
    return len(samples_found) > 0

def main():
    print("🎵 TribeX - 909 Sample Downloader\n")
    print("Downloading high-quality TR-909 samples...\n")
    
    result = download_pack()
    
    if result == "git":
        if extract_from_git():
            print("\n✅ Samples extracted successfully!")
        else:
            print("\n❌ Failed to extract samples")
            sys.exit(1)
    elif result:
        print("\n✅ Download complete!")
    else:
        print("\n❌ Download failed")
        print("\nManual alternative:")
        print("1. Download: https://github.com/tiagolr/drum-machine/tree/master/TR909")
        print("2. Or search: 'TR-909 samples free' on freesound.org")
        sys.exit(1)
    
    # Verify samples
    samples = [f for f in os.listdir('.') if f.startswith('909_') and f.endswith('.wav')]
    print(f"\n📊 Found {len(samples)}/8 samples")
    for s in samples:
        size = os.path.getsize(s)
        print(f"   {s}: {size:,} bytes")
    
    if len(samples) >= 8:
        print("\n🎉 All samples ready!")
        print("\nNext steps:")
        print("   cd /home/d/Schreibtisch/Tribex/909")
        print("   mkdir -p ../app/src/main/assets/samples")
        print("   cp 909_*.wav ../app/src/main/assets/samples/")
    else:
        print("\n⚠️  Some samples missing. Check the files manually.")

if __name__ == "__main__":
    main()
