#!/bin/bash
# dosrun.sh <outdir> <steps...>  - run the ORIGINAL game in DOSBox on a virtual display and drive it.
#   steps:  wait:<sec>  key:<xdotool key>  shot:<name>   (e.g. wait:8 shot:menu key:p wait:2 shot:play)
# The original files are copied to a scratch dir first so TENNIS.OPT/.HAL changes never touch orig/.
OUT=$1; shift
S=$(dirname "$OUT")/dosscratch; rm -rf "$S"; mkdir -p "$S/game" "$OUT"
HERE=$(cd "$(dirname "$0")/.." && pwd)
cp "$HERE"/orig/* "$S/game/"
cat > "$S/dosbox.conf" <<C
[sdl]
fullscreen=false
output=surface
windowresolution=original
[render]
aspect=false
scaler=none
[cpu]
core=normal
cycles=max
[sblaster]
sbtype=sb16
sbbase=220
irq=5
dma=1
hdma=5
[autoexec]
mount c $S/game
c:
tennis.exe
C
export DISPLAY=:97
Xvfb :97 -screen 0 800x600x24 >/dev/null 2>&1 & XP=$!
sleep 1
dosbox -conf "$S/dosbox.conf" >/dev/null 2>&1 & DP=$!
sleep 2
for st in "$@"; do
  case $st in
    wait:*) sleep ${st#wait:};;
    key:*)  xdotool key --clearmodifiers ${st#key:};;
    keydown:*) xdotool keydown ${st#keydown:};;
    keyup:*) xdotool keyup ${st#keyup:};;
    shot:*) scrot -u "$OUT/${st#shot:}.png" 2>/dev/null || scrot "$OUT/${st#shot:}.png";;
  esac
done
kill $DP $XP 2>/dev/null
# the DOS window is 320x200 at (80,100) of the 800x600 virtual screen: crop the screenshots
python3 - "$OUT" <<PY
import sys,glob
from PIL import Image
for f in glob.glob(sys.argv[1]+"/*.png"):
    im=Image.open(f)
    if im.size==(800,600): im.crop((80,100,400,300)).save(f)
PY
