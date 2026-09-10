"""Install compact emergency signage and the requested random light/prop events.

Run this once against the straight Showcase1 baseline. The updater only changes
task-owned map actors/assets and leaves the route and player untouched.
"""
import json
from pathlib import Path
import unreal as u

MAP = '/Game/Developers/MOON/Level/Showcase1'
ROOT = '/Game/Developers/MOON/Materials/HorrorVisual'
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors_api = u.get_editor_subsystem(u.EditorActorSubsystem)
asset_tools = u.AssetToolsHelpers.get_asset_tools()
mel = u.MaterialEditingLibrary
assert levels.load_level(MAP)

actors = list(actors_api.get_all_level_actors())
director = next(a for a in actors if isinstance(a, u.ShowcaseLoopDirector))
extension = next(a for a in actors if isinstance(a, u.ShowcaseRepeatExtension))
assert director.get_editor_property('straight_corridor')
assert extension.get_editor_property('infinite_straight')


def emergency_exit_material():
    texture_path = ROOT + '/T_EmergencyExit_RunMan'
    image = Path(u.Paths.project_dir()) / 'SourceArt' / 'ShowcaseHorror' / 'EmergencyExit_RunMan.png'
    assert image.exists(), image
    if not u.EditorAssetLibrary.does_asset_exist(texture_path):
        task = u.AssetImportTask()
        task.set_editor_property('filename', str(image))
        task.set_editor_property('destination_path', ROOT)
        task.set_editor_property('destination_name', 'T_EmergencyExit_RunMan')
        task.set_editor_property('automated', True)
        task.set_editor_property('replace_existing', False)
        task.set_editor_property('save', True)
        asset_tools.import_asset_tasks([task])
    texture = u.load_asset(texture_path)
    assert isinstance(texture, u.Texture2D)
    texture.set_editor_property('address_x', u.TextureAddress.TA_CLAMP)
    texture.set_editor_property('address_y', u.TextureAddress.TA_CLAMP)
    assert u.EditorAssetLibrary.save_loaded_asset(texture)

    material_path = ROOT + '/M_EmergencyExit_RunMan'
    if u.EditorAssetLibrary.does_asset_exist(material_path):
        return u.load_asset(material_path)
    mat = asset_tools.create_asset('M_EmergencyExit_RunMan', ROOT, u.Material, u.MaterialFactoryNew())
    assert mat
    mat.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    texcoord = mel.create_material_expression(mat, u.MaterialExpressionTextureCoordinate, -800, 0)
    mirror = mel.create_material_expression(mat, u.MaterialExpressionConstant2Vector, -800, 150)
    mirror.set_editor_property('r', -1.0)
    mirror.set_editor_property('g', 1.0)
    multiply_uv = mel.create_material_expression(mat, u.MaterialExpressionMultiply, -600, 0)
    offset = mel.create_material_expression(mat, u.MaterialExpressionConstant2Vector, -600, 150)
    offset.set_editor_property('r', 1.0)
    offset.set_editor_property('g', 0.0)
    add_uv = mel.create_material_expression(mat, u.MaterialExpressionAdd, -400, 0)
    sample = mel.create_material_expression(mat, u.MaterialExpressionTextureSample, -200, 0)
    sample.set_editor_property('texture', texture)
    intensity = mel.create_material_expression(mat, u.MaterialExpressionConstant, -200, 180)
    intensity.set_editor_property('r', .12)
    emissive = mel.create_material_expression(mat, u.MaterialExpressionMultiply, 0, 0)
    mel.connect_material_expressions(texcoord, '', multiply_uv, 'A')
    mel.connect_material_expressions(mirror, '', multiply_uv, 'B')
    mel.connect_material_expressions(multiply_uv, '', add_uv, 'A')
    mel.connect_material_expressions(offset, '', add_uv, 'B')
    mel.connect_material_expressions(add_uv, '', sample, 'UVs')
    mel.connect_material_expressions(sample, 'RGB', emissive, 'A')
    mel.connect_material_expressions(intensity, '', emissive, 'B')
    mel.connect_material_property(emissive, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    assert u.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


exit_material = emergency_exit_material()
cube = u.load_asset('/Engine/BasicShapes/Cube')
assert cube

# Replace the bare text actors with eight compact image signs at 36 m intervals. They are
# ordinary non-ticking fixtures so the corridor recycler preserves the spacing.
old_signs = [a for a in actors if a.get_actor_label().startswith('Straight_ExitDirection_')]
installed_signs = [a for a in actors if a.get_actor_label().startswith('Straight_EmergencyExitImage_')]
assert len(old_signs) == 12 and not installed_signs, 'Random visual-event pass is already installed.'
fixtures = [a for a in extension.get_editor_property('repeated_fixtures') if u.SystemLibrary.is_valid(a) and a not in old_signs]
for old in old_signs:
    assert actors_api.destroy_actor(old)
extension.set_editor_property('repeated_fixtures', fixtures)

groups = list(extension.get_editor_property('mesh_groups'))
assert not any(any(m and '/Materials/HorrorVisual/M_EmergencyExit_' in m.get_path_name() for m in g.get_editor_property('materials')) for g in groups)
extension.rebuild_extension()

signs = []
for cell in (-12, -9, -6, -3, 0, 3, 6, 9):
    sign = actors_api.spawn_actor_from_class(u.StaticMeshActor, u.Vector(cell * 1200 + 900, -367, 315), u.Rotator())
    assert sign
    sign.set_actor_label('Straight_EmergencyExitImage_' + str(cell))
    sign.set_folder_path('StraightShowcase/Wayfinding/EmergencyExit')
    sign.set_actor_scale3d(u.Vector(.58, .025, .58))
    mesh = sign.static_mesh_component
    mesh.set_mobility(u.ComponentMobility.MOVABLE)
    mesh.set_static_mesh(cube)
    mesh.set_material(0, exit_material)
    mesh.set_collision_profile_name('NoCollision')
    mesh.set_editor_property('generate_overlap_events', False)
    mesh.set_editor_property('cast_shadow', False)
    mesh.set_cull_distance(12000)
    signs.append(sign)
fixtures.extend(signs)
extension.set_editor_property('repeated_fixtures', fixtures)
extension.apply_fixture_distance_limits()

# The only horror systems installed here are the requested shuffled light and
# prop events. No global grade, figure, follower audio, or continuous warp.
current = list(actors_api.get_all_level_actors())
for actor in current:
    if actor.get_actor_label().startswith('Showcase_Dread') or actor.get_actor_label() == 'Loop_Reverse_TableLight':
        assert actors_api.destroy_actor(actor)
lamps = sorted((a for a in current if a.get_actor_label().startswith('Straight_Lamp_')),
               key=lambda a: a.get_actor_location().x)
chairs = sorted((a for a in current if a.get_actor_label().startswith('Straight_Change_SM_Chair_')),
                key=lambda a: a.get_actor_location().x)
frames = sorted((a for a in current if a.get_actor_label().startswith('Straight_Change_SM_WallPainting_1_')),
                key=lambda a: a.get_actor_location().x)
table_mesh = u.load_asset('/Game/RestaurantScene/Meshes/SM_Table')
assert len(lamps) == 24 and len(chairs) == 24 and len(frames) == 24 and table_mesh
director.set_editor_property('repeat_extension', extension)
director.set_editor_property('table_mesh', table_mesh)
director.set_editor_property('lamps', lamps)
director.set_editor_property('chair_props', chairs)
director.set_editor_property('frame_props', frames)
director.set_editor_property('emergency_exit_signs', signs)
director.set_editor_property('enable_horror_events', True)
director.set_editor_property('base_light_scale', .72)
director.set_editor_property('horror_seed', 91357)

assert len(fixtures) == 104
assert extension.get_repeated_instance_count() == 4008
assert levels.save_current_level()

out = Path(u.Paths.project_saved_dir()) / 'ShowcaseExpansion'
(out / 'random_visual_events_install.json').write_text(json.dumps({
    'emergency_exit_signs': 8,
    'emergency_exit_spacing_cm': 3600,
    'emergency_exit_texture': ROOT + '/T_EmergencyExit_RunMan',
    'sign_direction': '+X initially; mirrored to -X when the turn-back door appears',
    'repeated_fixtures': len(fixtures),
    'fixed_hism_instances': extension.get_repeated_instance_count(),
    'base_light_scale': .72,
    'random_event_types': 5,
    'chairs': len(chairs),
    'frames': len(frames),
    'table_mesh': table_mesh.get_path_name(),
}, indent=2), encoding='utf-8')
u.log_warning('SHOWCASE_RANDOM_VISUAL_EVENTS_SAVED')
u.SystemLibrary.quit_editor()
