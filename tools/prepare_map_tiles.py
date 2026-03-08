import os
import math
import requests
import struct
import argparse
from PIL import Image
from io import BytesIO
import time

# --- DEFAULTS ---
DEFAULT_LAT = 52.5200   # Berlin, Germany
DEFAULT_LON = 13.4050
DEFAULT_ZOOM = "15"
DEFAULT_RADIUS = 1
OUTPUT_DIR = "map_tiles"
USER_AGENT = "ESP-SATDUMP-TileFetcher/1.0 (https://github.com/your-username/ESP-SATDUMP)"

def deg2num(lat_deg, lon_deg, zoom):
    lat_rad = math.radians(lat_deg)
    n = 2.0 ** zoom
    xtile = int((lon_deg + 180.0) / 360.0 * n)
    ytile = int((1.0 - math.log(math.tan(lat_rad) + (1 / math.cos(lat_rad))) / math.pi) / 2.0 * n)
    return (xtile, ytile)

def convert_to_rgb565(img_data):
    img = Image.open(BytesIO(img_data)).convert('RGB').resize((256, 256))
    buf = bytearray()
    for y in range(256):
        for x in range(256):
            r, g, b = img.getpixel((x, y))
            # RGB888 to RGB565 (16-bit)
            rgb = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            # ESP32/TFT_eSPI expects little-endian for raw binary file reads into uint16_t
            buf.extend(struct.pack('<H', rgb)) 
    return buf

def download_tiles(lat, lon, zoom_levels, radius):
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    headers = {"User-Agent": USER_AGENT}

    for zoom in zoom_levels:
        xtile, ytile = deg2num(lat, lon, zoom)
        print(f"\n--- Zoom Level {zoom} ---")
        
        for dx in range(-radius, radius + 1):
            for dy in range(-radius, radius + 1):
                tx, ty = xtile + dx, ytile + dy
                url = f"https://tile.openstreetmap.org/{zoom}/{tx}/{ty}.png"
                
                # We need to preserve the /map/ structure for the SD card
                # The ESP32 code expects /map/{z}/{x}/{y}.bin
                target_dir = os.path.join(OUTPUT_DIR, "Tiles", str(zoom), str(tx))
                os.makedirs(target_dir, exist_ok=True)
                target_file = os.path.join(target_dir, f"{ty}.bin")
                
                if os.path.exists(target_file):
                    print(f"Skipping {zoom}/{tx}/{ty} (exists)")
                    continue

                print(f"Downloading {url} ...")
                try:
                    response = requests.get(url, headers=headers)
                    if response.status_code == 200:
                        rgb565_data = convert_to_rgb565(response.content)
                        with open(target_file, "wb") as f:
                            f.write(rgb565_data)
                    else:
                        print(f"  Error {response.status_code}")
                    time.sleep(0.5)
                except Exception as e:
                    print(f"  Failed: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Download and convert OSM tiles for ESP-SATDUMP.")
    parser.add_argument("--lat", type=float, default=DEFAULT_LAT, help="Latitude (default: Berlin)")
    parser.add_argument("--lon", type=float, default=DEFAULT_LON, help="Longitude (default: Berlin)")
    parser.add_argument("--zoom", type=str, default=DEFAULT_ZOOM, help="Zoom levels (comma-separated, e.g., 14,15,16)")
    parser.add_argument("--radius", type=int, default=DEFAULT_RADIUS, help="Tile radius around center (default: 1)")

    args = parser.parse_args()
    
    zooms = [int(z.strip()) for z in args.zoom.split(",")]
    
    print(f"Starting download for Lat:{args.lat} Lon:{args.lon} with Radius:{args.radius}")
    download_tiles(args.lat, args.lon, zooms, args.radius)
    
    print(f"\nDone! Tiles are in '{OUTPUT_DIR}/Tiles/'.")
    print(f"Copy the 'Tiles/' folder to your SD card root.")
