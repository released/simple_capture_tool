#!/usr/bin/env python3
"""Generate the project's centered white C on a green rounded frame."""

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


SIZE = 1024
OUTPUT = Path(__file__).resolve().parents[1] / "src" / "res" / "SimpleCaptureTool.ico"


def main() -> None:
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    frame = (82, 82, 942, 942)
    radius = 205
    shadow = Image.new("RGBA", image.size, (0, 0, 0, 0))
    ImageDraw.Draw(shadow).rounded_rectangle(
        (105, 105, 965, 965), radius=radius, fill=(0, 0, 0, 65)
    )
    image.alpha_composite(shadow)

    gradient = Image.new("RGBA", image.size, (0, 0, 0, 0))
    gradient_draw = ImageDraw.Draw(gradient)
    for y in range(SIZE):
        ratio = y / (SIZE - 1)
        color = (
            int(52 + (18 - 52) * ratio),
            int(199 + (122 - 199) * ratio),
            int(89 + (52 - 89) * ratio),
            255,
        )
        gradient_draw.line((0, y, SIZE, y), fill=color)
    mask = Image.new("L", image.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(frame, radius=radius, fill=255)
    gradient.putalpha(mask)
    image.alpha_composite(gradient)

    draw = ImageDraw.Draw(image)
    font_path = Path(r"C:\Windows\Fonts\segoeuib.ttf")
    font = ImageFont.truetype(str(font_path), 650)
    glyph = "C"
    bbox = draw.textbbox((0, 0), glyph, font=font, stroke_width=18)
    glyph_width = bbox[2] - bbox[0]
    glyph_height = bbox[3] - bbox[1]
    frame_width = frame[2] - frame[0]
    frame_height = frame[3] - frame[1]
    x = frame[0] + (frame_width - glyph_width) / 2 - bbox[0]
    y = frame[1] + (frame_height - glyph_height) / 2 - bbox[1]
    draw.text(
        (x, y), glyph, font=font, fill=(255, 255, 255, 255),
        stroke_width=18, stroke_fill=(10, 92, 38, 255)
    )

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    image.save(OUTPUT, format="ICO", sizes=[(16, 16), (24, 24), (32, 32),
                                             (48, 48), (64, 64), (128, 128),
                                             (256, 256)])
    print(f"Wrote {OUTPUT}")


if __name__ == "__main__":
    main()
