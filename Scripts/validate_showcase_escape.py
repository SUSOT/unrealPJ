"""Full-editor geometry checks and real PIE progression/door traversal. Never saves."""
import json
import math
import time
import traceback
from pathlib import Path
import unreal as u

u.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(u.Paths.project_saved_dir())/'ShowcaseExpansion'
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
api = u.get_editor_subsystem(u.EditorActorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
report = {'checks':[], 'errors':[]}
radius = 3055.7749073643904

def pos(s,z=114):
    angle=s/radius
    return u.Vector((radius-115)*math.sin(angle),radius-(radius-115)*math.cos(angle)+50,z)

def check(test,message):
    assert test,message
    report['checks'].append(message)
    u.log_warning('ESCAPE_CHECK '+message)

def geometry():
    world=editor.get_editor_world()
    actors=list(api.get_all_level_actors())
    d=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector))
    ext=next(a for a in actors if isinstance(a,u.ShowcaseRepeatExtension))
    check(ext.get_repeated_instance_count()==2336,'2336 HISM instances retained')
    changes=list(d.get_editor_property('spatial_changes'))
    originals={c.get_editor_property('target'):c.get_editor_property('target').get_actor_transform() for c in changes}
    check(len(originals)==31 and len(changes)==31,'31 forward-route props plus one independent reverse-route chair')
    reverse_chair=d.get_editor_property('reverse_chair')
    check(bool(reverse_chair),'Reverse-route chair assigned')
    originals[reverse_chair]=reverse_chair.get_actor_transform()
    check([d.get_editor_property(k) for k in ('first_change_distance','second_change_distance','turn_back_unlock_distance')]==[4000,7000,9000],'Saved map uses 40 / 70 / 90 m walking pace')
    errors=[]
    for stage in range(4):
        if stage:
            reverse_chair.set_actor_transform(d.get_editor_property('reverse_chair_tucked_pose'),False,True)
            for c in changes:
                if c.get_editor_property('stage')==stage:
                    c.get_editor_property('target').set_actor_transform(c.get_editor_property('changed_transform'),False,True)
        for i in range(512):
            s=i*37.5
            floor=u.SystemLibrary.line_trace_single(world,pos(s,60),pos(s,-50),u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],u.DrawDebugTrace.NONE,True)
            # The existing continuous underlay is at z=19; a line exactly on a
            # tile edge can hit that instead of the visible surface at z=20.
            if not floor or not floor.to_tuple()[0] or abs(floor.to_tuple()[4].z-20)>1.1: errors.append('floor stage%d s%.1f: %s'%(stage,s,str(floor)))
            for direction in (-1,1):
                hit=u.SystemLibrary.capsule_trace_single(world,pos(s),pos(s+direction*37.5),45,88,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],u.DrawDebugTrace.NONE,True)
                if hit and hit.to_tuple()[0]: errors.append('aisle stage%d s%.1f %s'%(stage,s,hit.to_tuple()[9]))
    for a,t in originals.items(): a.set_actor_transform(t,False,True)
    check(not errors,'All 4 stages: 2048 floor / 4096 bidirectional capsule checks clear'+(' '+str(errors[:10]) if errors else ''))
    # Destination capsule must be free, with a floor and enclosing walls.
    dest=d.get_editor_property('escape_door').get_editor_property('destination').get_actor_location()
    hit=u.SystemLibrary.capsule_trace_single(world,dest+u.Vector(0,0,3),dest+u.Vector(20,0,3),45,88,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],u.DrawDebugTrace.NONE,True)
    check(not hit or not hit.to_tuple()[0],'Escape destination capsule clearance')
    return originals

def scenario():
    geometry()
    levels.editor_request_begin_play()
    start=time.monotonic()
    while not editor.get_game_world():
        assert time.monotonic()-start<60,'PIE did not start'
        yield .2
    world=editor.get_game_world()
    yield 1.0
    pawn=u.GameplayStatics.get_player_pawn(world,0)
    pc=u.GameplayStatics.get_player_controller(world,0)
    check(bool(pawn and pc),'PIE uses a possessed player pawn')
    d=u.GameplayStatics.get_all_actors_of_class(world,u.ShowcaseLoopDirector)[0]
    door=d.get_editor_property('escape_door')
    check(d.get_editor_property('current_stage')==0 and not door.get_editor_property('revealed'),'PIE begins ordinary with no exit door')
    movement=pawn.get_component_by_class(u.CharacterMovementComponent)
    if movement: movement.set_movement_mode(u.MovementMode.MOVE_FLYING)
    height=pawn.get_component_by_class(u.CapsuleComponent).get_scaled_capsule_half_height()+24
    current=[220.0]
    def place(s,looking_back=False):
        pawn.set_actor_location(pos(s,height),False,True)
        pc.set_control_rotation(u.Rotator(yaw=math.degrees(s/radius)+(180 if looking_back else 0)))
        if movement: movement.stop_movement_immediately()
        current[0]=s
    def walk_to(target,looking_back=False):
        while abs(current[0]-target)>.1:
            delta=max(-40,min(40,target-current[0]))
            place(current[0]+delta,looking_back)
            yield .09
    place(220)
    yield .3
    yield from walk_to(-380,True)
    check(not door.get_editor_property('revealed') and d.get_editor_property('current_stage')==0,'Early reversal does not unlock exit')
    yield from walk_to(4300)
    check(d.get_editor_property('current_stage')==1,'First spatial change at 40 m')
    yield from walk_to(7400)
    check(d.get_editor_property('current_stage')==2,'Second spatial change at 70 m')
    yield from walk_to(9400)
    check(d.get_editor_property('current_stage')==3 and not door.get_editor_property('revealed'),'90 m unlocks reversal, not the door itself')
    place(current[0],True)
    yield .6
    check(not door.get_editor_property('revealed'),'Turning camera alone cannot reveal door')
    place(current[0],False)
    yield .3
    yield from walk_to(current[0]-600,False)
    check(not door.get_editor_property('revealed'),'Backpedalling without turning around cannot reveal door')
    yield from walk_to(current[0]-300,True)
    yield from walk_to(current[0]+100,False)
    check(not door.get_editor_property('revealed'),'Short reversal followed by forward motion does not unlock')
    place(current[0],True)
    yield .3
    yield from walk_to(current[0]-650,True)
    yield .5
    check(door.get_editor_property('revealed'),'Turning back and walking 5 m reveals door')
    check(not door.get_editor_property('escaped'),'Door appearance alone does not count as escape')
    p=door.get_actor_location()
    door_s=math.atan2(p.x,-(p.y-radius-50))*radius
    while door_s>current[0]: door_s-=2*math.pi*radius
    while door_s<current[0]-2*math.pi*radius: door_s+=2*math.pi*radius
    check(1000<current[0]-door_s<4200,'Exit placed around the bend on the backtracked route')
    # Arrival is physically swept through the doorway, not just its event invoked.
    yield from walk_to(door_s+220,True)
    yield 1.6
    check(door.get_open_amount()>.99,'Door opens fully when approached')
    forward=door.get_actor_forward_vector()
    for x in (180,140,100,60,30,0,-30):
        target=p+forward*x+u.Vector(0,0,height-20)
        pawn.set_actor_location(target,True,False)
        if movement: movement.stop_movement_immediately()
        yield .10
        if door.get_editor_property('escaped'): break
    yield .8
    check(door.get_editor_property('escaped') and d.get_editor_property('escape_complete'),'Crossing door fires escape completion')
    destination=door.get_editor_property('destination').get_actor_location()
    check((pawn.get_actor_location()-destination).length()<100,'Player arrives safely outside the repeating diner')
    report['final_progress_cm']=d.get_editor_property('forward_progress')
    report['pawn_class']=pawn.get_class().get_path_name()
    levels.editor_request_end_play()
    yield 1.0
    levels.editor_request_begin_play()
    yield 2.0
    world=editor.get_game_world()
    check(bool(world),'Second PIE starts')
    d=u.GameplayStatics.get_all_actors_of_class(world,u.ShowcaseLoopDirector)[0]
    check(d.get_editor_property('current_stage')==0 and not d.get_editor_property('escape_door').get_editor_property('revealed'),'Restart resets progression and hides doorway')
    levels.editor_request_end_play()
    yield 1.0

state={'next':time.monotonic()+1,'busy':False,'started':time.monotonic()}
run=scenario()
def tick(delta):
    if state['busy'] or time.monotonic()<state['next']: return
    state['busy']=True
    try:
        assert time.monotonic()-state['started']<240,'Validation timed out'
        state['next']=time.monotonic()+next(run)
    except StopIteration:
        report['passed']=True
        finish()
    except Exception:
        report['errors'].append(traceback.format_exc())
        u.log_error(report['errors'][-1])
        report['passed']=False
        finish()
    finally: state['busy']=False
def finish():
    (out/'escape_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    u.log_warning('SHOWCASE_ESCAPE_VALIDATION '+json.dumps(report))
    u.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()
    u.SystemLibrary.quit_editor()
handle=u.register_slate_post_tick_callback(tick)
