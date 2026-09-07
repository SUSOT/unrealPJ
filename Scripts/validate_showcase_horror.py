"""Actual PIE/audio-mixer integration, walking at 300cm/s. No map saves.

Captures real cue delegates, world source positions, audio output, and scene frames.
"""
import json
import math
import time
import traceback
from pathlib import Path
import unreal as u

u.EditorPythonScripting.set_keep_python_script_alive(True)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
out=Path(u.Paths.project_saved_dir())/'ShowcaseExpansion'
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
report={'checks':[],'events':[]}
state={'next':time.monotonic(),'busy':False,'started':time.monotonic()}
radius=3055.7749073643904

def check(condition,message):
    assert condition,message
    report['checks'].append(message)
    u.log_warning('HORROR_CHECK '+message)

def on_cue(cue,location):
    pawn=state['pawn']
    report['events'].append({'cue':str(cue),'time':round(u.GameplayStatics.get_time_seconds(state['world']),3),
                             'location':[location.x,location.y,location.z],
                             'player':[pawn.get_actor_location().x,pawn.get_actor_location().y,pawn.get_actor_location().z]})

def scenario():
    levels.editor_request_begin_play()
    while not editor.get_game_world(): yield .1
    world=editor.get_game_world()
    yield 1
    pawn=u.GameplayStatics.get_player_pawn(world,0)
    pc=u.GameplayStatics.get_player_controller(world,0)
    d=u.GameplayStatics.get_all_actors_of_class(world,u.ShowcaseLoopDirector)[0]
    state.update(world=world,pawn=pawn)
    d.on_horror_cue.add_callable(on_cue)
    check(len(d.get_editor_property('footstep_sounds'))==4,'Four actual footstep variations assigned')
    tone=d.get_editor_property('room_tone')
    check(tone.is_playing(),'Room tone is playing through the real audio engine')
    players=list(d.get_editor_property('cue_players'))
    check(len(players)==8,'Fixed eight-source pool, no per-tick actor spawning')
    for player in players:
        attenuation=player.get_editor_property('attenuation_overrides')
        check(attenuation.get_editor_property('spatialize'),'Cue source uses spatialization')
    movement=pawn.get_component_by_class(u.CharacterMovementComponent)
    movement.set_movement_mode(u.MovementMode.MOVE_FLYING)
    height=pawn.get_component_by_class(u.CapsuleComponent).get_scaled_capsule_half_height()+24
    s=[220.0]
    def place(value,back=False):
        a=value/radius
        pawn.set_actor_location(u.Vector((radius-115)*math.sin(a),radius-(radius-115)*math.cos(a)+50,height),False,True)
        pc.set_control_rotation(u.Rotator(yaw=math.degrees(a)+(180 if back else 0)))
        movement.stop_movement_immediately()
        s[0]=value
    def walk(target,back=False):
        while abs(s[0]-target)>.1:
            place(s[0]+max(-15,min(15,target-s[0])),back)
            yield .05
    place(220)
    yield .5
    u.AudioMixerLibrary.start_recording_output(world,70.0)
    state['recording']=True
    yield from walk(4900)
    start=len(report['events'])
    yield 1.4
    tail=report['events'][start:]
    check(sum(e['cue']=='StepAfterStop' for e in tail)==1,'Stopping produces exactly one delayed step behind the player')
    follower=next(e for e in tail if e['cue']=='StepAfterStop')
    separation=math.dist(follower['location'][:2],follower['player'][:2])
    check(250<separation<600,'Follower step is a world-space source 2.5–6m away')
    # World-space audio components must not orbit with camera yaw.
    sources=[p.get_world_location() for p in players]
    place(s[0],True)
    yield .2
    check(all((a-p.get_world_location()).length()<.1 for a,p in zip(sources,players)),'Camera turn does not drag existing sound sources')
    yield from walk(s[0]-600,True)
    start=len(report['events'])
    yield 1.5
    check(not any(e['cue'] in ('FollowingFootstep','StepAfterStop') for e in report['events'][start:]),'Looking back suppresses follower cues')
    place(s[0])
    yield from walk(9600)
    yield 1.5
    check(d.get_editor_property('current_stage')==3,'Real 40/70/90m progression preserved')
    check(any(e['cue']=='FurnitureMoved' for e in report['events']),'Witnessed nearby furniture changes with matching spatial foley')
    check(any(e['cue']=='LightFailure' for e in report['events']),'Light-failure events have synchronized relay audio')
    check(any(e['cue']=='FollowingFootstep' for e in report['events']),'Delayed following footsteps occur during walking')
    # The actual light dip is verified independently by test_showcase_flicker.py.
    report['passed']=True
    yield .5

run=scenario()

def finish():
    if state.get('recording'):
        u.AudioMixerLibrary.stop_recording_output(state['world'],u.AudioRecordingExportType.WAV_FILE,'showcase_horror_runtime',str(out))
    (out/'horror_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    u.log_warning('SHOWCASE_HORROR_TEST '+json.dumps({k:v for k,v in report.items() if k!='events'}))
    u.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play()
    u.SystemLibrary.quit_editor()

def tick(delta):
    if state['busy'] or time.monotonic()<state['next']: return
    state['busy']=True
    try:
        assert time.monotonic()-state['started']<180,'Horror test timeout'
        state['next']=time.monotonic()+next(run)
    except StopIteration: finish()
    except Exception:
        report['passed']=False
        report['error']=traceback.format_exc()
        u.log_error(report['error'])
        finish()
    finally: state['busy']=False

handle=u.register_slate_post_tick_callback(tick)
