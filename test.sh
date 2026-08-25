#!/usr/bin/env bash

set -euo pipefail

if ! command -v magick >/dev/null 2>&1
then
    echo "magick could not be found"
    exit 1
fi


./build.sh re

CMD="./build/release/phosphor"
ARGS="-p 100000 -h 1000 -w 1000 -i 8"

OUT_DIR="/tmp/renders"
mkdir -p "$OUT_DIR"

for model in ./models/test/*glb;
do
    name=$(basename "$model" .glb)
    render_path="$OUT_DIR/${name}_render.png"
    reference_path="${model}.png"
    strip_path="$OUT_DIR/${name}_strip.png"

    $CMD $ARGS -m "$model" -o "$render_path"

    if [[ ! -f "$reference_path" ]]; then
        echo "Missing reference image: $reference_path" >&2
        exit 1
    fi

    magick "$render_path" "$reference_path" +append "$strip_path"

    STRIPS+=("$strip_path")
done

magick "${STRIPS[@]}" -append "output.png"
echo "Output written to output.png"
