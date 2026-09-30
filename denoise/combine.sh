#!/usr/bin/env bash
set -euo pipefail

if ! command -v magick >/dev/null 2>&1; then
    echo "imagemagick not found" >&2
    exit 1
fi

out="output.png"

for file in ./*png
do
    echo "${file}"
        if [[ -e "$out" ]]; then
        magick "$out" \( "$file" -gravity NorthWest -pointsize 30 -annotate +10+10 "$file" \) -append "$out"
    else
        magick "$file" -gravity NorthWest -pointsize 30 -annotate +10+10 "$file" "$out"
    fi
done

echo "${out}"
