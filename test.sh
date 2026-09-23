#!/usr/bin/env bash

if ! command -v magick >/dev/null 2>&1; then
    echo "imagemagick not found"
    exit 1
fi

./build.sh release

cmd="./build/release/phosphor"
out_dir=$(mktemp -d)

declare -a strips=()
i=0

for model in ./models/test/*/*.glb; do
    name=$(basename "${model}" .glb)
    path=$(dirname "${model}")
    reference_path="screenshot.png"
    strip_path="${out_dir}/${name}_${i}_strip.png"

    i=$((i+1))
    $cmd "$@" -m "${model}" -o "${out_dir}" -r 1000
    if [ $? -ne 0 ]; then	
	exit 1
    fi	

    render_path="${out_dir}/$(ls -t "${out_dir}" | head -n1)"
    if [[ ! -f "$path/$reference_path" ]]; then
        echo "missing reference image: ${reference_path}" >&2
        exit 1
    fi

    magick "${render_path}" "${path}/${reference_path}" +append "${strip_path}"

    strips+=("${strip_path}")
done

magick "${strips[@]}" -append "output.png"
echo "combined output written to output.png"
