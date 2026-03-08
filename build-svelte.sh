#! /bin/sh
set -e
here="$(readlink -f "$(dirname "$0")")"
cd "$here/svelte"
npm run build
npx svelteesp32 -e psychic -s dist -o ../lib/watering/svelteesp32async.h --etag=true
# Use og3::WebServer wrapper
#sed -i 's/void initSvelteStaticFiles(PsychicHttpServer \* server)/void initSvelteStaticFiles(og3::WebServer \* web_server)/' ../lib/watering/svelteesp32async.h
# Call native_server() directly
#sed -i 's/server->on/web_server->native_server().on/g' ../lib/watering/svelteesp32async.h
#sed -i 's/server->defaultEndpoint/web_server->native_server().defaultEndpoint/g' ../lib/watering/svelteesp32async.h
