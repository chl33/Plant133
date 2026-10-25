#! /bin/sh
set -e
cd "$(dirname "$0")"/..
license-header-checker -r -i .git,.github,.pio,.ccls-cache,svelteesp32async.h -e '^// .*\n// .*' \
	-a ./util/license_header.txt . h cpp scad
