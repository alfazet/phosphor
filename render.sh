#!/usr/bin/env bash
set -euo pipefail

BLENDER_BIN="blender"
PHOSPHOR_BIN="./build/release/phosphor"
RENDER_PY="./render_glb.py"
RES=1024

render_blender() {
    local glb_path="$1"
    local out_path="$2"
    "${BLENDER_BIN}" -b -P "${RENDER_PY}" -- "${glb_path}" "${out_path}" "${RES}" 1>&2
}

render_phosphor() {
    local model_path="$1"
    local out_dir="$2"
    shift 2
    "${PHOSPHOR_BIN}" "$@" -m "${model_path}" -o "${out_dir}" -r "${RES}" --snapshots 0 1>&2
    echo "${out_dir}/$(ls -t "${out_dir}" | head -n1)"
}
