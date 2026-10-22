"""
process_project.py

CLI-хелпер для создания и обработки проектов в RailOptima:
  - Конвертирует GeoTIFF (.tif) в 16-бит PNG через gdal_translate
  - Читает метаданные высот и bbox через gdalinfo и пишет region_meta.json
  - При необходимости извлекает дороги и реки из OpenStreetMap (PBF)
  - Создает или обновляет project.json
"""

import argparse
import json
import os
import shutil
import subprocess
import sys

# Фикс кодировки для Windows-консоли
if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    except Exception:
        pass
from pathlib import Path

# Поддержка импорта из каталога скриптов
SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))


def log(msg):
    print(f"[ProcessProject] {msg}", flush=True)


def get_gdal_tool(tool_name):
    """Ищет gdal-утилиту в PATH или стандартных путях OSGeo4W/GISInternals."""
    exe = shutil.which(tool_name)
    if exe:
        return exe
    candidates = [
        f"C:\\OSGeo4W\\bin\\{tool_name}.exe",
        f"C:\\Program Files\\GDAL\\{tool_name}.exe",
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    return tool_name


def extract_metadata_from_tif(tif_path):
    gdalinfo_exe = get_gdal_tool("gdalinfo")
    cmd = [gdalinfo_exe, "-json", "-stats", str(tif_path)]
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        info = json.loads(res.stdout)
    except Exception as e:
        log(f"Ошибка gdalinfo: {e}, пробую текстовый gdalinfo...")
        cmd_text = [gdalinfo_exe, "-stats", str(tif_path)]
        res_text = subprocess.run(cmd_text, capture_output=True, text=True)
        info = {}

    bands = info.get("bands", [{}])
    min_elev = None
    max_elev = None
    if bands:
        min_elev = bands[0].get("min")
        max_elev = bands[0].get("max")

    if min_elev is None or max_elev is None:
        min_elev = 0.0
        max_elev = 1000.0

    wgs84 = info.get("wgs84Extent", {})
    coords = wgs84.get("coordinates", [[[]]])[0]
    if len(coords) >= 4:
        lons = [pt[0] for pt in coords]
        lats = [pt[1] for pt in coords]
        bbox = {
            "west": round(min(lons), 7),
            "south": round(min(lats), 7),
            "east": round(max(lons), 7),
            "north": round(max(lats), 7),
        }
    else:
        bbox = {"west": 38.5, "south": 44.5, "east": 39.5, "north": 45.5}

    size = info.get("size", [3600, 3600])
    pixel_size_m = 30.0
    geo_transform = info.get("geoTransform", [])
    if len(geo_transform) >= 2 and geo_transform[1] > 0:
        pixel_size_m = round(geo_transform[1] * 111320.0, 2)

    return {
        "min_elevation_m": round(float(min_elev), 1),
        "max_elevation_m": round(float(max_elev), 1),
        "pixel_size_m": pixel_size_m,
        "width_px": size[0],
        "height_px": size[1],
        "bbox": bbox,
    }


def convert_tif_to_png(tif_path, png_path, min_elev, max_elev):
    gdal_translate_exe = get_gdal_tool("gdal_translate")
    cmd = [
        gdal_translate_exe,
        "-ot", "UInt16",
        "-scale", str(min_elev), str(max_elev), "0", "65535",
        "-of", "PNG",
        str(tif_path), str(png_path),
    ]
    log(f"Конвертация GeoTIFF -> PNG: {' '.join(cmd)}")
    subprocess.run(cmd, check=True)
    log(f"PNG успешно создан: {png_path}")


def main():
    parser = argparse.ArgumentParser(description="Обработка файлов проекта для RailOptima")
    parser.add_argument("--dem", type=str, default=None, help="Путь к GeoTIFF файлу")
    parser.add_argument("--png", type=str, default=None, help="Путь к готовому 16-бит PNG")
    parser.add_argument("--output", "-o", type=str, required=True, help="Папка проекта")
    parser.add_argument("--name", type=str, default="Новый проект", help="Имя проекта")
    parser.add_argument("--category", type=int, default=3, help="Категория ЖД линии")
    parser.add_argument("--roads", action="store_true", help="Загрузить дороги OSM")
    parser.add_argument("--rivers", action="store_true", help="Загрузить реки OSM")

    args = parser.parse_args()
    out_dir = Path(args.output).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    png_target = out_dir / "heightmap.png"
    meta_target = out_dir / "region_meta.json"
    project_json_target = out_dir / "project.json"

    meta_data = {
        "region_name": args.name,
        "bbox": {"west": 38.5, "south": 44.5, "east": 39.5, "north": 45.5},
        "min_elevation_m": 0.0,
        "max_elevation_m": 1000.0,
        "pixel_size_m": 30.0,
        "width_px": 3600,
        "height_px": 3600,
    }

    if args.dem and os.path.exists(args.dem):
        log(f"Обработка GeoTIFF: {args.dem}")
        meta_extracted = extract_metadata_from_tif(args.dem)
        meta_data.update(meta_extracted)

        # Сохраняем копию DEM в проект
        dem_copy = out_dir / "source_dem.tif"
        if Path(args.dem).resolve() != dem_copy.resolve():
            shutil.copy2(args.dem, dem_copy)

        convert_tif_to_png(
            args.dem, png_target,
            meta_data["min_elevation_m"], meta_data["max_elevation_m"]
        )

        # Если запрошены дороги или реки — вызываем функции из prepare_region
        if args.roads or args.rivers:
            try:
                import prepare_region
                log("Извлечение геометрии дорог/рек из OSM...")
                pbf_cache = SCRIPT_DIR / "cache"
                pbf_cache.mkdir(exist_ok=True)
                pbf_url = prepare_region.resolve_pbf_url(meta_data["bbox"])
                if pbf_url:
                    pbf_path = prepare_region.download_pbf(pbf_url, str(pbf_cache))
                    road_filter = "driving" if args.roads else None
                    waterway_vals = ["river", "stream"] if args.rivers else None
                    prepare_region.extract_osm(
                        pbf_path, meta_data["bbox"], str(out_dir),
                        road_filter, waterway_vals
                    )
            except Exception as e:
                log(f"Внимание: не удалось извлечь OSM данные: {e}. Проект продолжит работу без них.")

    elif args.png and os.path.exists(args.png):
        log(f"Использую готовый PNG: {args.png}")
        if Path(args.png).resolve() != png_target.resolve():
            shutil.copy2(args.png, png_target)

        # Проверяем, есть ли рядом файл region_meta.json
        src_meta = Path(args.png).parent / "region_meta.json"
        if src_meta.exists():
            with open(src_meta, "r", encoding="utf-8") as f:
                meta_data = json.load(f)

    # Сохраняем region_meta.json
    with open(meta_target, "w", encoding="utf-8") as f:
        json.dump(meta_data, f, indent=2, ensure_ascii=False)

    # Создаем project.json
    project_data = {
        "name": args.name,
        "category": args.category,
        "has_roads": args.roads and (out_dir / "roads.geojson").exists(),
        "has_rivers": args.rivers and (out_dir / "rivers.geojson").exists(),
        "heightmap_path": "heightmap.png",
        "meta_path": "region_meta.json",
        "roads_path": "roads.geojson" if (out_dir / "roads.geojson").exists() else "",
        "rivers_path": "rivers.geojson" if (out_dir / "rivers.geojson").exists() else "",
        "has_saved_endpoints": False,
        "start_point": {"x": 0.0, "y": 0.0, "z": 0.0},
        "end_point": {"x": 0.0, "y": 0.0, "z": 0.0},
    }

    with open(project_json_target, "w", encoding="utf-8") as f:
        json.dump(project_data, f, indent=2, ensure_ascii=False)

    log(f"Проект успешно подготовлен в: {out_dir}")


if __name__ == "__main__":
    main()
