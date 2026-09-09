"""Real PIE geometry, infinite-cell reuse, instant turn-back exit and audio checks.
Run with audio mixer and process-only UnfocusedVolumeMultiplier=1. Never saves.
"""
import unreal as u, math, time, json, traceback, wave, array
from pathlib import Path
u.EditorPythonScripting.set_keep_python_script_alive(True)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
out=Path(u.Paths.project_saved_dir())/'ShowcaseExpansion'
report={'checks':[],'events':[]}
state={'busy':False,'next':time.monotonic(),'start':time.monotonic()}
def check(condition,message):
    assert condition,message
    report['checks'].append(message); u.log_warning('STRAIGHT_CHECK '+message)
def cue(name,location): report['events'].append({'name':str(name),'location':[location.x,location.y,location.z]})
def floor(world,x):
    hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,165,60),u.Vector(x,165,-50),u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],u.DrawDebugTrace.NONE,True)
    return bool(hit and hit.to_tuple()[0] and abs(hit.to_tuple()[4].z-20)<1.1)
def scenario():
    levels.editor_request_begin_play()
    while not editor.get_game_world(): yield .1
    yield 1
    world=editor.get_game_world(); state['world']=world
    pawn=u.GameplayStatics.get_player_pawn(world,0); pc=u.GameplayStatics.get_player_controller(world,0)
    d=u.GameplayStatics.get_all_actors_of_class(world,u.ShowcaseLoopDirector)[0]
    e=u.GameplayStatics.get_all_actors_of_class(world,u.ShowcaseRepeatExtension)[0]
    door=d.get_editor_property('escape_door'); d.on_horror_cue.add_callable(cue)
    movement=pawn.get_component_by_class(u.CharacterMovementComponent)
    movement.set_movement_mode(u.MovementMode.MOVE_FLYING)
    capsule=pawn.get_component_by_class(u.CapsuleComponent)
    height=capsule.get_scaled_capsule_half_height()+24
    x=[pawn.get_actor_location().x]
    def place(value,yaw=0):
        pawn.set_actor_location(u.Vector(value,165,height),False,True)
        movement.stop_movement_immediately(); pc.set_control_rotation(u.Rotator(yaw=yaw)); x[0]=value
    def walk(target,yaw=0):
        while abs(x[0]-target)>.1:
            place(x[0]+max(-30,min(30,target-x[0])),yaw); yield .05
    check(d.get_editor_property('straight_corridor') and e.get_editor_property('infinite_straight'),'Saved map enables straight direction and recycled cells')
    check(not door.get_editor_property('revealed'),'No door at the beginning')
    check(all(floor(world,i*37.5) for i in range(-360,361)),'721 initial floor samples have no gaps')
    collisions=[]
    for i in range(-350,350):
        p=u.Vector(i*37.5,165,height)
        hit=u.SystemLibrary.capsule_trace_single(world,p,p+u.Vector(37.5,0,0),45,88,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[pawn],u.DrawDebugTrace.NONE,True)
        if hit and hit.to_tuple()[0]: collisions.append(str(hit.to_tuple()[9]))
    check(not collisions,'700 capsule sweeps leave the straight aisle clear: '+str(collisions[:3]))
    u.AudioMixerLibrary.start_recording_output(world,100); state['recording']=True
    yield from walk(-500,180)
    check(d.get_editor_property('current_stage')==0 and not door.get_editor_property('revealed'),'Early reversal cannot reveal the exit')
    yield from walk(220)
    yield from walk(2050)
    check(d.get_editor_property('current_stage')==1,'First dread stage starts after 18m')
    yield .9
    check(any(a['name']=='DistantKnock' for a in report['events']),'Actual spatial knock cue is emitted')
    yield from walk(4050)
    check(d.get_editor_property('current_stage')==2,'Second dread stage starts after 38m')
    yield from walk(6300)
    yield .3
    check(d.get_editor_property('current_stage')==3 and door.get_editor_property('revealed'),'At 60m the door is already waiting behind the player')
    delta=pawn.get_actor_location()-door.get_actor_location()
    check(440<delta.x<1000 and abs(delta.y)<1,'Door is nearby behind in the clear aisle, with table clearance')
    before=pawn.get_actor_location(); pc.set_control_rotation(u.Rotator(yaw=180)); yield .2
    camera=pc.player_camera_manager
    view=camera.get_camera_location(); target=door.get_actor_location()+u.Vector(0,0,160)
    offset=target-view; direction=offset/offset.length(); rotation=camera.get_camera_rotation()
    yaw=math.radians(rotation.yaw); pitch=math.radians(rotation.pitch)
    forward=u.Vector(math.cos(pitch)*math.cos(yaw),math.cos(pitch)*math.sin(yaw),math.sin(pitch))
    check(direction.x*forward.x+direction.y*forward.y+direction.z*forward.z>.8,'A stationary camera turn immediately shows the nearby door')
    check((pawn.get_actor_location()-before).length()<1,'No backward walk or player teleport is needed to reveal it')
    fixed=door.get_actor_location()
    yield from walk(x[0]+250,0)
    check((door.get_actor_location()-fixed).length()<1,'Observed exit stays fixed when looking away again')
    yield from walk(fixed.x+220,180); yield 1.6
    check(door.get_open_amount()>.99,'Door opens when approached')
    for offset in (180,140,100,60,30,0,-30):
        pawn.set_actor_location(u.Vector(fixed.x+offset,165,height),True,False)
        movement.stop_movement_immediately(); yield .12
        if door.get_editor_property('escaped'): break
    yield .8
    check(door.get_editor_property('escaped') and d.get_editor_property('escape_complete'),'Physical door crossing completes escape')
    check((pawn.get_actor_location()-door.get_editor_property('destination').get_actor_location()).length()<100,'Player reaches the safe destination')
    u.AudioMixerLibrary.stop_recording_output(world,u.AudioRecordingExportType.WAV_FILE,'showcase_straight_runtime',str(out)); state['recording']=False
    yield 1
    with wave.open(str(out/'showcase_straight_runtime.wav')) as wav: samples=array.array('h',wav.readframes(wav.getnframes()))
    peak=max(abs(v) for v in samples)/32768; report['audio_peak']=peak
    check(.001<peak<.99,'Real engine audio is non-silent and unclipped')
    levels.editor_request_end_play(); yield 1
    levels.editor_request_begin_play(); yield 2
    world=editor.get_game_world(); pawn=u.GameplayStatics.get_player_pawn(world,0)
    d=u.GameplayStatics.get_all_actors_of_class(world,u.ShowcaseLoopDirector)[0]
    e=u.GameplayStatics.get_all_actors_of_class(world,u.ShowcaseRepeatExtension)[0]
    check(d.get_editor_property('current_stage')==0 and not d.get_editor_property('escape_door').get_editor_property('revealed'),'Restart clears progress and the door')
    d.set_actor_tick_enabled(False)
    movement=pawn.get_component_by_class(u.CharacterMovementComponent); movement.set_movement_mode(u.MovementMode.MOVE_FLYING)
    initial=e.get_repeated_instance_count()
    for at in (1200,6000,12000,20000,35000,-20000,-45000,46000,0):
        pawn.set_actor_location(u.Vector(at,165,height),False,True); movement.stop_movement_immediately(); yield .5
        check(abs(pawn.get_actor_location().x-at)<1,'Streaming never teleports the player at x='+str(at))
        check(all(floor(world,at+offset) for offset in range(-11000,11001,1000)),'Recycled floor extends at least 110m both ways at x='+str(at))
        check(e.get_repeated_instance_count()==initial,'Instance budget remains fixed at x='+str(at))
    report['recycled_cells']=e.get_editor_property('recycled_cell_count')
    check(report['recycled_cells']>50,'More than 50 distant cells recycled through forward/backward travel')
    report['passed']=True
run=scenario()
def finish():
    if state.get('recording'): u.AudioMixerLibrary.stop_recording_output(state['world'],u.AudioRecordingExportType.WAV_FILE,'showcase_straight_runtime',str(out))
    (out/'straight_validation.json').write_text(json.dumps(report,indent=2))
    u.log_warning('SHOWCASE_STRAIGHT_TEST '+json.dumps({k:v for k,v in report.items() if k!='events'}))
    u.unregister_slate_post_tick_callback(handle); levels.editor_request_end_play(); u.SystemLibrary.quit_editor()
def tick(dt):
    if state['busy'] or time.monotonic()<state['next']: return
    state['busy']=True
    try:
        assert time.monotonic()-state['start']<200,'Straight validation timeout'
        state['next']=time.monotonic()+next(run)
    except StopIteration: finish()
    except Exception:
        report['passed']=False; report['error']=traceback.format_exc(); u.log_error(report['error']); finish()
    finally: state['busy']=False
handle=u.register_slate_post_tick_callback(tick)
