"""Temporary offscreen visual checks, without saving preview changes to the map."""
import math
import time
import traceback
from pathlib import Path
import unreal as u

u.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(u.Paths.project_saved_dir())/'ShowcaseExpansion'
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
api=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=list(api.get_all_level_actors())
director=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector))
door=director.get_editor_property('escape_door')
r=director.get_editor_property('loop_radius')
def pos(s,z=180):
    a=s/r
    return u.Vector((r-115)*math.sin(a),r-(r-115)*math.cos(a)+50,z)

capture=api.spawn_actor_from_class(u.SceneCapture2D,pos(500))
c=capture.get_component_by_class(u.SceneCaptureComponent2D)
target=u.RenderingLibrary.create_render_target2d(world,1280,720,u.TextureRenderTargetFormat.RTF_RGBA8)
c.set_editor_property('texture_target',target)
c.set_editor_property('capture_source',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
c.set_editor_property('capture_every_frame',False)
c.set_editor_property('always_persist_rendering_state',True)
c.set_editor_property('fov_angle',80.0)
c.set_editor_property('show_flag_settings',[u.EngineShowFlagsSetting(show_flag_name='TextRender',enabled=True)])
pp=c.get_editor_property('post_process_settings')
for key,value in [('override_dynamic_global_illumination_method',True),('dynamic_global_illumination_method',u.DynamicGlobalIlluminationMethod.LUMEN),('override_auto_exposure_min_brightness',True),('override_auto_exposure_max_brightness',True),('auto_exposure_min_brightness',-3.0),('auto_exposure_max_brightness',-3.0)]: pp.set_editor_property(key,value)
c.set_editor_property('post_process_settings',pp)
names=['escape_normal','escape_distorted','escape_door_closed','escape_door_open','escape_destination','escape_sign_detail']
def setup(index):
    if index in (0,1):
        capture.set_actor_location(pos(500),False,True)
        capture.set_actor_rotation(u.Rotator(yaw=math.degrees(500/r)),True)
    if index==1:
        for change in sorted(director.get_editor_property('spatial_changes'),key=lambda change:change.get_editor_property('stage')):
            change.get_editor_property('target').set_actor_transform(change.get_editor_property('changed_transform'),False,True)
        for sign in director.get_editor_property('direction_signs'): sign.get_component_by_class(u.TextRenderComponent).set_text('<  EXIT')
        for i,lamp in enumerate(director.get_editor_property('lamps')):
            for light in lamp.get_components_by_class(u.LightComponent): light.set_intensity(light.get_editor_property('intensity')*(.42 if i%3==0 else .85))
    if index==2:
        door.set_is_temporarily_hidden_in_editor(False)
        door.reveal_at(u.Transform(location=pos(1800,20),rotation=u.Rotator(yaw=math.degrees(1800/r))))
        capture.set_actor_location(pos(2350),False,True)
        capture.set_actor_rotation(u.Rotator(yaw=math.degrees(2350/r)+180),True)
    if index==3:
        hinge=next(p for p in door.get_components_by_class(u.SceneComponent) if p.get_name()=='Hinge')
        hinge.set_relative_rotation(u.Rotator(yaw=108),False,True)
    if index==4:
        capture.set_actor_location(u.Vector(0,-9000,180),False,True)
        capture.set_actor_rotation(u.Rotator(),True)
    if index==5:
        sign=director.get_editor_property('direction_signs')[0]
        capture.set_actor_location(sign.get_actor_location()+sign.get_actor_forward_vector()*250,False,True)
        capture.set_actor_rotation(u.Rotator(yaw=sign.get_actor_rotation().yaw+180),True)
        text=sign.get_component_by_class(u.TextRenderComponent)
        u.log_warning('ESCAPE_TEXT_DIAGNOSTIC '+str([(key,str(text.get_editor_property(key))) for key in ('text','font','text_material','world_size','visible','hidden_in_game')]))
        u.log_warning('ESCAPE_TEXT_BOUNDS '+str(sign.get_actor_bounds(False)))
setup(0)
state={'index':0,'frames':0,'since':time.monotonic(),'busy':False}
def tick(delta):
    if state['busy']: return
    state['busy']=True
    try:
        state['frames']+=1
        if state['frames']%5==0: c.capture_scene()
        if state['frames']<60 or time.monotonic()-state['since']<10: return
        u.RenderingLibrary.export_render_target(world,target,str(out),names[state['index']]+'.png')
        u.log_warning('ESCAPE_CAPTURE '+names[state['index']])
        state['index']+=1
        if state['index']==len(names):
            u.unregister_slate_post_tick_callback(handle)
            u.SystemLibrary.quit_editor()
            return
        setup(state['index'])
        state['frames']=0
        state['since']=time.monotonic()
    except Exception:
        u.log_error(traceback.format_exc())
        u.unregister_slate_post_tick_callback(handle)
        u.SystemLibrary.quit_editor()
    finally: state['busy']=False
handle=u.register_slate_post_tick_callback(tick)
