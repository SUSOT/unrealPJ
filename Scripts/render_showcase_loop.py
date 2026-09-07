"""Offscreen visual QA. Temporary capture/exposure changes are not saved."""
import json
import math
import runpy
import time
import traceback
from pathlib import Path
import unreal as u

u.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(u.Paths.project_saved_dir()) / 'ShowcaseExpansion'
result = json.loads((out / 'loop_result.json').read_text(encoding='utf-8'))
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level(result['map'])
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
api = u.get_editor_subsystem(u.EditorActorSubsystem)
r = result['radius_cm']
views = [('loop_start', 200), ('loop_quarter', 5000), ('loop_seam', 18800)]
def camera_transform(s):
    angle = s / r
    return u.Transform(location=u.Vector((r-115)*math.sin(angle), r-(r-115)*math.cos(angle)+50, 180),
                       rotation=u.Rotator(yaw=math.degrees(angle)))
capture = api.spawn_actor_from_class(u.SceneCapture2D, u.Vector())
capture.set_actor_transform(camera_transform(views[0][1]), False, True)
component = capture.get_component_by_class(u.SceneCaptureComponent2D)
target = u.RenderingLibrary.create_render_target2d(world, 1280, 720, u.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property('texture_target', target)
component.set_editor_property('capture_source', u.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property('capture_every_frame', False)
component.set_editor_property('always_persist_rendering_state', True)
component.set_editor_property('fov_angle', 85.0)
settings = component.get_editor_property('post_process_settings')
settings.set_editor_property('override_dynamic_global_illumination_method', True)
settings.set_editor_property('dynamic_global_illumination_method', u.DynamicGlobalIlluminationMethod.LUMEN)
settings.set_editor_property('override_auto_exposure_min_brightness', True)
settings.set_editor_property('override_auto_exposure_max_brightness', True)
settings.set_editor_property('auto_exposure_min_brightness', -3.0)
settings.set_editor_property('auto_exposure_max_brightness', -3.0)
component.set_editor_property('post_process_settings', settings)
state = {'index': 0, 'since': time.monotonic(), 'frames': 0, 'checked': False}
def tick(delta):
    try:
        state['frames'] += 1
        if state['frames'] % 5 == 0:
            component.capture_scene()
        if not state['checked'] and state['frames'] > 10:
            state['checked'] = True
            try:
                runpy.run_path(str(Path(u.Paths.project_dir()) / 'Scripts' / 'validate_showcase_loop.py'),
                               init_globals={'USE_LOADED_WORLD': True})
            except Exception:
                u.log_error(traceback.format_exc())
        if state['frames'] < 60 or time.monotonic() - state['since'] < 12:
            return
        u.RenderingLibrary.export_render_target(world, target, str(out), views[state['index']][0] + '.png')
        u.log_warning('SHOWCASE_LOOP_CAPTURED ' + views[state['index']][0])
        state['index'] += 1
        if state['index'] == len(views):
            u.unregister_slate_post_tick_callback(handle)
            api.destroy_actor(capture)
            u.SystemLibrary.quit_editor()
            return
        capture.set_actor_transform(camera_transform(views[state['index']][1]), False, True)
        state['frames'], state['since'] = 0, time.monotonic()
    except Exception:
        u.log_error(traceback.format_exc())
        u.unregister_slate_post_tick_callback(handle)
        u.SystemLibrary.quit_editor()
handle = u.register_slate_post_tick_callback(tick)
