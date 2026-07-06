#! /bin/sh
set -e
here="$(readlink -f "$(dirname "$0")")"
cd "$here/svelte"
npm run build
npx --yes svelteesp32 -e psychic -s dist -o ../lib/watering/svelteesp32async.h --etag=always
