"""Replace the long diner with a compact, physically closed 16-room circular diner.

Run with GeometryScripting, PythonScriptPlugin and EditorScriptingUtilities enabled.
Only Showcase1 and newly authored shell assets are saved. Existing asset materials
and the world's post process/fog settings are preserved.
"""
import json
import math
from pathlib import Path
import unreal as u

MAP = '/Game/Developers/MOON/Level/Showcase1'
ASSETS = '/Game/Developers/MOON/Meshes/ShowcaseLoop'
COUNT, PITCH, SOURCE_X, MID_Y = 16, 1200.0, 1980.0, 50.0
RADIUS = COUNT * PITCH / (2 * math.pi)
OUT = Path(u.Paths.project_saved_dir()) / 'ShowcaseExpansion'
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors_api = u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level(MAP)
actors = list(actors_api.get_all_level_actors())
by_label = {a.get_actor_label(): a for a in actors}
extension = next(a for a in actors if isinstance(a, u.ShowcaseRepeatExtension))
assert extension.get_repeated_instance_count() == 33320, 'Unexpected map state; refusing to replace layout.'
template = [a for a in actors if str(a.get_folder_path()) == '3']
sources = [a for a in template if isinstance(a, u.StaticMeshActor)]
fixtures = [a for a in template if 'Ceilinglamp' in a.get_class().get_name() or 'BP_Cablespline' in a.get_class().get_name()]
assert len(sources) == 170 and len(fixtures) == 2

def position(s, y, z):
    angle = s / RADIUS
    radius = RADIUS - (y - MID_Y)
    return u.Vector(radius * math.sin(angle), RADIUS - radius * math.cos(angle) + MID_Y, z)

def bent_transform(transform, cell=0):
    p = transform.translation
    s = p.x - SOURCE_X + cell * PITCH
    q = u.Rotator(yaw=math.degrees(s / RADIUS)).quaternion()
    result = u.Transform(location=position(s, p.y, p.z), scale=transform.scale3d)
    result.rotation = q * transform.rotation
    return result

def copy_mesh(mesh):
    dynamic = u.DynamicMesh()
    result = u.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(mesh, dynamic,
        u.GeometryScriptCopyMeshFromAssetOptions(), u.GeometryScriptMeshReadLOD())
    assert result[-1] == u.GeometryScriptOutcomePins.SUCCESS, str(result)
    return dynamic

def bend_mesh(dynamic):
    result = u.GeometryScript_MeshQueries.get_all_vertex_positions(dynamic, False)
    positions = u.GeometryScript_List.convert_vector_list_to_array(result[1])
    warped = [position(p.x, p.y, p.z) for p in positions]
    vector_list = u.GeometryScript_List.convert_array_to_vector_list(warped)
    u.GeometryScript_MeshEdits.set_all_mesh_vertex_positions(dynamic, vector_list)
    u.GeometryScript_Normals.recompute_normals(dynamic, u.GeometryScriptCalculateNormalsOptions())

def append(target, source, transform):
    u.GeometryScript_MeshEdits.append_mesh(target, source, transform)

def create_asset(name, dynamic, materials):
    path = ASSETS + '/' + name
    # These three task-owned meshes can be rebuilt while iterating the loop.
    if u.EditorAssetLibrary.does_asset_exist(path):
        mesh = u.load_asset(path)
        copy_options = u.GeometryScriptCopyMeshToAssetOptions()
        copy_options.set_editor_property('enable_recompute_tangents', True)
        copy_options.set_editor_property('replace_materials', True)
        copy_options.set_editor_property('new_materials', materials)
        result = u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(dynamic, mesh, copy_options, u.GeometryScriptMeshWriteLOD())
        assert result[-1] == u.GeometryScriptOutcomePins.SUCCESS, str(result)
        assert u.EditorAssetLibrary.save_loaded_asset(mesh)
        return mesh
    options = u.GeometryScriptCreateNewStaticMeshAssetOptions()
    options.set_editor_property('enable_collision', True)
    options.set_editor_property('collision_mode', u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    options.set_editor_property('enable_recompute_tangents', True)
    result = u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(dynamic, path, options)
    mesh = result[0]
    assert mesh and result[-1] == u.GeometryScriptOutcomePins.SUCCESS, str(result)
    for index, material in enumerate(materials):
        mesh.set_material(index, material)
    assert u.EditorAssetLibrary.save_loaded_asset(mesh)
    return mesh

# Generate a continuous curved floor using the original 3 m floor tile UVs.
floor_source = next(a for a in sources if a.static_mesh_component.static_mesh.get_name() == 'SM_Floor')
floor_piece = copy_mesh(floor_source.static_mesh_component.static_mesh)
floor = u.DynamicMesh()
for x in (0, 300, 600, 900):
    for y in (-400, -100, 200):
        append(floor, floor_piece, u.Transform(location=u.Vector(x, y, 20)))
u.GeometryScript_MeshSubdivide.apply_uniform_tessellation(floor, 5)
bend_mesh(floor)
floor_asset = create_asset('SM_ShowcaseLoop_Floor', floor, list(floor_source.static_mesh_component.get_materials()))

# Preserve the original wall mesh, trim, material slots, and UVs. Normalize its
# segment start positions before bending so the last segment meets the first.
wall_source = next(a for a in sources if a.static_mesh_component.static_mesh.get_name() == 'SM_Wall01')
wall_piece = copy_mesh(wall_source.static_mesh_component.static_mesh)
walls = u.DynamicMesh()
for x in (0, 300, 600, 900):
    append(walls, wall_piece, u.Transform(location=u.Vector(x, -400, 20)))
    append(walls, wall_piece, u.Transform(location=u.Vector(x + 300, 500, 20), rotation=u.Rotator(yaw=180)))
u.GeometryScript_MeshSubdivide.apply_uniform_tessellation(walls, 5)
bend_mesh(walls)
wall_asset = create_asset('SM_ShowcaseLoop_Walls', walls, list(wall_source.static_mesh_component.get_materials()))

# Solid backing prevents light leaks without a hidden straight corridor below it.
backing = u.DynamicMesh()
primitive_options = u.GeometryScriptPrimitiveOptions()
for y, z, width, height in ((50, -1, 960, 40), (50, 410, 960, 25), (-425, 210, 30, 450), (525, 210, 30, 450)):
    u.GeometryScript_Primitives.append_box(backing, primitive_options,
        u.Transform(location=u.Vector(600, y, z)), 1200, width, height, 32, 1, 1,
        u.GeometryScriptPrimitiveOriginMode.CENTER)
bend_mesh(backing)
shell_asset = create_asset('SM_ShowcaseLoop_SolidShell', backing,
    [u.load_asset('/Engine/BasicShapes/BasicShapeMaterial')])

# Reuse the serialized HISM owner. Source transforms now contain the complete
# ring, so RepeatCount is one and no additional runtime construction is needed.
groups = {}
def add(mesh, materials, transform, collision=True, shadow=True, cull=0):
    key = (mesh.get_path_name(), tuple(m.get_path_name() if m else '' for m in materials), collision, shadow, cull)
    groups.setdefault(key, {'mesh': mesh, 'materials': materials, 'transforms': [], 'collision': collision, 'shadow': shadow, 'cull': cull})['transforms'].append(transform)

center = u.Vector(0, RADIUS + MID_Y, 0)
for cell in range(COUNT):
    q = u.Rotator(yaw=360.0 * cell / COUNT).quaternion()
    # All curved shell assets share the first cell's world-space coordinates.
    shift = center - q.rotate_vector(center)
    t = u.Transform(location=shift, rotation=u.Rotator(yaw=360.0 * cell / COUNT))
    for mesh in (floor_asset, wall_asset, shell_asset):
        add(mesh, list(mesh.get_materials()) if hasattr(mesh, 'get_materials') else
            [s.material_interface for s in mesh.get_editor_property('static_materials')], t)

seen = set()
prop_sources = []
for actor in sources:
    component = actor.static_mesh_component
    mesh = component.static_mesh
    name = mesh.get_name()
    if name in ('SM_Floor', 'SM_Wall01', 'Cube'):
        continue
    t = component.get_world_transform()
    p, q, scale = t.translation, t.rotation, t.scale3d
    key = (mesh.get_path_name(), tuple(m.get_path_name() if m else '' for m in component.get_materials()),
           tuple(round(v, 1) for v in (p.x, p.y, p.z)),
           tuple(round(v, 4) for v in (q.x, q.y, q.z, q.w, scale.x, scale.y, scale.z)))
    if key in seen:
        continue
    seen.add(key)
    prop_sources.append(actor)
    tiny = name.startswith('SM_Shaker') or name == 'SM_NapkinDispenser'
    decoration = tiny or name == 'SM_Decals' or 'Cables' in name or 'Pipes_Support' in name
    collision = not decoration and component.get_collision_enabled() != u.CollisionEnabled.NO_COLLISION
    cull = 3000 if tiny else 5000 if decoration else 8000 if 'WallPainting' in name or 'Chalkboard' in name else 0
    for cell in range(COUNT):
        add(mesh, list(component.get_materials()), bent_transform(t, cell), collision,
            bool(component.get_editor_property('cast_shadow')) and not tiny, cull)

new_fixtures = []
for cell in range(COUNT):
    for source in fixtures:
        duplicate = extension.duplicate_fixture(source, u.Vector())
        assert duplicate
        duplicate.set_actor_transform(bent_transform(source.get_actor_transform(), cell), False, True)
        kind = 'Lamp' if 'Ceilinglamp' in source.get_class().get_name() else 'Cable'
        duplicate.set_actor_label('Loop_%s_%02d' % (kind, cell + 1))
        duplicate.set_folder_path('CircularLoop/Fixtures/%02d' % (cell + 1))
        duplicate.set_editor_property('tags', [u.Name('ShowcaseCircularLoop')])
        new_fixtures.append(duplicate)

mesh_groups = []
for data in groups.values():
    group = u.ShowcaseRepeatMeshGroup()
    for key, value in [('mesh', data['mesh']), ('materials', data['materials']), ('source_transforms', data['transforms']),
                       ('collision_enabled', data['collision']), ('cast_shadow', data['shadow']), ('detail_cull_distance', data['cull'])]:
        group.set_editor_property(key, value)
    mesh_groups.append(group)
extension.set_actor_transform(u.Transform(), False, True)
extension.set_editor_property('repeat_count', 1)
extension.set_editor_property('mesh_groups', mesh_groups)
extension.set_editor_property('repeated_fixtures', new_fixtures)
extension.set_actor_label('Showcase1_CircularLoop_16_Rooms')
extension.set_folder_path('CircularLoop/InstancedArchitecture')
extension.set_editor_property('tags', [u.Name('ShowcaseCircularLoop')])
extension.rebuild_extension()
extension.apply_fixture_distance_limits()

# Explicitly limit removal to known diner structure and previous extension.
# World settings, post process, fog and any unrecognized actors are retained.
before_labels = {a['label'] for a in json.loads((OUT / 'before_loop.json').read_text(encoding='utf-8'))['actors']}
removed = []
for actor in actors:
    cls = actor.get_class().get_name()
    is_diner = isinstance(actor, u.StaticMeshActor) or 'Ceilinglamp' in cls or 'BP_Cablespline' in cls or isinstance(actor, u.LightmassImportanceVolume)
    if is_diner and actor.get_actor_label() in before_labels:
        removed.append(actor.get_actor_label())
        assert actors_api.destroy_actor(actor)
player_start = next(a for a in actors if isinstance(a, u.PlayerStart))
player_start.set_actor_location(position(220, 150, 112), False, True)
player_start.set_actor_rotation(u.Rotator(yaw=math.degrees(220 / RADIUS)), True)
assert levels.save_current_level()

report = {'map': MAP, 'cells': COUNT, 'circumference_m': COUNT * PITCH / 100,
    'radius_cm': RADIUS, 'center': [0, RADIUS + MID_Y, 0], 'corridor_width_m': 9,
    'footprint_diameter_m': (2 * RADIUS + 1080) / 100,
    'instances': extension.get_repeated_instance_count(), 'hism_groups': len(mesh_groups),
    'source_props_after_deduplication': len(prop_sources), 'fixtures': len(new_fixtures),
    'removed_previous_layout_actors': removed, 'new_assets': [m.get_path_name() for m in (floor_asset, wall_asset, shell_asset)]}
(OUT / 'loop_result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log_warning('SHOWCASE_LOOP_SAVED ' + json.dumps({k:v for k,v in report.items() if k != 'removed_previous_layout_actors'}))
