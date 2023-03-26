from glob import glob
from PIL import Image

import numpy as np
import re

def get_basename(s: str):
    s = re.sub(r"(raw_)?images/", "", s)
    s = re.sub(r"\.(csv|png)", "", s)
    return s

def convert_file(fn: str):
    print("Converting", fn)
    in_file = "raw_images/" + fn + ".csv"
    out_file = "images/" + fn + ".png"

    data = np.loadtxt(in_file, delimiter=',', dtype=np.uint32)
    data = data.reshape((data.shape[0], -1, 3))[::-1]
    img = Image.fromarray((data / 16777216).astype(np.uint8), "RGB")
    img.save(out_file)

if __name__ == "__main__":
    raw_files = sorted(glob('raw_images/*.csv'))
    img_files = sorted(glob('images/*.png'))

    to_process = set([get_basename(fn) for fn in raw_files]) - set([get_basename(fn) for fn in img_files])
    print(to_process)

    for fn in to_process:
        convert_file(fn)
