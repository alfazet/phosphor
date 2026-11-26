#!/usr/bin/env bash
set -euo pipefail

source "./render.sh"

if ! command -v magick >/dev/null 2>&1; then
    echo "imagemagick not found" >&2
    exit 1
fi

if [[ $# -lt 2 ]]; then
    echo "usage: $0 <model.glb> <out.png> [-- phosphor args...]" >&2
    exit 1
fi

model="$1"
out="$2"
shift 2
if [[ "${1:-}" == "--" ]]; then
    shift
fi

mkdir -p "$(dirname "${out}")"

out_dir=$(mktemp -d)
name=$(basename "${model}" .glb)
blender_path="${out_dir}/${name}_blender.png"

render_blender "${model}" "${blender_path}"
render_path=$(render_phosphor "${model}" "${out_dir}" "$@")

magick "${render_path}" "${blender_path}" +append "${out}"
echo "${out}"
