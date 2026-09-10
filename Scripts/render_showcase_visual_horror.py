"""Targeted visual QA renders for exit-sign reversal and random events. Never saves."""
import time
import traceback
from pathlib import Path
import unreal as u

u.EditorPythonScripting.set_keep_python_script_alive(True)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
actors_api = u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
preview = actors_api.spawn_actor_from_class(u.SceneCapture2D, u.Vector(550, 165, 185), u.Rotator())
preview.set_actor_label('RandomVisualEvents_QA_Capture')
out = Path(u.Paths.project_saved_dir()) / 'ShowcaseExpansion'
state = {'busy': False, 'next': time.monotonic(), 'start': time.monotonic()}


def scenario():
    levels.editor_request_begin_play()
    while not editor.get_game_world():
        yield .1
    yield 1
    world = editor.get_game_world()
    pawn = u.GameplayStatics.get_player_pawn(world, 0)
    pc = u.GameplayStatics.get_player_controller(world, 0)
    director = u.GameplayStatics.get_all_actors_of_class(world, u.ShowcaseLoopDirector)[0]
    camera = next(
        a for a in u.GameplayStatics.get_all_actors_of_class(world, u.SceneCapture2D)
        if a.get_actor_label() == 'RandomVisualEvents_QA_Capture')
    capture = camera.get_component_by_class(u.SceneCaptureComponent2D)
    target = u.RenderingLibrary.create_render_target2d(world, 1024, 768, u.TextureRenderTargetFormat.RTF_RGBA8)
    for key, value in [
        ('texture_target', target),
        ('capture_source', u.SceneCaptureSource.SCS_FINAL_COLOR_LDR),
        ('capture_every_frame', False),
        ('always_persist_rendering_state', True),
        ('fov_angle', 67.0),
    ]:
        capture.set_editor_property(key, value)
    capture.hide_actor_components(pawn)
    movement = pawn.get_component_by_class(u.CharacterMovementComponent)
    movement.set_movement_mode(u.MovementMode.MOVE_FLYING)
    height = pawn.get_component_by_class(u.CapsuleComponent).get_scaled_capsule_half_height() + 24
    x = [220]

    def place(value):
        x[0] = value
        pawn.set_actor_location(u.Vector(value, 165, height), False, True)
        movement.stop_movement_immediately()
        pc.set_control_rotation(u.Rotator())

    def walk(goal):
        while x[0] < goal:
            place(min(goal, x[0] + 40))
            yield .05

    def shot(name, location, focus, frames=6):
        camera.set_actor_location(location, False, True)
        camera.set_actor_rotation(u.MathLibrary.find_look_at_rotation(location, focus), True)
        for _ in range(frames):
            capture.capture_scene()
            yield .04
        u.RenderingLibrary.export_render_target(world, target, str(out), name + '.png')

    signs = list(director.get_editor_property('emergency_exit_signs'))
    first_sign = min(signs, key=lambda actor: abs(actor.get_actor_location().x - x[0]))
    focus = first_sign.get_actor_location()
    yield from shot('visual_exit_sign_forward', u.Vector(focus.x, 165, 185), focus)

    yield from walk(6300)
    yield 1
    nearest_sign = min(signs, key=lambda actor: abs(actor.get_actor_location().x - x[0]))
    focus = nearest_sign.get_actor_location()
    yield from shot('visual_exit_sign_reversed', u.Vector(focus.x, 165, 185), focus)

    captured = set()
    deadline = time.monotonic() + 50
    while time.monotonic() < deadline and len(captured) < 3:
        event = str(director.get_editor_property('current_horror_event')).lower()
        if 'red_pulse' in event and 'red' not in captured:
            captured.add('red')
            yield from shot('visual_event_red', u.Vector(x[0], 165, 185), u.Vector(x[0] + 1500, 165, 250), 3)
        elif ('forward_blackout' in event or 'chair_reveal' in event) and 'blackout' not in captured:
            captured.add('blackout')
            yield from shot('visual_event_blackout', u.Vector(x[0], 165, 185), u.Vector(x[0] + 1500, 165, 250), 3)
        elif 'flicker_out' in event and 'flicker' not in captured:
            captured.add('flicker')
            yield from shot('visual_event_flicker', u.Vector(x[0], 165, 185), u.Vector(x[0] + 1500, 165, 250), 3)
        yield .04
    assert captured == {'red', 'blackout', 'flicker'}, 'Timed out before all lighting-event renders.'
    u.log_warning('SHOWCASE_RANDOM_VISUAL_RENDER_PASS')


run = scenario()


def finish():
    u.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play()
    u.SystemLibrary.quit_editor()


def tick(_delta):
    if state['busy'] or time.monotonic() < state['next']:
        return
    state['busy'] = True
    try:
        assert time.monotonic() - state['start'] < 75, 'Random visual render timeout'
        state['next'] = time.monotonic() + next(run)
    except StopIteration:
        finish()
    except Exception:
        u.log_error(traceback.format_exc())
        finish()
    finally:
        state['busy'] = False


handle = u.register_slate_post_tick_callback(tick)
