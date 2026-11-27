#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$HOME/Downloads/RailOptima-UnrealDev-git"
UE="$HOME/Applications/UnrealEngine/UE_5.8.2/Engine"
OUT="$HOME/RailOptimaBuild"
LOG="$OUT/build-$(date +%F_%H%M).log"
mkdir -p "$OUT"
cd "$PROJECT_DIR"

echo "== Предпроверка =="
file RawContent/heightmap.png | grep -qi "PNG image" || { echo "ОШИБКА: heightmap.png не PNG"; exit 1; }
for m in M_RoadLine M_EndpointMarker; do
  f="Content/RailOptima/Materials/$m.uasset"
  [ -s "$f" ] || { echo "ОШИБКА: $f пустой или отсутствует"; exit 1; }
  head -c 4 "$f" | xxd -p | grep -qi "c1832a9e" || { echo "ОШИБКА: $f не похож на uasset"; exit 1; }
done
ls Source/*.Target.cs

echo "== Сборка (лог: $LOG) =="
nice -n 10 "$UE/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$PROJECT_DIR/RailOptima.uproject" \
  -platform=Linux -clientconfig=Development \
  -build -cook -stage -pak -compressed \
  -archive -archivedirectory="$OUT" \
  -noP4 -utf8output -unattended 2>&1 | tee "$LOG"

echo "Готово: $OUT/Linux/"
