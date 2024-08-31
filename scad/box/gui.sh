#! /bin/sh
here="$(readlink -f $(dirname "$0"))"
exec python3 "${here}/gui.py"
