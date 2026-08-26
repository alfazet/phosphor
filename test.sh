#!/usr/bin/env bash

if ! command -v magick >/dev/null 2>&1; then
    echo "imagemagick not found"
    exit 1
fi

./build.sh release

cmd="./build/release/phosphor"
out_dir="/tmp/renders"
mkdir -p "$out_dir"

declare -a strips=()

for model in ./models/test/*.glb; do
    name=$(basename "$model" .glb)
    render_path="$out_dir/${name}_render.png"
    reference_path="${model}.png"
    strip_path="$out_dir/${name}_strip.png"

    $cmd "$@" -m "$model" -o "$render_path"

    if [[ ! -f "$reference_path" ]]; then
        echo "missing reference image: $reference_path" >&2
        exit 1
    fi

    magick "$render_path" "$reference_path" +append "$strip_path"

    strips+=("$strip_path")
done

magick "${strips[@]}" -append "output.png"
echo "combined output written to output.png"
