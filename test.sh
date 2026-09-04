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

for model in ./models/test/*/*.glb; do
    name=$(basename "$model" .glb)
    path=$(dirname "$model")
    render_path="$out_dir/${name}_render.png"
    reference_path="screenshot.png"
    strip_path="$out_dir/${name}_strip.png"

    $cmd "$@" -m "$model" -o "$render_path"

    if [[ ! -f "$path/$reference_path" ]]; then
        echo "missing reference image: $reference_path" >&2
        exit 1
    fi

    magick "$render_path" "$path/$reference_path" +append "$strip_path"

    strips+=("$strip_path")
done

magick "${strips[@]}" -append "output.png"
echo "combined output written to output.png"
