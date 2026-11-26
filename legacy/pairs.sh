#!/usr/bin/env bash
set -euo pipefail

if ! command -v magick >/dev/null 2>&1; then
    echo "imagemagick not found" >&2
    exit 1
fi

if [[ $# -lt 1 ]]; then
    echo "usage: $0 <out.png> [-- phosphor args...]" >&2
    exit 1
fi

out="$1"
shift
if [[ "${1:-}" == "--" ]]; then
    shift
fi

mkdir -p "$(dirname "${out}")"

out_dir=$(mktemp -d)
declare -a strips=()

for model in ./models/test/*/*.glb; do
    name=$(basename "${model}" .glb)
    strip_path="${out_dir}/${name}_strip.png"

    written_path=$(./pair.sh "${model}" "${strip_path}" -- "$@")
    strips+=("${written_path}")
done

magick "${strips[@]}" -append "${out}"
echo "${out}"
