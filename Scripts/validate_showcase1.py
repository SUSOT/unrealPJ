"""Read-only reload, source-preservation, fixture and collision checks."""
import json
from pathlib import Path
import unreal

output = Path(unreal.Paths.project_saved_dir()) / 'ShowcaseExpansion'
before = json.loads((output / 'inspection.json').read_text(encoding='utf-8'))
result = json.loads((output / 'result.json').read_text(encoding='utf-8'))
if not globals().get('USE_LOADED_WORLD', False):
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(result['map'])
subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = list(subsystem.get_all_level_actors())
by_label = {a.get_actor_label(): a for a in actors}
extensions = [a for a in actors if isinstance(a, unreal.ShowcaseRepeatExtension)]
assert len(extensions) == 1
extension = extensions[0]
assert len(extension.get_editor_property('repeated_fixtures')) == 392
# This is the same once-only policy invoked by BeginPlay, after Blueprint construction.
extension.apply_fixture_distance_limits()
assert extension.get_repeated_instance_count() == 33320
components = extension.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
assert len(components) == 22
caps = {item['actor'] for item in result['moved_endcaps']}
errors = []
for old in before['actors']:
    actor = by_label.get(old['label'])
    if not actor:
        errors.append('Missing original actor ' + old['label'])
        continue
    t = actor.get_actor_transform()
    expected = old['transform']['translation'][:]
    if old['label'] in caps:
        expected[0] += 235200.0
    actual = [t.translation.x, t.translation.y, t.translation.z]
    if max(abs(a-b) for a, b in zip(actual, expected)) > 0.02:
        errors.append('Unexpected location ' + old['label'])
    actual_rotation = [t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w]
    expected_rotation = old['transform']['rotation']
    if min(max(abs(a-b) for a,b in zip(actual_rotation, expected_rotation)),
           max(abs(a+b) for a,b in zip(actual_rotation, expected_rotation))) > 0.0001:
        errors.append('Unexpected rotation ' + old['label'])
    if max(abs(a-b) for a,b in zip([t.scale3d.x,t.scale3d.y,t.scale3d.z], old['transform']['scale'])) > 0.0001:
        errors.append('Unexpected scale ' + old['label'])

lamps = [a for a in actors if a.get_actor_label().startswith('Endless_Lamp_')]
cables = [a for a in actors if a.get_actor_label().startswith('Endless_Cable_')]
assert len(lamps) == len(cables) == 196
source_lamp = next(a for a in actors if str(a.get_folder_path()) == '3' and 'Ceilinglamp' in a.get_class().get_name())
source_lights = source_lamp.get_components_by_class(unreal.LightComponent)
for lamp in lamps:
    lights = lamp.get_components_by_class(unreal.LightComponent)
    assert len(lights) == 2
    cell_index = int(lamp.get_actor_label().rsplit('_', 1)[1]) - 5
    offset = unreal.Vector(result['extension_start_x_cm'] - 1980.0 + cell_index * 1200.0, 0, 0)
    if (lamp.get_actor_location() - source_lamp.get_actor_location() - offset).length() > 0.02:
        errors.append('Lamp transform mismatch: ' + lamp.get_actor_label())
    for light, source_light in zip(lights, source_lights):
        if (light.get_world_location() - source_light.get_world_location() - offset).length() > 0.02:
            errors.append('Light position mismatch: ' + lamp.get_actor_label())
        if light.get_editor_property('mobility') != unreal.ComponentMobility.MOVABLE:
            errors.append('Light must be movable: ' + lamp.get_actor_label())
        if light.get_editor_property('max_draw_distance') != 7000.0:
            errors.append('Light distance reset: ' + lamp.get_actor_label())
        if light.get_editor_property('max_distance_fade_range') != 2000.0:
            errors.append('Light fade reset: ' + lamp.get_actor_label())
source_cable = next(a for a in actors if str(a.get_folder_path()) == '3' and 'BP_Cablespline' in a.get_class().get_name())
source_splines = source_cable.get_components_by_class(unreal.SplineMeshComponent)
for cable in cables:
    cell_index = int(cable.get_actor_label().rsplit('_', 1)[1]) - 5
    offset = unreal.Vector(result['extension_start_x_cm'] - 1980.0 + cell_index * 1200.0, 0, 0)
    if (cable.get_actor_location() - source_cable.get_actor_location() - offset).length() > 0.02:
        errors.append('Cable transform mismatch: ' + cable.get_actor_label())
    splines = cable.get_components_by_class(unreal.SplineMeshComponent)
    if len(splines) != len(source_splines):
        errors.append('Cable shape mismatch: ' + cable.get_actor_label())
    for component, source in zip(splines, source_splines):
        if (component.get_world_location() - source.get_world_location() - offset).length() > 0.02:
            errors.append('Cable component position mismatch: ' + cable.get_actor_label())
        if (component.get_start_position() - source.get_start_position()).length() > 0.01:
            errors.append('Cable start changed: ' + cable.get_actor_label())
        if (component.get_end_position() - source.get_end_position()).length() > 0.01:
            errors.append('Cable end changed: ' + cable.get_actor_label())

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
def trace(start, end):
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(*start), unreal.Vector(*end),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    if hit is None:
        return None
    return hit.to_tuple()

floor_tests = 0
for index in range(196):
    x = result['extension_start_x_cm'] + index * 1200
    for delta in (-2.0, 2.0):
        hit = trace((x+delta, 100, 80), (x+delta, 100, -50))
        floor_tests += 1
        if not hit or not hit[0] or abs(hit[4].z - 20.0) > 2:
            errors.append('Floor discontinuity at %.2f: %s' % (x+delta, str(hit)))
passage = trace((4200, 100, 180), (235000, 100, 180))
if passage and passage[0]:
    errors.append('Corridor passage obstructed: ' + str(passage))
end_wall = trace((239200, 100, 180), (239600, 100, 180))
if not end_wall or not end_wall[0] or end_wall[9].get_actor_label() not in caps:
    errors.append('Far end wall missing')

report = {'actor_count': len(actors), 'original_actors_preserved': len(before['actors']),
          'instances': extension.get_repeated_instance_count(), 'hism_components': len(components),
          'lamps': len(lamps), 'cables': len(cables), 'cable_segments_per_actor': len(source_splines),
          'floor_seam_collision_checks': floor_tests, 'errors': errors}
(output / 'validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log_warning('SHOWCASE_VALIDATION ' + json.dumps(report))
assert not errors, 'Showcase validation failed; see validation.json'
