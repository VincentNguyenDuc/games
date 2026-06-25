#!/bin/bash
LOG=/tmp/display-start.log
echo "[$(date)] Starting display services..." > "$LOG"

setsid Xvfb :99 -screen 0 1280x720x24 -ac >> "$LOG" 2>&1 &
sleep 0.8

DISPLAY=:99 setsid openbox >> "$LOG" 2>&1 &
sleep 0.3

setsid x11vnc -display :99 -nopw -listen localhost -rfbport 5900 -forever >> "$LOG" 2>&1 &
sleep 0.3

setsid websockify --web=/usr/share/novnc/ 0.0.0.0:6080 localhost:5900 >> "$LOG" 2>&1 &

echo "[$(date)] Done. Open http://localhost:6080/vnc.html" >> "$LOG"
