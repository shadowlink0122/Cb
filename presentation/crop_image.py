#!/usr/bin/env python3
from PIL import Image
import sys

input_path = "/Users/shadowlink/Documents/IMG_2154.jpg"
output_path = "/Users/shadowlink/Documents/git/Cb/presentation/profile.jpg"

img = Image.open(input_path)
width, height = img.size
size = min(width, height)
left = (width - size) // 2
top = (height - size) // 2
right = left + size
bottom = top + size
img_cropped = img.crop((left, top, right, bottom))
img_cropped = img_cropped.resize((400, 400), Image.Resampling.LANCZOS)
img_cropped.save(output_path, quality=95)
print(f"Cropped and saved to {output_path}")
