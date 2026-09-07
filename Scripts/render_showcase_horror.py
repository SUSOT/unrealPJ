"""Read-only lighting QA. Capture saved exposure and a -3 EV comparison."""
import json
import math
import time
import traceback
from pathlib import Path
import unreal as u

u.EditorPythonScripting.set_keep_python_script_alive(True)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
api=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=list(api.get_all_level_actors())
d=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector))
out=Path(u.Paths.project_saved_dir())/'ShowcaseExpansion'
diagnostics=[]
for lamp in d.get_editor_property('lamps')[:1]:
    for c in lamp.get_components_by_class(u.MeshComponent):
        for i in range(c.get_num_materials()):
            m=c.get_material(i)
            if m: diagnostics.append({'mesh':c.get_name(),'material':m.get_path_name(),'scalars':[str(n) for n in u.MaterialEditingLibrary.get_scalar_parameter_names(m)]})
for pp_actor in [a for a in actors if isinstance(a,u.PostProcessVolume)]:
    pp=pp_actor.get_editor_property('settings')
    diagnostics.append({'pp':pp_actor.get_actor_label(),'values':{k:str(pp.get_editor_property(k)) for k in ('auto_exposure_min_brightness','auto_exposure_max_brightness','auto_exposure_bias','color_saturation','color_gain')}})
(out/'horror_lighting_inspection.json').write_text(json.dumps(diagnostics,indent=2),encoding='utf-8')
r=d.get_editor_property('loop_radius')
a=500/r
camera=api.spawn_actor_from_class(u.SceneCapture2D,u.Vector((r-115)*math.sin(a),r-(r-115)*math.cos(a)+50,180))
camera.set_actor_rotation(u.Rotator(yaw=math.degrees(a)),True)
c=camera.get_component_by_class(u.SceneCaptureComponent2D)
target=u.RenderingLibrary.create_render_target2d(world,1280,720,u.TextureRenderTargetFormat.RTF_RGBA8)
for k,v in [('texture_target',target),('capture_source',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR),('capture_every_frame',False),('always_persist_rendering_state',True),('fov_angle',80.0)]: c.set_editor_property(k,v)
pp=c.get_editor_property('post_process_settings')
pp.set_editor_property('override_dynamic_global_illumination_method',True)
pp.set_editor_property('dynamic_global_illumination_method',u.DynamicGlobalIlluminationMethod.LUMEN)
c.set_editor_property('post_process_settings',pp)
state={'index':0,'time':time.monotonic(),'busy':False,'frames':0}

def tick(dt):
    if state['busy']: return
    state['busy']=True
    try:
        state['frames']+=1
        if state['frames']%3==0: c.capture_scene()
        if time.monotonic()-state['time']<8 or state['frames']<45: return
        name=['horror_saved_exposure','horror_readable_exposure'][state['index']]
        u.RenderingLibrary.export_render_target(world,target,str(out),name+'.png')
        state['index']+=1
        if state['index']==2:
            u.unregister_slate_post_tick_callback(handle)
            u.SystemLibrary.quit_editor()
            return
        for k,v in [('override_auto_exposure_min_brightness',True),('override_auto_exposure_max_brightness',True),('auto_exposure_min_brightness',-3.0),('auto_exposure_max_brightness',-3.0)]: pp.set_editor_property(k,v)
        c.set_editor_property('post_process_settings',pp)
        state.update(time=time.monotonic(),frames=0)
    except Exception:
        u.log_error(traceback.format_exc())
        u.unregister_slate_post_tick_callback(handle)
        u.SystemLibrary.quit_editor()
    finally: state['busy']=False
handle=u.register_slate_post_tick_callback(tick)
