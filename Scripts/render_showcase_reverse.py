"""Read-only reverse-table composition QA at the saved exposure; no map save."""
import math, time, traceback
from pathlib import Path
import unreal as u
u.EditorPythonScripting.set_keep_python_script_alive(True)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
api=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
d=next(a for a in api.get_all_level_actors() if isinstance(a,u.ShowcaseLoopDirector))
chair=d.get_editor_property('reverse_chair')
out=Path(u.Paths.project_saved_dir())/'ShowcaseExpansion'
# Reproduce the settled local-light multiplier; do not change the exposure.
for lamp in d.get_editor_property('lamps'):
    distance=(lamp.get_actor_location()-chair.get_actor_location()).length2d()
    t=max(0,min(1,(distance-1800)/800)); weight=1-t*t*(3-2*t)
    factor=1-.9*weight
    for light in lamp.get_components_by_class(u.LightComponent):
        light.set_intensity(light.get_editor_property('intensity')*factor)
    for mesh in lamp.get_components_by_class(u.MeshComponent):
        for slot in range(mesh.get_num_materials()):
            material=mesh.get_material(slot)
            if material and 'EmissiveStrenght' in [str(n) for n in u.MaterialEditingLibrary.get_scalar_parameter_names(material)]:
                value=u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(material,'EmissiveStrenght')
                mesh.create_dynamic_material_instance(slot).set_scalar_parameter_value('EmissiveStrenght',value*factor)
r=d.get_editor_property('loop_radius'); angle=-750/r
location=u.Vector((r-115)*math.sin(angle),r-(r-115)*math.cos(angle)+50,180)
camera=api.spawn_actor_from_class(u.SceneCapture2D,location)
look=chair.get_actor_bounds(False)[0]+u.Vector(0,0,25)
camera.set_actor_rotation(u.MathLibrary.find_look_at_rotation(location,look),True)
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
        name=['reverse_pulled_chair','reverse_tucked_chair'][state['index']]
        u.RenderingLibrary.export_render_target(world,target,str(out),name+'.png')
        state['index']+=1
        if state['index']==2:
            u.log_warning('SHOWCASE_REVERSE_RENDER_PASS')
            u.unregister_slate_post_tick_callback(handle); u.SystemLibrary.quit_editor(); return
        chair.set_actor_transform(d.get_editor_property('reverse_chair_tucked_pose'),False,True)
        state.update(time=time.monotonic(),frames=0)
    except Exception:
        u.log_error(traceback.format_exc())
        u.unregister_slate_post_tick_callback(handle); u.SystemLibrary.quit_editor()
    finally: state['busy']=False
handle=u.register_slate_post_tick_callback(tick)
