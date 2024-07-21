#! /bin/sh
set -e
cd "$(dirname "$0")"/..
license-header-checker -r -i .git,.github,.pio \
	-a ./util/license_header.txt . h cpp scad
