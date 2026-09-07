"""Actual PIE reverse encounter: sight, spatial sound, chair, free return, normal route.
Use -AudioMixer -ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1.0 for hidden runs.
Never saves the map.
"""
import json, math, time, traceback, wave, array
from pathlib import Path
import unreal as u
u.EditorPythonScripting.set_keep_python_script_alive(True)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
out=Path(u.Paths.project_saved_dir())/'ShowcaseExpansion'
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
report={'checks':[],'events':[]}
state={'busy':False,'next':time.monotonic(),'start':time.monotonic()}
def check(test,message):
    assert test,message
    report['checks'].append(message)
    u.log_warning('REVERSE_CHECK '+message)
def cue(name,location):
    p=state['pawn'].get_actor_location()
    report['events'].append({'name':str(name),'location':[location.x,location.y,location.z],
                            'player':[p.x,p.y,p.z],'time':u.GameplayStatics.get_time_seconds(state['world'])})
def count(name): return sum(e['name']==name for e in report['events'])
def scenario():
    levels.editor_request_begin_play()
    while not editor.get_game_world(): yield .1
    world=editor.get_game_world()
    yield 1
    pawn=u.GameplayStatics.get_player_pawn(world,0)
    pc=u.GameplayStatics.get_player_controller(world,0)
    d=u.GameplayStatics.get_all_actors_of_class(world,u.ShowcaseLoopDirector)[0]
    state.update(world=world,pawn=pawn)
    d.on_horror_cue.add_callable(cue)
    chair=d.get_editor_property('reverse_chair')
    check(bool(chair and d.get_editor_property('reverse_focus_light') and d.get_editor_property('reverse_cutlery_sound')),'Reverse chair, light and sound are saved in the map')
    original=chair.get_actor_transform()
    tucked=d.get_editor_property('reverse_chair_tucked_pose')
    tone=d.get_editor_property('room_tone')
    baseline_volume=tone.get_editor_property('volume_multiplier')
    focus=d.get_editor_property('reverse_focus_light').get_component_by_class(u.LightComponent)
    saved_focus=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    focus_peak=next(a for a in saved_focus if a.get_actor_label()=='Loop_Reverse_TableLight').get_component_by_class(u.LightComponent).get_editor_property('intensity')
    check(focus.get_editor_property('intensity')<.01,'Reverse spotlight starts off on ordinary forward route')
    movement=pawn.get_component_by_class(u.CharacterMovementComponent)
    movement.set_movement_mode(u.MovementMode.MOVE_FLYING)
    height=pawn.get_component_by_class(u.CapsuleComponent).get_scaled_capsule_half_height()+24
    r=d.get_editor_property('loop_radius'); s=[220.0]
    def place(value,back=True):
        a=value/r
        pawn.set_actor_location(u.Vector((r-115)*math.sin(a),r-(r-115)*math.cos(a)+50,height),False,True)
        pc.set_control_rotation(u.Rotator(yaw=math.degrees(a)+(180 if back else 0)))
        movement.stop_movement_immediately(); s[0]=value
    def walk(target,back=True):
        while abs(s[0]-target)>.1:
            place(s[0]+max(-15,min(15,target-s[0])),back)
            yield .05
    def look_at_chair():
        location=pc.player_camera_manager.get_camera_location()
        target=chair.get_actor_bounds(False)[0]
        pc.set_control_rotation(u.MathLibrary.find_look_at_rotation(location,target))
    place(220)
    yield .4
    u.AudioMixerLibrary.start_recording_output(world,100)
    state['recording']=True
    yield from walk(-750)
    look_at_chair()
    yield 1.3
    check(count('ReverseScrapeBehind')==1,'Looking at the pulled chair produces one scrape behind')
    scrape=next(e for e in report['events'] if e['name']=='ReverseScrapeBehind')
    check(250<math.dist(scrape['location'][:2],scrape['player'][:2])<500,'Scrape is a real source 2.5–5m behind the approach')
    check((chair.get_actor_location()-original.translation).length()<.1,'Watched chair remains still')
    check(tone.get_editor_property('volume_multiplier')<baseline_volume*.20,'Approaching the reverse table suppresses the room tone')
    check(focus.get_editor_property('intensity')>focus_peak*.8,'The one table receives its dedicated warm light')
    check(d.get_editor_property('current_stage')==0 and not d.get_editor_property('escape_door').get_editor_property('revealed'),'Wrong-way scene does not unlock forward horror or escape')
    place(s[0],False)
    yield .8
    check((chair.get_actor_location()-tucked.translation).length()<.1,'Looking behind tucks the previously observed chair out of view')
    look_at_chair()
    yield .8
    camera=pc.player_camera_manager
    center,extent=chair.get_actor_bounds(False)
    view=camera.get_camera_location()
    center_hit=u.SystemLibrary.line_trace_single(world,view,center,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[pawn],u.DrawDebugTrace.NONE,True)
    upper_hit=u.SystemLibrary.line_trace_single(world,view,center+u.Vector(0,0,extent.z*.85),u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[pawn],u.DrawDebugTrace.NONE,True)
    report['reobserve']={'view':str(view),'rotation':str(camera.get_camera_rotation()),'center':str(center),
        'trace':str(center_hit.to_tuple() if center_hit else None),'upper_trace':str(upper_hit.to_tuple() if upper_hit else None)}
    yield from walk(-150,False)
    yield 1
    check(count('ReverseCutlery')==1,'Returning after rechecking the chair produces one cutlery cue')
    check(d.get_editor_property('reverse_encounter_stage')==3,'Reverse encounter completes without death or a forced teleport')
    yield from walk(220,False)
    yield 4
    check(tone.get_editor_property('volume_multiplier')>baseline_volume*.90,'Original ambience restores on return to the start')
    # A second visit must not mechanically repeat the same surprise.
    yield from walk(-750)
    look_at_chair(); yield .7
    place(s[0],False); yield .6
    yield from walk(220,False)
    check(count('ReverseScrapeBehind')==1 and count('ReverseCutlery')==1,'Reverse encounter is one-shot during this playthrough')
    yield from walk(4300,False)
    check(d.get_editor_property('current_stage')==1,'Returning and taking the proper route still starts normal 40m progression')
    u.AudioMixerLibrary.stop_recording_output(world,u.AudioRecordingExportType.WAV_FILE,'showcase_reverse_runtime',str(out))
    state['recording']=False
    yield 1
    with wave.open(str(out/'showcase_reverse_runtime.wav')) as wav:
        samples=array.array('h',wav.readframes(wav.getnframes()))
        peak=max(abs(v) for v in samples)/32768
    report['recorded_peak']=peak
    check(.001<peak<.99,'Recorded engine output contains real, unclipped audio')
    report['passed']=True
run=scenario()
def finish():
    if state.get('recording'): u.AudioMixerLibrary.stop_recording_output(state['world'],u.AudioRecordingExportType.WAV_FILE,'showcase_reverse_runtime',str(out))
    (out/'reverse_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    u.log_warning('SHOWCASE_REVERSE_TEST '+json.dumps({k:v for k,v in report.items() if k!='events'}))
    u.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play(); u.SystemLibrary.quit_editor()
def tick(dt):
    if state['busy'] or time.monotonic()<state['next']: return
    state['busy']=True
    try:
        assert time.monotonic()-state['start']<160,'Reverse test timeout'
        state['next']=time.monotonic()+next(run)
    except StopIteration: finish()
    except Exception:
        report['passed']=False; report['error']=traceback.format_exc()
        u.log_error(report['error']); finish()
    finally: state['busy']=False
handle=u.register_slate_post_tick_callback(tick)
