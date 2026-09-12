"""Generate LOW TIDE's small reusable project-authored hub surface maps."""
import math
import random
from pathlib import Path

from PIL import Image


OUTPUT = Path(__file__).resolve().parents[1] / "SourceAssets" / "HubMaterials"
SIZE = 512


def make_wood():
    random.seed(120926)
    knots = [(random.randrange(SIZE), random.randrange(SIZE), random.uniform(5.0, 11.0)) for _ in range(6)]
    pixels = []
    for y in range(SIZE):
        for x in range(SIZE):
            warp = 0.0
            knot_line = 0.0
            for kx, ky, radius in knots:
                dx = min(abs(x - kx), SIZE - abs(x - kx))
                dy = min(abs(y - ky), SIZE - abs(y - ky))
                # Narrow vertical distance and broad longitudinal distance create
                # small knots stretched with the fibre rather than round ripples.
                distance = math.hypot(dx * 0.16, dy * 1.45)
                influence = math.exp(-(distance / (radius * 2.0)) ** 2)
                warp += influence * math.atan2(dy * 1.7, dx * 0.16 + 0.01) * 7.0
                knot_line += influence * math.cos(distance * 0.72)
            slow_wander = math.sin(x * 0.017) * 3.2 + math.sin(x * 0.041 + 1.7) * 1.1
            grain = math.sin((y + slow_wander + warp) * 0.22)
            grain += 0.38 * math.sin((y + slow_wander * 0.5 + warp) * 0.47 + x * 0.006)
            fine = math.sin((y + math.sin(x * 0.028) * 1.8) * 0.93) * 0.22
            vein_phase = math.sin((y + slow_wander * 0.7 + warp) * 0.34 + math.sin(x * 0.021) * 0.6)
            dark_vein = max(0.0, abs(vein_phase) - 0.82) / 0.18
            broad = math.sin(y * 0.037 + math.sin(x * 0.009) * 1.9)
            value = 190 + grain * 14 + fine * 11 + broad * 7 + knot_line * 15 - dark_vein ** 2 * 14
            value += random.uniform(-2.0, 2.0)
            value = max(132, min(230, int(value)))
            pixels.append((value, value, value, 255))
    image = Image.new("RGBA", (SIZE, SIZE))
    image.putdata(pixels)
    image.save(OUTPUT / "T_LT_HubWoodGrain.png", optimize=True)


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    make_wood()
    print("Generated {} ({}x{}, project-authored).".format(OUTPUT / "T_LT_HubWoodGrain.png", SIZE, SIZE))


if __name__ == "__main__":
    main()
