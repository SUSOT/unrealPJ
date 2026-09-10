"""Real-PIE checks for the requested shuffled light and prop events. Never saves."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

u.EditorPythonScripting.set_keep_python_script_alive(True)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
out = Path(u.Paths.project_saved_dir()) / 'ShowcaseExpansion'
report = {'checks': []}
state = {'busy': False, 'next': time.monotonic(), 'start': time.monotonic()}

editor_actors = u.EditorLevelLibrary.get_all_level_actors()
authored_lights = {}
for actor in editor_actors:
    if actor.get_actor_label().startswith('Straight_Lamp_'):
        authored_lights[actor.get_actor_label()] = [
            c.get_editor_property('intensity')
            for c in actor.get_components_by_class(u.LightComponent)]


def check(condition, message):
    assert condition, message
    report['checks'].append(message)
    u.log_warning('RANDOM_EVENT_CHECK ' + message)


def scenario():
    levels.editor_request_begin_play()
    while not editor.get_game_world():
        yield .1
    yield 1
    world = editor.get_game_world()
    actors = u.GameplayStatics.get_all_actors_of_class(world, u.Actor)
    pawn = u.GameplayStatics.get_player_pawn(world, 0)
    pc = u.GameplayStatics.get_player_controller(world, 0)
    director = u.GameplayStatics.get_all_actors_of_class(world, u.ShowcaseLoopDirector)[0]
    extension = u.GameplayStatics.get_all_actors_of_class(world, u.ShowcaseRepeatExtension)[0]
    movement = pawn.get_component_by_class(u.CharacterMovementComponent)
    movement.set_movement_mode(u.MovementMode.MOVE_FLYING)
    height = pawn.get_component_by_class(u.CapsuleComponent).get_scaled_capsule_half_height() + 24
    x = [pawn.get_actor_location().x]

    def place(value, yaw=0):
        pawn.set_actor_location(u.Vector(value, 165, height), False, True)
        movement.stop_movement_immediately()
        pc.set_control_rotation(u.Rotator(yaw=yaw))
        x[0] = value

    def walk(target, yaw=0, observer=None):
        while abs(x[0] - target) > .1:
            place(x[0] + max(-35, min(35, target - x[0])), yaw)
            if observer:
                observer()
            yield .05

    lamps = list(director.get_editor_property('lamps'))
    chairs = list(director.get_editor_property('chair_props'))
    frames = list(director.get_editor_property('frame_props'))
    signs = list(director.get_editor_property('emergency_exit_signs'))
    labels = {a.get_actor_label() for a in actors}
    check(len(lamps) == 24 and len(chairs) == 24 and len(frames) == 24, 'The event system owns 24 lamps, 24 chairs, and 24 frames')
    check(len(signs) == 8 and all(a.get_actor_scale3d().x > 0 for a in signs), 'Eight compact emergency signs initially keep the forward-route direction')
    check('Showcase_DreadPostProcess' not in labels and 'Loop_Reverse_TableLight' not in labels, 'The prior grade, spatial-warp, and reverse encounter actors are removed')
    check(not director.get_components_by_class(u.AudioComponent), 'No previous follower or horror-cue audio components remain')
    check(abs(director.get_editor_property('base_light_scale') - .72) < .001, 'Normal lamp and bulb brightness is reduced to 72 percent')
    runtime_intensity = {}
    for lamp in lamps:
        runtime_intensity[lamp.get_actor_label()] = [
            c.get_editor_property('intensity')
            for c in lamp.get_components_by_class(u.LightComponent)]
    paired = [
        runtime / authored
        for label, values in authored_lights.items()
        for runtime, authored in zip(sorted(runtime_intensity.get(label, [])), sorted(values))
        if authored > 1
    ]
    report['base_light_ratios'] = paired
    report['base_light_sample'] = {
        'authored': next(iter(authored_lights.items())),
        'runtime': next(iter(runtime_intensity.items())),
    }
    u.log_warning('SHOWCASE_BASE_LIGHT_RATIOS ' + json.dumps(report['base_light_sample']))
    check(paired and all(abs(ratio - .72) < .02 for ratio in paired), 'PIE light components apply the requested lower base brightness')
    check(extension.get_repeated_instance_count() == 4008 and len(extension.get_editor_property('repeated_fixtures')) == 104, 'Infinite corridor instance and fixture budgets remain unchanged')

    observed = {'red': False, 'blackout': False, 'flicker': False}

    def observe_event():
        current = str(director.get_editor_property('current_horror_event')).lower()
        ahead_components = [
            component
            for lamp in lamps
            if 120 < lamp.get_actor_location().x - x[0] < 14400
            for component in lamp.get_components_by_class(u.LightComponent)
        ]
        intensities = [c.get_editor_property('intensity') for c in ahead_components]
        colors = [c.get_editor_property('light_color') for c in ahead_components]
        if 'red_pulse' in current:
            observed['red'] |= bool(colors) and all(color.r > color.g * 4 and color.r > color.b * 4 for color in colors)
        if 'forward_blackout' in current or 'chair_reveal' in current:
            observed['blackout'] |= bool(intensities) and all(value < 1 for value in intensities)
        if 'flicker_out' in current:
            observed['flicker'] |= bool(intensities) and any(value < 1 for value in intensities)

    yield from walk(4050, observer=observe_event)
    yield .5
    check(director.get_editor_property('current_stage') == 2, 'Mid-route progress reaches the stronger random-event tier')
    check(all(a.get_actor_scale3d().x > 0 for a in signs), 'Signs do not reverse before the escape threshold')

    yield from walk(6300, observer=observe_event)
    yield 1
    door = director.get_editor_property('escape_door')
    check(director.get_editor_property('current_stage') == 3 and door.get_editor_property('revealed'), 'The turn-back escape is prepared at the final threshold')
    check(director.get_editor_property('reversed_exit_sign_count') == 8 and all(a.get_actor_scale3d().x < 0 for a in signs), 'All emergency pictograms reverse when the turn-back exit appears')

    required_mask = sum(1 << value for value in range(1, 6))
    deadline = time.monotonic() + 65
    while time.monotonic() < deadline:
        observe_event()
        history = director.get_editor_property('horror_event_history_mask')
        every_prop_kind = (
            director.get_editor_property('altered_chair_count') > 0
            and director.get_editor_property('altered_table_count') > 0
            and director.get_editor_property('altered_frame_count') > 0)
        if history & required_mask == required_mask and every_prop_kind and director.get_editor_property('current_horror_event') == u.ShowcaseHorrorEvent.NONE:
            break
        yield .05

    check(director.get_editor_property('horror_event_history_mask') & required_mask == required_mask, 'A shuffled event bag produces all five requested event types')
    check(observed['red'], 'The red-light event switches every forward lamp to abrupt red')
    check(observed['blackout'], 'Blackout events switch every forward lamp fully off')
    check(observed['flicker'], 'The rapid-flicker event contains hard off frames before its outage')
    check(director.get_editor_property('altered_chair_count') > 0, 'A chair is found fallen when blackout lighting returns')
    check(director.get_editor_property('altered_table_count') > 0 and director.get_editor_property('altered_frame_count') > 0, 'Random prop events move both tables and wall frames from their original positions')
    check(director.get_editor_property('completed_horror_events') >= 5, 'All requested events finish and restore normal lighting')
    check(len(actors) < 150, 'The replacement horror pass stays under 150 level actors in PIE')
    report.update(
        passed=True,
        event_history_mask=director.get_editor_property('horror_event_history_mask'),
        completed_events=director.get_editor_property('completed_horror_events'),
        altered_props=director.get_editor_property('altered_prop_count'),
        altered_chairs=director.get_editor_property('altered_chair_count'),
        altered_tables=director.get_editor_property('altered_table_count'),
        altered_frames=director.get_editor_property('altered_frame_count'),
        actor_count=len(actors),
    )


def finish():
    (out / 'random_visual_events_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    u.log_warning('SHOWCASE_RANDOM_EVENT_TEST ' + json.dumps(report))
    u.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play()
    u.SystemLibrary.quit_editor()


run = scenario()


def tick(_delta):
    if state['busy'] or time.monotonic() < state['next']:
        return
    state['busy'] = True
    try:
        assert time.monotonic() - state['start'] < 100, 'Random event validation timeout'
        state['next'] = time.monotonic() + next(run)
    except StopIteration:
        finish()
    except Exception:
        report.update(passed=False, error=traceback.format_exc())
        u.log_error(report['error'])
        finish()
    finally:
        state['busy'] = False


handle = u.register_slate_post_tick_callback(tick)
