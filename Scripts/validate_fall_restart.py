"""PIE: verify safe ground, then both capsule and independent ragdoll restart."""
import unreal as u
import time
import json
import traceback
from pathlib import Path

u.EditorPythonScripting.set_keep_python_script_alive(True)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
assert levels.load_level('/Game/Developers/LCM/Levels/TestLevel')
report = {'checks': []}
state = {'next': 0, 'start': time.monotonic(), 'busy': False}

def scenario():
    levels.editor_request_begin_play()
    while not editor.get_game_world():
        yield .1
    yield 2
    for ragdoll in [False, True]:
        world = editor.get_game_world()
        pawn = u.GameplayStatics.get_player_pawn(world, 0)
        start = pawn.get_actor_location()
        yield 1
        assert pawn == u.GameplayStatics.get_player_pawn(editor.get_game_world(), 0)
        assert len(u.GameplayStatics.get_all_actors_of_class(world, u.FallRestartVolume)) == 1
        if ragdoll:
            mesh = pawn.get_component_by_class(u.SkeletalMeshComponent)
            mesh.set_collision_profile_name('Ragdoll')
            mesh.set_simulate_physics(True)
            mesh.set_world_location(u.Vector(4000, 0, -1800), False, True)
            assert pawn.get_actor_location().z > -1500
        else:
            pawn.set_actor_location(u.Vector(4000, 0, -1800), False, True)
        deadline = time.monotonic() + 25
        while True:
            yield .2
            new_world = editor.get_game_world()
            new_pawn = u.GameplayStatics.get_player_pawn(new_world, 0) if new_world else None
            if new_pawn and new_pawn != pawn:
                break
            assert time.monotonic() < deadline, 'Level did not restart'
        yield 1
        location = new_pawn.get_actor_location()
        assert abs(location.y - start.y) < 150 and abs(location.x - start.x) < 150 and location.z > 0
        report['checks'].append('Ragdoll reload and start restored' if ragdoll else 'Capsule reload and start restored')
    report['passed'] = True

run = scenario()
def tick(_delta):
    if state['busy'] or time.monotonic() < state['next']: return
    state['busy'] = True
    done = False
    try:
        assert time.monotonic() - state['start'] < 80
        state['next'] = time.monotonic() + next(run)
    except StopIteration:
        done = True
    except Exception:
        report.update(passed=False, error=traceback.format_exc())
        done = True
    finally:
        state['busy'] = False
    if done:
        Path(u.Paths.project_saved_dir(), 'fall_restart_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
        u.log_warning('FALL_RESTART_TEST ' + json.dumps(report))
        u.unregister_slate_post_tick_callback(handle)
        levels.editor_request_end_play()
        u.SystemLibrary.quit_editor()
handle = u.register_slate_post_tick_callback(tick)
