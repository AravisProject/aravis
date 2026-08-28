#!/bin/sh

for file in "$@"; do
    sed -i -E \
        's/^([[:space:]]*#[[:space:]]*include[[:space:]]+)<(arv[^>]+\.h)>/\1"\2"/' \
        "$file"
done
