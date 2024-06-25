#!/usr/bin/env bash

echo "$0: Patching worker import line for webpack in $1"

sed -E -i.bk "s/^.*worker = new Worker\(new URL\(import\.meta\.url\), workerOptions\);$/worker = new Worker(new URL(\"$2\", import.meta.url).href, workerOptions);/" $1

