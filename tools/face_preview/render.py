"""Turn faces.txt into faces.png (needs Pillow: pip install pillow)."""
from PIL import Image, ImageDraw

lines = open("faces.txt").read().split("\n")
frames, i = [], 0
while i < len(lines) and lines[i]:
    frames.append((lines[i], lines[i + 1 : i + 65]))
    i += 65

cols, scale, pad = 4, 3, 24
width = cols * (128 * scale + 12)
height = ((len(frames) + cols - 1) // cols) * (64 * scale + pad + 12)
img = Image.new("RGB", (width, height), (30, 30, 30))
draw = ImageDraw.Draw(img)
for k, (name, rows) in enumerate(frames):
    ox = (k % cols) * (128 * scale + 12) + 6
    oy = (k // cols) * (64 * scale + pad + 12) + pad
    draw.text((ox, oy - 16), name, fill=(220, 220, 220))
    draw.rectangle([ox - 1, oy - 1, ox + 128 * scale, oy + 64 * scale], outline=(80, 80, 80), fill=(0, 0, 0))
    for y, row in enumerate(rows):
        for x, c in enumerate(row):
            if c == "1":
                draw.rectangle([ox + x * scale, oy + y * scale, ox + x * scale + scale - 1, oy + y * scale + scale - 1], fill=(120, 220, 255))
img.save("faces.png")
print(f"faces.png: {len(frames)} faces")
