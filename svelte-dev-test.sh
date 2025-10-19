#! /bin/sh
set -e
here="$(readlink -f "$(dirname "$0")")"
cd "$here/svelte"
npm run dev
