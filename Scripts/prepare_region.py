"""
prepare_region.py

Автоматическая подготовка данных для RailOptima:
    1. Если готовый DEM уже есть — использует его, не качает повторно.
    2. Иначе скачивает DEM из OpenTopography по bbox.
    3. Конвертирует GeoTIFF → 16-бит PNG через gdal_translate.
    4. Читает bbox и метаданные из GeoTIFF.
    5. Ищет PBF OSM: явный URL > osmfinder > перебор регионов.
    6. Скачивает PBF (или берёт локальный / из кэша).
    7. Извлекает дороги → roads.geojson.
    8. Извлекает реки → rivers.geojson.
    9. Всё кладёт в RawContent/.
   10. Копирует готовые файлы в Content/RawContent/, чтобы
       Unreal Engine находил их и в редакторе, и в собранном билде.

Зависимости:
    pip install requests python-dotenv pyrosm geopandas osmfinder shapely

GDAL в PATH (gdal_translate, gdalinfo):
    Windows: gisinternals.com или OSGeo4W
    Linux:   sudo apt install gdal-bin

Запуск:
    python prepare_region.py --bbox 38.5 44.5 39.5 45.5 --region krasnodar
    python prepare_region.py --region krasnodar      # если DEM уже есть
    python prepare_region.py                          # всё из .env
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import requests

# ─── Фикс кодировки для Windows-консоли ────────────────────────
if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    except Exception:
        pass

try:
    from dotenv import load_dotenv
    HAS_DOTENV = True
except ImportError:
    HAS_DOTENV = False


RUSSIA_FED_DISTRICTS = [
    ("south-fed-district",
     "https://download.geofabrik.de/russia/south-fed-district-latest.osm.pbf"),
    ("north-caucasus-fed-district",
     "https://download.geofabrik.de/russia/north-caucasus-fed-district-latest.osm.pbf"),
    ("central-fed-district",
     "https://download.geofabrik.de/russia/central-fed-district-latest.osm.pbf"),
    ("northwestern-fed-district",
     "https://download.geofabrik.de/russia/northwestern-fed-district-latest.osm.pbf"),
    ("volga-fed-district",
     "https://download.geofabrik.de/russia/volga-fed-district-latest.osm.pbf"),
    ("ural-fed-district",
     "https://download.geofabrik.de/russia/ural-fed-district-latest.osm.pbf"),
    ("siberian-fed-district",
     "https://download.geofabrik.de/russia/siberian-fed-district-latest.osm.pbf"),
    ("far-eastern-fed-district",
     "https://download.geofabrik.de/russia/far-eastern-fed-district-latest.osm.pbf"),
]


# ─────────────────────────────────────────────────────────────────
#  Утилиты
# ─────────────────────────────────────────────────────────────────

def log(msg):
    try:
        print(f"[prepare_region] {msg}")
        sys.stdout.flush()
    except UnicodeEncodeError:
        print(f"[prepare_region] {msg.encode('ascii', 'replace').decode('ascii')}")
        sys.stdout.flush()


def ensure_dir(path):
    Path(path).mkdir(parents=True, exist_ok=True)


def load_config(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def save_json(data, path):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)


def check_gdal():
    for tool in ("gdal_translate", "gdalinfo"):
        try:
            subprocess.run([tool, "--version"],
                           check=True, capture_output=True)
        except (FileNotFoundError, subprocess.CalledProcessError):
            log(f"Утилита {tool} не найдена в PATH.")
            return False
    return True


# ─────────────────────────────────────────────────────────────────
#  Работа с GeoTIFF
# ─────────────────────────────────────────────────────────────────

def find_existing_dem(output_dir, region_name):
    exact = os.path.join(output_dir, f"{region_name}_dem.tif")
    if os.path.exists(exact):
        log(f"Найден готовый DEM: {exact}")
        return exact
    try:
        for f in os.listdir(output_dir):
            if f.endswith("_dem.tif"):
                path = os.path.join(output_dir, f)
                log(f"Найден DEM в папке: {path}")
                return path
    except FileNotFoundError:
        pass
    return None


def read_bbox_from_tif(tif_path):
    try:
        result = subprocess.run(
            ["gdalinfo", "-json", tif_path],
            check=True, capture_output=True, text=True, encoding="utf-8"
        )
        info = json.loads(result.stdout)
    except (subprocess.CalledProcessError, json.JSONDecodeError) as e:
        log(f"Не удалось прочитать {tif_path}: {e}")
        return None

    corners = info.get("cornerCoordinates", {})
    if not corners:
        return None

    lons, lats = [], []
    for key in ("upperLeft", "lowerLeft", "upperRight", "lowerRight"):
        if key in corners:
            lons.append(corners[key][0])
            lats.append(corners[key][1])

    if not lons:
        return None

    bbox = {
        "west":  min(lons),
        "south": min(lats),
        "east":  max(lons),
        "north": max(lats),
    }
    log(f"bbox из GeoTIFF: {bbox}")
    return bbox


def extract_dem_metadata(tif_path):
    log(f"Читаю метаданные DEM: {tif_path}")

    try:
        subprocess.run(
            ["gdalinfo", "-stats", tif_path],
            check=True, capture_output=True, text=True, encoding="utf-8"
        )
    except subprocess.CalledProcessError as e:
        log(f"gdalinfo -stats завершился с ошибкой: {(e.stderr or '')[:300]}")

    result = subprocess.run(
        ["gdalinfo", "-json", tif_path],
        check=True, capture_output=True, text=True, encoding="utf-8"
    )
    info = json.loads(result.stdout)

    min_elev, max_elev = None, None
    for band in info.get("bands", []):
        if "minimum" in band:
            v = band["minimum"]
            min_elev = v if min_elev is None else min(min_elev, v)
        if "maximum" in band:
            v = band["maximum"]
            max_elev = v if max_elev is None else max(max_elev, v)

    if min_elev is None or max_elev is None:
        log("Не удалось получить Min/Max высоты. Ставлю дефолты.")
        min_elev = min_elev if min_elev is not None else 0.0
        max_elev = max_elev if max_elev is not None else 3000.0

    geo_transform = info.get("geoTransform", [0, 0.000277, 0, 0, 0, -0.000277])
    pixel_size_deg = abs(geo_transform[1])
    pixel_size_m = pixel_size_deg * 111320.0
    size = info.get("size", [0, 0])

    meta = {
        "min_elevation_m": round(min_elev, 1),
        "max_elevation_m": round(max_elev, 1),
        "pixel_size_m": round(pixel_size_m, 2),
        "width": size[0],
        "height": size[1],
    }
    log(f"Метаданные: {meta}")
    return meta


def convert_tif_to_png(tif_path, png_path, min_elev, max_elev):
    log(f"Конвертирую GeoTIFF → PNG: {png_path}")
    cmd = [
        "gdal_translate",
        "-ot", "UInt16",
        "-scale", str(min_elev), str(max_elev), "0", "65535",
        "-of", "PNG",
        tif_path, png_path,
    ]
    subprocess.run(cmd, check=True, capture_output=True)
    size_mb = os.path.getsize(png_path) / (1024 * 1024)
    log(f"PNG сохранён: {png_path} ({size_mb:.1f} MB)")


# ─────────────────────────────────────────────────────────────────
#  Поиск подходящего PBF
# ─────────────────────────────────────────────────────────────────

def resolve_pbf_url(bbox):
    try:
        import osmfinder
        from shapely.geometry import box
    except ImportError:
        log("osmfinder/shapely не установлены — пропускаю автопоиск.")
        return None

    try:
        geom = box(bbox["west"], bbox["south"], bbox["east"], bbox["north"])
        log(f"Ищу PBF Geofabrik для bbox "
            f"({bbox['west']:.4f}, {bbox['south']:.4f}, "
            f"{bbox['east']:.4f}, {bbox['north']:.4f})...")
        result = osmfinder.find(geom, source="Geofabrik")
        if result and getattr(result, "extract", None):
            extract_id = getattr(result.extract, "id", "unknown")
            extract_url = getattr(result.extract, "url", None)
            log(f"Найден регион: {extract_id}")
            if extract_url:
                log(f"URL: {extract_url}")
                return extract_url
        log("osmfinder не нашёл подходящий регион.")
        return None
    except Exception as e:
        log(f"Ошибка osmfinder: {e}")
        return None


def pbf_has_data(pbf_path, bbox, road_filter):
    try:
        from pyrosm import OSM
        bbox_list = [bbox["west"], bbox["south"], bbox["east"], bbox["north"]]
        osm = OSM(pbf_path, bounding_box=bbox_list)

        try:
            roads = osm.get_network(network_type=road_filter)
            if roads is not None and len(roads) > 0:
                return True
        except Exception:
            pass

        try:
            roads = osm.get_data_by_custom_criteria(
                custom_filter={"highway": True},
                filter_type="keep",
                keep_ways=True,
                keep_relations=False,
                keep_nodes=False,
            )
            if roads is not None and len(roads) > 0:
                return True
        except Exception:
            pass

        return False
    except Exception as e:
        log(f"  Ошибка проверки PBF: {e}")
        return False


def find_pbf_by_trial(bbox, cache_dir, road_filter):
    log("Перебираю федеральные округа России по списку.")
    for name, url in RUSSIA_FED_DISTRICTS:
        log(f"Пробую регион: {name}")
        try:
            pbf_path = download_pbf(url, cache_dir)
        except Exception as e:
            log(f"  Скачивание не удалось: {e}")
            continue

        log("  Проверяю, есть ли данные для bbox...")
        if pbf_has_data(pbf_path, bbox, road_filter):
            log(f"  Регион {name} содержит данные для bbox.")
            return pbf_path
        else:
            log(f"  Регион {name} не покрывает bbox, иду дальше.")

    return None


# ─────────────────────────────────────────────────────────────────
#  DEM из OpenTopography
# ─────────────────────────────────────────────────────────────────

def fetch_dem(bbox, dem_type, api_key, output_tif):
    log(f"Скачиваю DEM ({dem_type}) для bbox {bbox}...")
    url = "https://portal.opentopography.org/API/globaldem"
    params = {
        "demtype": dem_type,
        "south": bbox["south"],
        "north": bbox["north"],
        "west": bbox["west"],
        "east": bbox["east"],
        "outputFormat": "GTiff",
        "API_Key": api_key,
    }
    r = requests.get(url, params=params, stream=True, timeout=300)
    if r.status_code == 401:
        log("HTTP 401: Неверный API-ключ OpenTopography.")
        sys.exit(1)
    elif r.status_code == 403:
        log("HTTP 403: Доступ запрещён.")
        sys.exit(1)
    elif r.status_code == 429:
        log("HTTP 429: Превышен дневной лимит запросов.")
        sys.exit(1)
    r.raise_for_status()

    with open(output_tif, "wb") as f:
        for chunk in r.iter_content(chunk_size=8192):
            f.write(chunk)

    size_mb = os.path.getsize(output_tif) / (1024 * 1024)
    log(f"DEM сохранён: {output_tif} ({size_mb:.1f} MB)")
    return output_tif


# ─────────────────────────────────────────────────────────────────
#  Скачивание PBF
# ─────────────────────────────────────────────────────────────────

def download_pbf(url, cache_dir):
    ensure_dir(cache_dir)
    filename = url.split("/")[-1]
    local_path = os.path.join(cache_dir, filename)

    if os.path.exists(local_path):
        size_mb = os.path.getsize(local_path) / (1024 * 1024)
        log(f"PBF уже в кэше: {local_path} ({size_mb:.1f} MB)")
        return local_path

    log(f"Скачиваю PBF: {url}")
    log("  (Это может занять несколько минут.)")

    r = requests.get(url, stream=True, timeout=600)
    r.raise_for_status()

    total = int(r.headers.get("content-length", 0))
    downloaded = 0
    last_reported = 0

    with open(local_path, "wb") as f:
        for chunk in r.iter_content(chunk_size=1024 * 1024):
            f.write(chunk)
            downloaded += len(chunk)
            if total > 0:
                percent = int(downloaded / total * 100)
                if percent >= last_reported + 10:
                    last_reported = percent
                    log(f"  ... {percent}% "
                        f"({downloaded / (1024*1024):.0f} / "
                        f"{total / (1024*1024):.0f} MB)")

    size_mb = os.path.getsize(local_path) / (1024 * 1024)
    log(f"PBF скачан: {local_path} ({size_mb:.1f} MB)")
    return local_path


# ─────────────────────────────────────────────────────────────────
#  Извлечение OSM
# ─────────────────────────────────────────────────────────────────

def _save_geojson(gdf, path):
    gdf.to_file(path, driver="GeoJSON", encoding="utf-8")
    size_mb = os.path.getsize(path) / (1024 * 1024)
    return size_mb


def extract_roads(osm, output_dir):
    roads = None

    # Способ 1: get_network("driving").
    try:
        log("Пробую get_network('driving')...")
        roads = osm.get_network(network_type="driving")
        if roads is None or len(roads) == 0:
            log("  get_network('driving') вернул 0 объектов.")
            roads = None
        else:
            log(f"  get_network('driving'): {len(roads)} объектов.")
    except Exception as e:
        log(f"  get_network('driving') ошибка: {e}")
        roads = None

    # Способ 2: кастомный фильтр highway=*.
    if roads is None or len(roads) == 0:
        try:
            log("Пробую кастомный фильтр highway=*...")
            roads = osm.get_data_by_custom_criteria(
                custom_filter={"highway": True},
                filter_type="keep",
                keep_ways=True,
                keep_relations=False,
                keep_nodes=False,
            )
            if roads is not None and len(roads) > 0:
                log(f"  Кастомный фильтр: {len(roads)} объектов.")
            else:
                log("  Кастомный фильтр вернул 0 объектов.")
                roads = None
        except Exception as e:
            log(f"  Кастомный фильтр ошибка: {e}")
            roads = None

    if roads is None or len(roads) == 0:
        log("Дороги не найдены ни одним способом.")
        return

    try:
        counts = roads.geometry.geom_type.value_counts().to_dict()
        log(f"  Типы геометрий в данных: {counts}")
    except Exception as e:
        log(f"  Не удалось посчитать типы геометрий: {e}")

    try:
        log(f"  CRS: {roads.crs}")
        log(f"  Пример геометрии: {roads.geometry.iloc[0]}")
    except Exception as e:
        log(f"  Не удалось получить пример: {e}")

    linear_types = ["LineString", "MultiLineString", "Polygon", "MultiPolygon"]
    roads_linear = roads[roads.geometry.geom_type.isin(linear_types)]

    if len(roads_linear) == 0:
        log("  Нет ни LineString, ни MultiLineString, ни Polygon.")
        return

    log(f"  Оставляем линейных и площадных: {len(roads_linear)}")

    try:
        exploded = roads_linear.explode(index_parts=False)
    except Exception as e:
        log(f"  Ошибка explode(): {e}")
        return

    exploded = exploded[exploded.geometry.geom_type == "LineString"]

    if len(exploded) == 0:
        log("  После explode не осталось LineString.")
        return

    roads_path = os.path.join(output_dir, "roads.geojson")
    try:
        size_mb = _save_geojson(exploded, roads_path)
        log(f"Дорог: {len(exploded)} → {roads_path} ({size_mb:.1f} MB)")
    except Exception as e:
        log(f"Ошибка сохранения GeoJSON: {e}")


def extract_rivers(osm, output_dir, waterway_values):
    log(f"Извлекаю реки (waterway in {waterway_values})...")
    try:
        rivers = osm.get_data_by_custom_criteria(
            custom_filter={"waterway": waterway_values},
            filter_type="keep",
            keep_ways=True,
            keep_relations=False,
            keep_nodes=False,
        )
        if rivers is None or len(rivers) == 0:
            log("Реки не найдены.")
            return

        log(f"  Рек (сырых): {len(rivers)}")
        try:
            counts = rivers.geometry.geom_type.value_counts().to_dict()
            log(f"  Типы геометрий рек: {counts}")
        except Exception:
            pass

        linear_types = ["LineString", "MultiLineString"]
        rivers_linear = rivers[rivers.geometry.geom_type.isin(linear_types)]

        if len(rivers_linear) == 0:
            log("  Среди рек нет линейных объектов.")
            return

        exploded = rivers_linear.explode(index_parts=False)
        exploded = exploded[exploded.geometry.geom_type == "LineString"]

        if len(exploded) == 0:
            log("  После explode не осталось LineString у рек.")
            return

        rivers_path = os.path.join(output_dir, "rivers.geojson")
        size_mb = _save_geojson(exploded, rivers_path)
        log(f"Рек: {len(exploded)} → {rivers_path} ({size_mb:.1f} MB)")
    except Exception as e:
        log(f"Ошибка при извлечении рек: {e}")


def extract_osm(pbf_path, bbox, output_dir, road_filter, waterway_values):
    try:
        from pyrosm import OSM
    except ImportError:
        log("pyrosm не установлен. Установи: pip install pyrosm geopandas")
        sys.exit(1)

    log(f"Читаю PBF: {pbf_path}")

    bbox_list = [bbox["west"], bbox["south"], bbox["east"], bbox["north"]]
    osm = OSM(pbf_path, bounding_box=bbox_list)

    extract_roads(osm, output_dir)
    extract_rivers(osm, output_dir, waterway_values)


# ─────────────────────────────────────────────────────────────────
#  Копирование в Content/RawContent
# ─────────────────────────────────────────────────────────────────

def copy_to_content_raw(output_dir):
    """
    Копирует готовые файлы из RawContent/ в Content/RawContent/.
    В редакторе UE и в собранном билде используются пути через
    ProjectContentDir() = Content/. Чтобы не было расхождений,
    кладём копию рядом с ассетами.
    """
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent
    content_dir = project_root / "Content"
    content_raw_dir = content_dir / "RawContent"

    if not content_dir.exists():
        log(f"Папка Content/ не найдена: {content_dir}. Пропускаю копирование.")
        return

    content_raw_dir.mkdir(parents=True, exist_ok=True)

    names = ("heightmap.png", "region_meta.json",
             "roads.geojson", "rivers.geojson")

    copied = []
    for name in names:
        src = Path(output_dir) / name
        if not src.exists():
            continue

        dst = content_raw_dir / name
        try:
            shutil.copy2(src, dst)
            size_mb = dst.stat().st_size / (1024 * 1024)
            copied.append(f"{name} ({size_mb:.1f} MB)")
        except Exception as e:
            log(f"Не удалось скопировать {name}: {e}")

    if copied:
        log(f"Скопировано в {content_raw_dir}:")
        for c in copied:
            log(f"  • {c}")
    else:
        log(f"Нечего копировать в {content_raw_dir}.")


# ─────────────────────────────────────────────────────────────────
#  Главная функция
# ─────────────────────────────────────────────────────────────────

def prepare_region(config):
    output_dir = config["output_dir"]
    region_name = config.get("region_name", "region")
    bbox_from_config = config["bbox"]

    ensure_dir(output_dir)

    if not check_gdal():
        log("GDAL (gdal_translate / gdalinfo) не найден в PATH.")
        log("  Windows: gisinternals.com или OSGeo4W.")
        log("  Linux:   sudo apt install gdal-bin")
        sys.exit(1)

    # ─── DEM ────────────────────────────────────────────────
    tif_path = find_existing_dem(output_dir, region_name)

    if tif_path:
        log(f"Использую существующий DEM: {tif_path}")
        log("Скачивание из OpenTopography пропущено.")
    else:
        api_key = config["dem"].get("api_key_direct") or os.environ.get(
            config["dem"]["api_key_env"]
        )
        if not api_key:
            log("API-ключ OpenTopography не найден.")
            sys.exit(1)
        tif_path = os.path.join(output_dir, f"{region_name}_dem.tif")
        fetch_dem(bbox_from_config, config["dem"]["type"], api_key, tif_path)

    # ─── bbox из GeoTIFF ────────────────────────────────────
    tif_bbox = read_bbox_from_tif(tif_path)
    if tif_bbox:
        log(f"Использую bbox из GeoTIFF: {tif_bbox}")
        bbox = tif_bbox
    else:
        log(f"Не удалось прочитать bbox из GeoTIFF, беру из конфига: {bbox_from_config}")
        bbox = bbox_from_config

    # ─── PNG ────────────────────────────────────────────────
    png_path = os.path.join(output_dir, "heightmap.png")
    if not os.path.exists(png_path):
        meta = extract_dem_metadata(tif_path)
        convert_tif_to_png(tif_path, png_path,
                           meta["min_elevation_m"], meta["max_elevation_m"])
    else:
        log(f"PNG уже существует: {png_path} (пропускаю конвертацию)")

    # ─── region_meta.json ───────────────────────────────────
    meta = extract_dem_metadata(tif_path)
    region_meta = {
        "region_name": region_name,
        "bbox": bbox,
        "min_elevation_m": meta["min_elevation_m"],
        "max_elevation_m": meta["max_elevation_m"],
        "pixel_size_m": meta["pixel_size_m"],
        "width_px": meta["width"],
        "height_px": meta["height"],
    }
    save_json(region_meta, os.path.join(output_dir, "region_meta.json"))
    log("region_meta.json сохранён.")

    # ─── PBF ────────────────────────────────────────────────
    pbf_cfg = config["pbf_source"]

    if pbf_cfg["mode"] == "local":
        pbf_path = pbf_cfg["local_path"]
        if not pbf_path or not os.path.exists(pbf_path):
            log(f"Локальный PBF не найден: {pbf_path}")
            sys.exit(1)
    else:
        pbf_url = pbf_cfg.get("geofabrik_url") or None
        if not pbf_url:
            pbf_url = resolve_pbf_url(bbox)

        if pbf_url:
            pbf_path = download_pbf(pbf_url, pbf_cfg["cache_dir"])
        else:
            pbf_path = find_pbf_by_trial(
                bbox,
                pbf_cfg["cache_dir"],
                config["osm"]["road_filter"]
            )
            if not pbf_path:
                log("Ни один федеральный округ не содержит данных для этого bbox.")
                sys.exit(1)

    # ─── OSM → GeoJSON ──────────────────────────────────────
    extract_osm(
        pbf_path, bbox, output_dir,
        config["osm"]["road_filter"],
        config["osm"]["waterway_values"],
    )

    # ─── Копирование в Content/RawContent ───────────────────
    copy_to_content_raw(output_dir)

    log("Готово. Файлы в: " + os.path.abspath(output_dir))


# ─────────────────────────────────────────────────────────────────
#  CLI
# ─────────────────────────────────────────────────────────────────

def main():
    script_dir = Path(__file__).resolve().parent
    env_path = script_dir / ".env"

    if env_path.exists():
        if HAS_DOTENV:
            load_dotenv(env_path)
            log(f"Загружен .env: {env_path}")
        else:
            log(".env найден, но python-dotenv не установлен.")
    else:
        log(f".env не найден: {env_path}")
        log("Работаю с системными переменными.")

    parser = argparse.ArgumentParser(
        description="Готовит DEM (OpenTopography) и OSM (PBF) для RailOptima."
    )
    parser.add_argument("--config", "-c", type=str, help="Путь к config.json")
    parser.add_argument("--bbox", "-b", nargs=4, type=float,
                        metavar=("WEST", "SOUTH", "EAST", "NORTH"),
                        help="Границы области в градусах")
    parser.add_argument("--region", type=str, default=None,
                        help="Имя региона (для имён файлов)")
    parser.add_argument("--output", "-o", type=str, default=None,
                        help="Папка для сохранения")
    parser.add_argument("--pbf", type=str, default=None,
                        help="Путь к локальному PBF-файлу")
    parser.add_argument("--geofabrik-url", type=str, default=None,
                        help="URL PBF. Если не задан — ищется автоматически.")
    parser.add_argument("--api-key", type=str, default=None,
                        help="API-ключ OpenTopography (перебивает .env)")

    args = parser.parse_args()

    if args.config:
        config = load_config(args.config)
    else:
        output = args.output or os.environ.get("OUTPUT_DIR", "../RawContent")
        region = args.region or os.environ.get("REGION_NAME", "region")

        if args.bbox:
            west, south, east, north = args.bbox
        elif os.environ.get("BBOX_WEST"):
            west  = float(os.environ.get("BBOX_WEST",  0.0))
            south = float(os.environ.get("BBOX_SOUTH", 0.0))
            east  = float(os.environ.get("BBOX_EAST",  0.0))
            north = float(os.environ.get("BBOX_NORTH", 0.0))
        else:
            west = south = east = north = None

        if west is None:
            output_path = Path(output).resolve()
            has_dem = False
            if output_path.exists():
                has_dem = any(f.endswith("_dem.tif") for f in os.listdir(output_path))
            if not has_dem:
                log("Не задан bbox и нет готового DEM.")
                sys.exit(1)
            west = south = east = north = 0.0

        pbf_local = args.pbf or os.environ.get("PBF_LOCAL_PATH") or None
        gfb_url   = args.geofabrik_url or os.environ.get("GEOFABRIK_URL") or None

        config = {
            "region_name": region,
            "bbox": {
                "west":  west,
                "south": south,
                "east":  east,
                "north": north,
            },
            "output_dir": output,
            "pbf_source": {
                "mode": "local" if pbf_local else "auto",
                "local_path": pbf_local,
                "geofabrik_url": gfb_url,
                "cache_dir": "./cache",
            },
            "dem": {
                "type": os.environ.get("DEM_TYPE", "SRTMGL1"),
                "api_key_env": "OPENTOPO_API_KEY",
                "api_key_direct": args.api_key,
            },
            "osm": {
                "road_filter": os.environ.get("OSM_ROAD_FILTER", "driving"),
                "waterway_values": ["river", "stream"],
                "clip_to_bbox": True,
            },
        }

    prepare_region(config)


if __name__ == "__main__":
    main()