"""Full-editor (offscreen) verification; temporary captures are never saved to the map."""
import runpy
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
output = Path(unreal.Paths.project_saved_dir()) / 'ShowcaseExpansion'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
capture = subsystem.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(1000, 150, 180))
component = capture.get_component_by_class(unreal.SceneCaptureComponent2D)
target = unreal.RenderingLibrary.create_render_target2d(world, 1280, 720, unreal.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property('texture_target', target)
component.set_editor_property('capture_source', unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property('capture_every_frame', False)
component.set_editor_property('always_persist_rendering_state', True)
component.set_editor_property('fov_angle', 85.0)
settings = component.get_editor_property('post_process_settings')
settings.set_editor_property('override_dynamic_global_illumination_method', True)
settings.set_editor_property('dynamic_global_illumination_method', unreal.DynamicGlobalIlluminationMethod.LUMEN)
settings.set_editor_property('override_auto_exposure_min_brightness', True)
settings.set_editor_property('override_auto_exposure_max_brightness', True)
settings.set_editor_property('auto_exposure_min_brightness', -3.0)
settings.set_editor_property('auto_exposure_max_brightness', -3.0)
component.set_editor_property('post_process_settings', settings)
views = [('original', 1000), ('connection', 3700), ('extended', 20000)]
state = {'index': 0, 'since': time.monotonic(), 'checked': False, 'frames': 0}

def tick(delta):
    try:
        state['frames'] += 1
        if state['frames'] % 5 == 0:
            component.capture_scene()
        if not state['checked'] and state['frames'] > 10:
            state['checked'] = True
            try:
                runpy.run_path(str(Path(unreal.Paths.project_dir()) / 'Scripts' / 'validate_showcase1.py'),
                               init_globals={'USE_LOADED_WORLD': True})
            except Exception:
                unreal.log_error(traceback.format_exc())
        if time.monotonic() - state['since'] < 10 or state['frames'] < 60:
            return
        name, x = views[state['index']]
        unreal.RenderingLibrary.export_render_target(world, target, str(output), name + '.png')
        unreal.log_warning('SHOWCASE_RENDERED ' + name)
        state['index'] += 1
        if state['index'] == len(views):
            unreal.unregister_slate_post_tick_callback(handle)
            subsystem.destroy_actor(capture)
            unreal.SystemLibrary.quit_editor()
            return
        capture.set_actor_location(unreal.Vector(views[state['index']][1], 150, 180), False, False)
        state['since'] = time.monotonic()
        state['frames'] = 0
    except Exception:
        unreal.log_error(traceback.format_exc())
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()

handle = unreal.register_slate_post_tick_callback(tick)
