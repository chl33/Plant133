#! /bin/sh
set -e
here="$(readlink -f "$(dirname "$0")")"
cd "$here/svelte"
npm run build
npx svelteesp32 -e async -s dist -o ../lib/watering/svelteesp32async.h --etag=true
