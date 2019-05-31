#!/usr/bin/env python
# Simple zip implementation (can be used e.g. on Windows)
import os
import sys
import zipfile

def zipdir(path, ziph):
    # ziph is zipfile handle
    for root, dirs, files in os.walk(path):
        for file in files:
            ziph.write(os.path.join(root, file))

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: zip.py DESTINATION DIR_TO_ZIP")
        sys.exit(1)
    destination = sys.argv[1]
    dir_to_zip = sys.argv[2]
    zipf = zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED)
    zipdir(dir_to_zip, zipf)
    zipf.close()
