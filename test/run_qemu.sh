#!/bin/bash
# run_qemu.sh: Solstice on System 7.6.1 in the QEMU Quadra 800.  It opens on the solargraph (screen_60s); then the
# Lisa's letter keys, one page every 10 s: D 70, O 80, E 90, W 100, Y 110, F 120, F 130, G 140, M 150, L 160, P 170 to 220 (six pictures).
#   bash test/run_qemu.sh [seconds]   (230 for every page)
# Puts the application in Startup Items and boots headless (Return at 25 and 40 s for the 'not shut down' warning).
set -u
cd "$(dirname "$0")/.."
T=/Volumes/RetroDev/src/Retro68-build/toolchain/bin
Q=/Volumes/RetroDev/emulators/qemu
SECS=${1:-230}
W=test/work; OUT=test/out; PORT=45812
mkdir -p $W $OUT; rm -f $OUT/*
cp -c $Q/hd.img $W/q.img
cp $Q/pram.img $W/q_pram.img
SI=":System Folder:Startup Items"
( export HOME=$PWD/$W
  $T/hmount $W/q.img >/dev/null
  $T/hcopy -m build/Solstice.bin "$SI:Solstice"
  $T/hcopy -r "$SI:Solstice Trace" "$OUT/trace.txt" 2>/dev/null
  $T/humount )
/opt/local/bin/qemu-system-m68k -M q800 -m 128 -bios $Q/Q800.ROM -display none -g 1152x870x8 \
    -drive file=$W/q_pram.img,format=raw,if=mtd \
    -device scsi-hd,scsi-id=0,drive=hd0 \
    -drive file=$W/q.img,format=raw,media=disk,if=none,id=hd0 \
    -nic user,model=dp83932 \
    -monitor tcp:127.0.0.1:$PORT,server,nowait > $W/qemu.log 2>&1 &
pid=$!
mon() { printf '%s\n' "$1" | nc -w 2 127.0.0.1 $PORT >/dev/null 2>&1; }
for ((t=5; t<=SECS; t+=5)); do
  sleep 5
  (( t == 25 || t == 40 )) && mon "sendkey ret"
  case $t in 70) mon "sendkey d";; 80) mon "sendkey o";; 90) mon "sendkey e";; 100) mon "sendkey w";; 110) mon "sendkey y";;
             120|130) mon "sendkey f";; 140) mon "sendkey g";; 150) mon "sendkey m";; 160) mon "sendkey l";; 170|180|190|200|210|220) mon "sendkey p";; esac
  (( t % 10 == 5 )) && (( t >= 75 )) && { mon "screendump $PWD/$OUT/screen_${t}s.ppm"; sleep 1; sips -s format png $OUT/screen_${t}s.ppm --out $OUT/screen_${t}s.png >/dev/null 2>&1 && rm -f $OUT/screen_${t}s.ppm; }
  if (( t % 10 == 0 )); then
    mon "screendump $PWD/$OUT/screen_${t}s.ppm"
    sleep 1
    sips -s format png $OUT/screen_${t}s.ppm --out $OUT/screen_${t}s.png >/dev/null 2>&1 && rm -f $OUT/screen_${t}s.ppm
  fi
done
mon quit; sleep 2; kill -9 $pid 2>/dev/null; wait $pid 2>/dev/null
( export HOME=$PWD/$W
  $T/hmount $W/q.img >/dev/null
  $T/hls -l "$SI" > $OUT/startup_items.txt
  $T/hcopy -r "$SI:Solstice Trace" "$OUT/trace.txt" 2>/dev/null
  $T/humount )
cat $OUT/startup_items.txt
