## Usage

From project root:
```bash
pip install -r requirements.txt
python bench/bench.py
```

This runs the renderer on each model in the `models/test` directory, comparing the outputs against the reference images.

## Output
A run produces a directory under `bench/runs` with:
- a global `.csv` with one row per model per seed
- rendered image for each model
- logs for each model

## Configuration
Configure a run by providing a `suite.json`.
Global config values can be overridden per model in the `"scenes"` array.
