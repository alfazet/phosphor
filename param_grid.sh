#!/usr/bin/env bash
# param_grid.sh
set -euo pipefail

source "./render.sh"

if ! command -v magick >/dev/null 2>&1; then
    echo "imagemagick not found" >&2
    exit 1
fi


if [[ $# -lt 2 ]]; then
    echo "usage: $0 <model.glb> <out.png> [phosphor args...]" >&2
    exit 1
fi

model="$1"
out="$2"
shift 2

out_dir=$(mktemp -d)

param1="--defocus-angle"
start1=1
end1=5
step1=1

param2="--focus-distance"
start2=2
end2=10
step2=2

declare -a strips=()

for ((i=start1; i<=end1; i+=step1))
do
	declare -a row=()
	for ((j=start2; j<=end2; j+=step2))
	do
		render_path=$(render_phosphor "${model}" "${out_dir}" "$@" ${param1} ${i} ${param2} ${j})
		unique_path="${out_dir}/${i}_${j}.png"
		magick "${render_path}" -gravity NorthWest -pointsize 30 -annotate +10+10 "${param1} ${i} ${param2} ${j}" "${unique_path}"
		row+=("${unique_path}")
	done
	strip_path="${out_dir}/${i}.png"
	strips+=("${strip_path}")
	magick "${row[@]}" -append "${strip_path}"
done

magick "${strips[@]}" +append "${out}"
echo "${out}"
