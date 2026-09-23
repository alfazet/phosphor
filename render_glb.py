#!/usr/bin/env python

import bpy
import sys
import os

argv = sys.argv
argv = argv[argv.index("--") + 1:]
glb_path = os.path.abspath(argv[0])
out_path = os.path.abspath(argv[1])
res = int(argv[2])

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

scene = bpy.context.scene

if scene.camera is None:
    cams = [o for o in scene.objects if o.type == 'CAMERA']
    if cams:
        scene.camera = cams[0]
    else:
        print(f"no camera in {glb_path}")
        sys.exit(0)

scene.render.engine = 'CYCLES'
scene.cycles.samples = 64
scene.render.resolution_x = res
scene.render.resolution_y = res
scene.render.image_settings.file_format = 'PNG'
scene.render.filepath = out_path

prefs = bpy.context.preferences
cycles_prefs = prefs.addons['cycles'].preferences
for backend in ('CUDA', 'HIP'):
    cycles_prefs.compute_device_type = backend
    cycles_prefs.get_devices()
    if any(d.type == backend for d in cycles_prefs.devices):
        break

for device in cycles_prefs.devices:
    device.use = True

scene.cycles.device = 'GPU'

bpy.ops.render.render(write_still=True)
