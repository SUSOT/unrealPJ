"""Extend the authored diner, preserving the original four rooms and entrance."""
import json
from pathlib import Path
import unreal

MAP = '/Game/Developers/MOON/Level/Showcase1'
REPEATS = 196
PITCH = 1200.0
SOURCE_MIN_X = 1980.0
END_CAPS = ['Cube4', 'Walls_SM_Wall_3x4m7', 'Walls_SM_Wall_3x4m9']
OUTPUT = Path(unreal.Paths.project_saved_dir()) / 'ShowcaseExpansion'

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(MAP):
    raise RuntimeError('Could not load Showcase1')
actors = actors_subsystem.get_all_level_actors()
if any(isinstance(actor, unreal.ShowcaseRepeatExtension) for actor in actors):
    raise RuntimeError('This map already has the extension; refusing to duplicate it twice.')

by_label = {actor.get_actor_label(): actor for actor in actors}
if any(label not in by_label for label in END_CAPS):
    raise RuntimeError('The original end wall no longer matches the inspected layout.')
template = [actor for actor in actors if str(actor.get_folder_path()) == '3']
static_sources = [actor for actor in template if isinstance(actor, unreal.StaticMeshActor)]
blueprint_sources = [actor for actor in template if 'BP_Cablespline' in actor.get_class().get_name() or 'PF_Ceilinglamp' in actor.get_class().get_name()]
if len(static_sources) != 170 or len(blueprint_sources) != 2:
    raise RuntimeError('Unexpected source-cell contents: %d meshes, %d blueprints' % (len(static_sources), len(blueprint_sources)))

last_floors = [actor for actor in actors if str(actor.get_folder_path()) == '4' and isinstance(actor, unreal.StaticMeshActor)
               and actor.static_mesh_component.static_mesh.get_name() == 'SM_Floor']
first_x = max(actor.get_actor_bounds(False)[0].x + actor.get_actor_bounds(False)[1].x for actor in last_floors)
groups = {}
for actor in static_sources:
    comp = actor.static_mesh_component
    mesh = comp.static_mesh
    materials = list(comp.get_materials())
    name = mesh.get_name()
    tiny = name.startswith('SM_Shaker') or name == 'SM_NapkinDispenser'
    noncolliding = tiny or name == 'SM_Decals' or 'Cables' in name or 'Pipes_Support' in name
    collision = not noncolliding and comp.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION
    cast_shadow = bool(comp.get_editor_property('cast_shadow')) and not tiny
    distance = 0
    if tiny:
        distance = 3000
    elif name == 'SM_Decals':
        distance = 4000
    elif 'WallPainting' in name or 'Chalkboard' in name:
        distance = 6000
    elif 'Cables' in name or 'Pipes_Support' in name:
        distance = 7000
    elif name in ('SM_PlantPot', 'SM_Chair'):
        distance = 10000
    elif name in ('SM_Table', 'SM_Booth'):
        distance = 18000

    key = (mesh.get_path_name(), tuple(m.get_path_name() if m else '' for m in materials), collision, cast_shadow, distance)
    if key not in groups:
        groups[key] = {'mesh': mesh, 'materials': materials, 'transforms': [], 'collision': collision, 'shadow': cast_shadow, 'distance': distance}

    t = comp.get_world_transform()
    p = t.translation
    if name == 'SM_Floor':
        # Use the same 3 m tiles but snap the new 12 m cells to avoid the old
        # 50 cm overlaps and the slightly shifted last tile of the source cell.
        p.x = SOURCE_MIN_X + round((p.x - SOURCE_MIN_X) / 300.0) * 300.0
        p.y = -400.0 + round((p.y + 400.0) / 300.0) * 300.0
        p.z = 20.0
    if name == 'Cube' and actor.get_actor_bounds(False)[1].x > 500.0:
        p.x = SOURCE_MIN_X + PITCH * 0.5
        scale = t.scale3d
        scale.x = PITCH / 100.0
        t.scale3d = scale
    p.x -= SOURCE_MIN_X
    t.translation = p
    groups[key]['transforms'].append(t)

extension = actors_subsystem.spawn_actor_from_class(unreal.ShowcaseRepeatExtension, unreal.Vector(first_x, 0, 0))
extension.set_actor_label('Showcase1_EndlessExtension_196_Cells')
extension.set_folder_path('EndlessExtension/InstancedArchitecture')
extension.set_editor_property('tags', [unreal.Name('Showcase1EndlessExtension')])
extension.set_editor_property('repeat_count', REPEATS)
extension.set_editor_property('repeat_spacing', PITCH)
mesh_groups = []
for data in groups.values():
    group = unreal.ShowcaseRepeatMeshGroup()
    group.set_editor_property('mesh', data['mesh'])
    group.set_editor_property('materials', data['materials'])
    group.set_editor_property('source_transforms', data['transforms'])
    group.set_editor_property('collision_enabled', data['collision'])
    group.set_editor_property('cast_shadow', data['shadow'])
    group.set_editor_property('detail_cull_distance', data['distance'])
    mesh_groups.append(group)
extension.set_editor_property('mesh_groups', mesh_groups)
extension.rebuild_extension()
expected = len(static_sources) * REPEATS
if extension.get_repeated_instance_count() != expected:
    raise RuntimeError('Unexpected generated mesh count')

created_blueprints = []
fixture_actors = []
for index in range(REPEATS):
    offset = unreal.Vector(first_x - SOURCE_MIN_X + index * PITCH, 0, 0)
    for source in blueprint_sources:
        duplicate = extension.duplicate_fixture(source, offset)
        if not duplicate:
            raise RuntimeError('Failed to duplicate ' + source.get_actor_label())
        kind = 'Lamp' if 'Ceilinglamp' in source.get_class().get_name() else 'Cable'
        duplicate.set_actor_label('Endless_%s_%03d' % (kind, index + 5))
        duplicate.set_folder_path('EndlessExtension/Fixtures/%03d' % (index + 5))
        duplicate.set_editor_property('tags', [unreal.Name('Showcase1EndlessExtension')])
        if kind == 'Cable':
            for component in duplicate.get_components_by_class(unreal.StaticMeshComponent):
                component.set_cull_distance(7000.0)
                component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
                component.set_editor_property('generate_overlap_events', False)
        for component in duplicate.get_components_by_class(unreal.LightComponent):
            component.set_editor_property('max_draw_distance', 7000.0)
            component.set_editor_property('max_distance_fade_range', 2000.0)
        created_blueprints.append(duplicate.get_actor_label())
        fixture_actors.append(duplicate)
    if index % 25 == 0:
        unreal.log_warning('SHOWCASE_EXTENSION_PROGRESS %d/%d' % (index + 1, REPEATS))

extension.set_editor_property('repeated_fixtures', fixture_actors)
extension.apply_fixture_distance_limits()

endcap_changes = []
for label in END_CAPS:
    cap = by_label[label]
    p = cap.get_actor_location()
    before = [p.x, p.y, p.z]
    p.x += REPEATS * PITCH
    cap.set_actor_location(p, False, False)
    endcap_changes.append({'actor': label, 'before': before, 'after': [p.x, p.y, p.z]})

if not level_subsystem.save_current_level():
    raise RuntimeError('Could not save expanded Showcase1')

result = {'map': MAP, 'original_actors': len(actors), 'original_cells': 4, 'additional_cells': REPEATS,
          'cell_pitch_cm': PITCH, 'extension_start_x_cm': first_x, 'extension_end_x_cm': first_x + REPEATS * PITCH,
          'added_length_m': REPEATS * PITCH / 100, 'mesh_instances': expected, 'hism_components': len(mesh_groups),
          'fixture_blueprints': len(created_blueprints), 'moved_endcaps': endcap_changes,
          'details_cull_cm': [3000, 4000, 6000, 7000, 10000, 18000], 'light_cull_cm': 7000,
          'note': 'Finite 200-cell extension. Existing entrance, original four cells, post process and fog are unchanged.'}
OUTPUT.mkdir(parents=True, exist_ok=True)
with (OUTPUT / 'result.json').open('w', encoding='utf-8') as handle:
    json.dump(result, handle, ensure_ascii=False, indent=2)
unreal.log_warning('SHOWCASE_EXTENSION_SAVED ' + json.dumps(result))
