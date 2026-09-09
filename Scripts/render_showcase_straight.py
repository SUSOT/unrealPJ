"""Capture real PIE lighting at ordinary, dread, and turn-back moments. Never saves."""
import unreal as u, time, traceback, math
from pathlib import Path
u.EditorPythonScripting.set_keep_python_script_alive(True)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem); editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
preview=u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(u.SceneCapture2D,u.Vector(220,165,185))
preview.set_actor_label('Straight_QA_Capture')
out=Path(u.Paths.project_saved_dir())/'ShowcaseExpansion'
state={'busy':False,'next':time.monotonic()}
def scenario():
    levels.editor_request_begin_play()
    while not editor.get_game_world(): yield .1
    yield 1
    w=editor.get_game_world(); pawn=u.GameplayStatics.get_player_pawn(w,0); pc=u.GameplayStatics.get_player_controller(w,0)
    d=u.GameplayStatics.get_all_actors_of_class(w,u.ShowcaseLoopDirector)[0]
    movement=pawn.get_component_by_class(u.CharacterMovementComponent); movement.set_movement_mode(u.MovementMode.MOVE_FLYING)
    height=pawn.get_component_by_class(u.CapsuleComponent).get_scaled_capsule_half_height()+24
    camera=next(a for a in u.GameplayStatics.get_all_actors_of_class(w,u.SceneCapture2D) if a.get_actor_label()=='Straight_QA_Capture')
    c=camera.get_component_by_class(u.SceneCaptureComponent2D)
    target=u.RenderingLibrary.create_render_target2d(w,1280,720,u.TextureRenderTargetFormat.RTF_RGBA8)
    for k,v in [('texture_target',target),('capture_source',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR),('capture_every_frame',False),('always_persist_rendering_state',True),('fov_angle',80.0)]: c.set_editor_property(k,v)
    pp=c.get_editor_property('post_process_settings'); pp.set_editor_property('override_dynamic_global_illumination_method',True)
    pp.set_editor_property('dynamic_global_illumination_method',u.DynamicGlobalIlluminationMethod.LUMEN); c.set_editor_property('post_process_settings',pp)
    c.hide_actor_components(pawn)
    x=220
    for goal,yaw,name in [(220,0,'straight_start'),(4050,0,'straight_dread'),(6400,180,'straight_turn_door')]:
        while x<goal:
            x=min(goal,x+60); pawn.set_actor_location(u.Vector(x,165,height),False,True)
            movement.stop_movement_immediately(); pc.set_control_rotation(u.Rotator(yaw=0)); yield .06
        pc.set_control_rotation(u.Rotator(yaw=yaw)); yield .3
        camera.set_actor_location(u.Vector(x,165,185),False,True); camera.set_actor_rotation(u.Rotator(yaw=yaw),True)
        for frame in range(45): c.capture_scene(); yield .10
        u.RenderingLibrary.export_render_target(w,target,str(out),name+'.png')
    u.log_warning('SHOWCASE_STRAIGHT_RENDER_PASS')
run=scenario()
def tick(dt):
    if state['busy'] or time.monotonic()<state['next']: return
    state['busy']=True
    try: state['next']=time.monotonic()+next(run)
    except StopIteration: finish()
    except Exception: u.log_error(traceback.format_exc()); finish()
    finally: state['busy']=False
def finish():
    u.unregister_slate_post_tick_callback(handle); levels.editor_request_end_play(); u.SystemLibrary.quit_editor()
handle=u.register_slate_post_tick_callback(tick)
