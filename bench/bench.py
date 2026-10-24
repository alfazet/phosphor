#!/usr/bin/env python

from metrics import ssim, mse

import argparse, csv, json, shutil, subprocess, sys, time
from datetime import datetime
from pathlib import Path
import re

import numpy as np
from matplotlib.image import imread


def load(path):
    img = imread(path).astype(np.float64)
    if img.ndim == 2:
        img = np.stack([img] * 3, axis=-1)
    return img[..., :3]

def final_frame_name(out_dir):
    pngs = list(Path(out_dir).glob("*.png"))
    # assuming the oldest png in output dir is the render
    return max(pngs, key=lambda p: p.stat().st_mtime) if pngs else None

def scene_settings(cfg, name):
    over = next((s for s in cfg.get("scenes", []) if s.get("name") == name), {})
    sc = {k: v for k, v in cfg.items() if k != "scenes"}
    for k, v in over.items():
        if k == "name":
            continue
        if k in ("args", "thresholds"):
            sc[k] = {**(cfg.get(k) or {}), **(v or {})}
        else:
            sc[k] = v
    return sc

_FINISHED_RE = re.compile(r"finished\s+(?P<name>.+?),\s*took\s+(?P<ms>\d+(?:\.\d+)?)\s*ms")
def parse_finished_times(log_path):
    results = []
    with Path(log_path).open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            m = _FINISHED_RE.search(line.rstrip())
            if m:
                results.append((m.group("name"), float(m.group("ms"))))
    return results

def render(cfg, glb, seed, out_dir, suite_args):
    log = out_dir / "renderer.log"
    args = {**suite_args, "-m": str(glb), "--seed": seed, "-o": str(out_dir), "--logs": log}
    cmd = [cfg["renderer"]] + [t for k, v in args.items() for t in (k, str(v))]
    out_dir.mkdir(parents=True, exist_ok=True)

    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=cfg["timeout_s"])
    except subprocess.TimeoutExpired:
        return None, "timeout", -1, -1, -1,

    ms = [v for _, v in parse_finished_times(log)] if log.exists() else []
    t0 = ms[0] if len(ms) > 0 else -1
    t1 = ms[1] if len(ms) > 1 else -1
    t2 = sum(ms[2:]) if len(ms) > 2 else -1

    t0 /= 1000
    t1 /= 1000
    t2 /= 1000

    if p.returncode != 0:
        return None, f"exit {p.returncode}", t0, t1, t2
    png = final_frame_name(out_dir)
    if not png.exists():
        return None, f"missing {png.name}", t0, t1, t2
    return png, "", t0, t1, t2

def find_models(cfg, only=None, exclude=None):
    root = Path(cfg["models_dir"])
    if not root.exists():
        return []
    return [d for d in sorted(root.iterdir()) if d.is_dir()
            and (not only or (d.name in only))
            and (not exclude or (d.name not in exclude))]

def pick_file(folder, suffix, ext, name):
    if not folder.is_dir():
        return None, f"{folder} is not a directory"

    cands = sorted(
            (p for p in folder.iterdir()
             if p.is_file() and p.suffix.lower() == suffix and not p.name.startswith(".")),
            key=lambda p: p.name.lower(),
            )
    if not cands:
        return None, f"no {ext} (*{suffix}) in {folder}"
    if len(cands) > 1:
        # prefer the file named after the folder
        named = [p for p in cands if p.stem.lower() == name.lower()]
        chosen = named[0] if named else cands[0]
        print(f"WARNING [{name}]: {len(cands)} {ext} files in {folder}, using {chosen.name}",
              file=sys.stderr)
        return chosen, None
    return cands[0], None

def run_suite(cfg, only, exclude):
    models = find_models(cfg, only, exclude)
    if not models:
        print(f"no models found in {cfg['models_dir']}")
        return 1

    run_dir = Path(cfg["runs_dir"]) / datetime.now().strftime("%Y%m%d-%H%M%S")
    rows, failures = [], []

    for d in models:
        name = d.name
        glb, err = pick_file(d, ".glb", "glTF model", name)
        if glb is None:
            failures.append(f"{name}: {err}")
            continue
        ref, err = pick_file(d, ".png", "reference image", name)
        if ref is None:
            failures.append(f"{name}: {err}")
            continue
        cfg = scene_settings(cfg, name)
        th = cfg["thresholds"]
        ref = load(ref)
        ssims, mses = [], []

        for seed in cfg["seeds"]:
            png, err, t0, t1, t2 = render(cfg, glb, seed, run_dir / name / f"seed{seed}", cfg["args"])
            row = {"run": run_dir.name, "model": name, "seed": seed, "time reading": round(t0, 2), "time structs": round(t1, 2), "time render": round(t2, 2),
                   "ssim": "", "mse": "", "status": err or "ok"}
            if not err:
                img = load(png)
                if img.shape != ref.shape:
                    row["status"] = f"shape {img.shape} != ref {ref.shape}"
                else:
                    row["ssim"], row["mse"] = round(ssim(img, ref), 5), round(mse(img, ref), 6)
                    ssims.append(row["ssim"]); mses.append(row["mse"])
            if row["status"] != "ok":
                failures.append(f"{name} seed {seed}: {row['status']}")
            rows.append(row)
            print(f"{name:16s} seed {seed:<6d} {row['status']:8s} "
                  f"SSIM {row['ssim']}  MSE {row['mse']}  ({row['time reading']} s) ({row['time structs']} s) ({row['time render']} s)")

        if ssims:
            m_ssim, s_ssim, m_mse = float(np.mean(ssims)), float(np.std(ssims)), float(np.mean(mses))
            print(f"{name:16s} mean SSIM {m_ssim:.4f} (std {s_ssim:.4f})  mean MSE {m_mse:.6f}\n")
            if m_ssim < th["ssim_min"]:
                failures.append(f"{name}: mean SSIM {m_ssim:.4f} < {th['ssim_min']}")
            if m_mse > th["mse_max"]:
                failures.append(f"{name}: mean MSE {m_mse:.6f} > {th['mse_max']}")
            if s_ssim > th["ssim_std_max"]:
                failures.append(f"{name}: SSIM std {s_ssim:.4f} > {th['ssim_std_max']}")

    run_dir.mkdir(parents=True, exist_ok=True)
    fields = ["run", "model", "seed", "time reading", "time structs", "time render", "ssim", "mse", "status"]
    with open(run_dir / "results.csv", "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)
    print(f"results: {run_dir / 'results.csv'}")

    if failures:
        print("\nFAILED:")
        for m in failures:
            print("  -", m)
        return 1
    print("all models passed.")
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--suite", default="bench/suite.json", help="test config")
    ap.add_argument("--only", nargs="+", help="model name(s) to run")
    ap.add_argument("--exclude", nargs="+", help="model name(s) to exclude")
    a = ap.parse_args()
    cfg = json.loads(Path(a.suite).read_text())
    return run_suite(cfg, a.only, a.exclude)


if __name__ == "__main__":
    sys.exit(main())

