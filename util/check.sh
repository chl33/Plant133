#! /bin/sh
set -e
root="$(readlink -f "$(dirname "$0")"/..)"
cd "$root"
./util/license-headers.sh
# pio test -e native
