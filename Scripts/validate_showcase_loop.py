"""Run in a full editor, not a commandlet: collision scenes need editor initialization."""
import json
import math
import re
from pathlib import Path
import unreal as u

out = Path(u.Paths.project_saved_dir()) / 'ShowcaseExpansion'
report = json.loads((out / 'loop_result.json').read_text(encoding='utf-8'))
if not globals().get('USE_LOADED_WORLD', False):
    assert u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(report['map'])
actors = list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
extension = next(a for a in actors if isinstance(a, u.ShowcaseRepeatExtension))
assert extension.get_repeated_instance_count() == report['instances']
assert len(extension.get_editor_property('repeated_fixtures')) == 32
assert not any(a.get_actor_label().startswith('Endless_') for a in actors)
extension.apply_fixture_distance_limits()
radius = report['radius_cm']
errors = []

def pos(s, y, z):
    angle = s / radius
    r = radius - (y - 50)
    return u.Vector(r * math.sin(angle), radius - r * math.cos(angle) + 50, z)

def trace(start, end):
    hit = u.SystemLibrary.line_trace_single(world, start, end, u.TraceTypeQuery.TRACE_TYPE_QUERY1,
        False, [], u.DrawDebugTrace.NONE, True)
    return hit.to_tuple() if hit else None

# Every 37.5 cm, including the wrap from room 16 back to room 1.
for index in range(512):
    s = index * 37.5
    hit = trace(pos(s, 150, 60), pos(s, 150, -50))
    if not hit or not hit[0] or abs(hit[4].z - 20) > 1:
        errors.append('Floor gap at %.1f cm' % s)
    for direction in (-1, 1):
        hit = u.SystemLibrary.capsule_trace_single(world, pos(s, 165, 114), pos(s + direction * 37.5, 165, 114),
            45, 88, u.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], u.DrawDebugTrace.NONE, True)
        if hit:
            data = hit.to_tuple()
            if data[0]:
                mesh = data[10].get_editor_property('static_mesh') if data[10] else None
                errors.append('Passage obstruction at %.1f cm, direction %d: %s' % (s, direction, mesh.get_name() if mesh else str(data[9])))
    for y in (-450, 550):
        hit = trace(pos(s, 50, 250), pos(s, y, 250))
        if not hit or not hit[0]:
            errors.append('Wall gap at %.1f cm, side %.0f' % (s, y))

for cell in range(16):
    s = cell * 1200
    for delta in (-1, 1):
        hit = trace(pos(s+delta, 150, 60), pos(s+delta, 150, -50))
        if not hit or not hit[0] or abs(hit[4].z-20) > 1:
            errors.append('Cell seam gap at %.1f' % (s+delta))
for a in actors:
    if a.get_actor_label().startswith('Loop_Lamp_'):
        if abs(a.get_actor_location().z - 509.999084) > 0.1:
            errors.append('Lamp height mismatch: ' + a.get_actor_label())
        for component in a.get_components_by_class(u.LightComponent):
            if component.get_editor_property('mobility') != u.ComponentMobility.MOVABLE:
                errors.append('Static lamp: ' + a.get_actor_label())
old = json.loads((out / 'before_loop.json').read_text(encoding='utf-8'))
old_pp = next(a['post_process'] for a in old['actors'] if 'post_process' in a)
pp = next(a for a in actors if isinstance(a, u.PostProcessVolume))
def without_addresses(text):
    return re.sub(r'0x[0-9a-fA-F]+', '0xADDRESS', text)
if without_addresses(old_pp) != without_addresses(str(pp.get_editor_property('settings'))):
    errors.append('Post-processing unexpectedly changed')
validation = {'actors': len(actors), 'instances': extension.get_repeated_instance_count(),
    'floor_checks': 544, 'capsule_checks_both_directions': 1024, 'wall_checks': 1024,
    'post_process_unbound': pp.get_editor_property('unbound'), 'errors': errors}
(out / 'loop_validation.json').write_text(json.dumps(validation, indent=2), encoding='utf-8')
u.log_warning('SHOWCASE_LOOP_VALIDATION ' + json.dumps(validation))
assert not errors, 'Loop validation failed'
