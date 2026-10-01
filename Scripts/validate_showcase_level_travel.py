"""PIE check: approach opens door; only crossing it travels to TestLevel."""
import unreal as u
import time
import json
import traceback
from pathlib import Path
u.EditorPythonScripting.set_keep_python_script_alive(True)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
state = {'next': 0, 'busy': False, 'start': time.monotonic()}
report = {'checks': []}

def scenario():
    levels.editor_request_begin_play()
    while not editor.get_game_world(): yield .1
    yield 1
    world = editor.get_game_world()
    door = u.GameplayStatics.get_all_actors_of_class(world, u.ShowcaseEscapeDoor)[0]
    director = u.GameplayStatics.get_all_actors_of_class(world, u.ShowcaseLoopDirector)[0]
    director.set_actor_tick_enabled(False)
    pawn = u.GameplayStatics.get_player_pawn(world, 0)
    move = pawn.get_component_by_class(u.CharacterMovementComponent)
    move.set_movement_mode(u.MovementMode.MOVE_FLYING)
    door.reveal_at(u.Transform(location=u.Vector(500, 165, 20)))
    pawn.set_actor_location(u.Vector(700, 165, 120), False, True)
    deadline = time.monotonic() + 20
    while door.get_open_amount() < .99:
        yield .1
        assert time.monotonic() < deadline, 'Door failed to open'
    assert u.GameplayStatics.get_current_level_name(editor.get_game_world(), True) == 'Showcase1'
    report['checks'].append('Approaching opens door without premature travel')
    pawn.set_actor_location(u.Vector(540, 165, 120), False, True)
    yield .2
    pawn.set_actor_location(u.Vector(490, 165, 120), False, True)
    deadline = time.monotonic() + 30
    while True:
        yield .2
        world = editor.get_game_world()
        if world and u.GameplayStatics.get_current_level_name(world, True) == 'TestLevel': break
        assert time.monotonic() < deadline, 'No level travel after crossing'
    yield 1
    pawn = u.GameplayStatics.get_player_pawn(world, 0)
    assert pawn and abs(pawn.get_actor_location().y + 14960) < 150
    assert len(u.GameplayStatics.get_all_actors_of_class(world, u.FallRestartVolume)) == 1
    report['checks'].append('Crossing loads TestLevel at PlayerStart with fall restart active')
    report['passed'] = True

run = scenario()
def tick(_dt):
    if state['busy'] or time.monotonic() < state['next']: return
    state['busy'] = True
    done = False
    try:
        assert time.monotonic() - state['start'] < 60
        state['next'] = time.monotonic() + next(run)
    except StopIteration: done = True
    except Exception:
        report.update(passed=False, error=traceback.format_exc())
        done = True
    finally: state['busy'] = False
    if done:
        Path(u.Paths.project_saved_dir(), 'showcase_travel_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
        u.log_warning('SHOWCASE_TRAVEL_TEST ' + json.dumps(report))
        u.unregister_slate_post_tick_callback(handle)
        levels.editor_request_end_play()
        u.SystemLibrary.quit_editor()
handle = u.register_slate_post_tick_callback(tick)
