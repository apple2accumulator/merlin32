#!/bin/sh
PREFIX=/usr/local
MERLIN="$PREFIX/libexec/Merlin32"
MACRO_DIR="$PREFIX/share/merlin32/asminc"
"$MERLIN" -V "$MACRO_DIR" "$@"
