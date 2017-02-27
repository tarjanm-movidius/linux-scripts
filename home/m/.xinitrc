#!/bin/sh

grep -wq '\[startup\]' ~/.fluxbox/apps || echo "[startup] {~/.fluxbox/fbstart}" >> ~/.fluxbox/apps

#pulseaudio --start
exec /etc/X11/Sessions/fluxbox
