#!/usr/bin/env bash

if ! command -v magick >/dev/null 2>&1; then
    echo "imagemagick not found"
    exit 1
fi

./build.sh release

cmd="./build/release/phosphor"
out_dir=$(mktemp -d)

param1="--defocus-angle"
start1=1
end1=10
step1=1

param2="--focus-distance"
start2=2
end2=20
step2=2

declare -a strips=()

for ((i=start1; i<=end1; i+=step1))
do
	declare -a row=()
	for ((j=start2; j<=end2; j+=step2))
	do
		render_path="${out_dir}/${i}_${j}.png"
		$cmd "$@" ${param1} ${i} ${param2} ${j} -o "${render_path}" 
		magick "${render_path}" -gravity NorthWest -pointsize 30 -annotate +10+10 "${param1} ${i} ${param2} ${j}" "${render_path}"
		row+=("${render_path}")
	done
	strip_path="${out_dir}/${i}.png"
	strips+=("${strip_path}")
	magick "${row[@]}" -append "${strip_path}"
done

magick "${strips[@]}" +append "output.png"
echo "combined output written to output.png"
