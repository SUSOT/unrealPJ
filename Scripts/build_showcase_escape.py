"""Install the turn-back escape on the current circular Showcase1, once only.

Only this level and new Escape materials are saved. The original repeat props
are extracted from HISM into 32 movable actors, not copied on top of instances.
"""
import json
import math
from pathlib import Path
import unreal as u

MAP = '/Game/Developers/MOON/Level/Showcase1'
ROOT = '/Game/Developers/MOON/Materials/Escape'
out = Path(u.Paths.project_saved_dir()) / 'ShowcaseExpansion'
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
api = u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level(MAP)
actors = list(api.get_all_level_actors())
assert not any(isinstance(a, u.ShowcaseLoopDirector) for a in actors), 'Escape already installed; refusing to duplicate it.'
ext = next(a for a in actors if isinstance(a, u.ShowcaseRepeatExtension))
assert ext.get_repeated_instance_count() == 2368
radius = 3055.7749073643904

def pos(s, y=165, z=20):
    angle = s/radius
    r = radius-(y-50)
    return u.Vector(r*math.sin(angle), radius-r*math.cos(angle)+50, z)

def material(name, color, emissive=False):
    path = ROOT + '/' + name
    # These task-owned materials may already exist after a failed unsaved run.
    if u.EditorAssetLibrary.does_asset_exist(path):
        return u.load_asset(path)
    m = u.AssetToolsHelpers.get_asset_tools().create_asset(name, ROOT, u.Material, u.MaterialFactoryNew())
    assert m
    mel = u.MaterialEditingLibrary
    c = mel.create_material_expression(m, u.MaterialExpressionConstant3Vector, -300, 0)
    c.set_editor_property('constant', u.LinearColor(*color, 1.0))
    if emissive:
        m.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
        mel.connect_material_property(c, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        mel.connect_material_property(c, '', u.MaterialProperty.MP_BASE_COLOR)
        rough = mel.create_material_expression(m, u.MaterialExpressionConstant, -300, 160)
        rough.set_editor_property('r', 0.85)
        mel.connect_material_property(rough, '', u.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    assert u.EditorAssetLibrary.save_loaded_asset(m)
    return m

frame_mat = material('M_Escape_Frame', (.055,.065,.06))
leaf_mat = material('M_Escape_Door', (.38,.43,.40))
glow_mat = material('M_Escape_Threshold', (.26,.32,.34), True)
room_mat = material('M_Escape_Vestibule', (.43,.47,.46))

director = api.spawn_actor_from_class(u.ShowcaseLoopDirector, u.Vector())
director.set_actor_label('Showcase1_TurnBackEscape')
director.set_folder_path('CircularLoop/Escape/System')
changes = []
extracted = []
groups = list(ext.get_editor_property('mesh_groups'))
for group in groups:
    mesh = group.get_editor_property('mesh')
    name = mesh.get_name()
    if name not in ('SM_Chair', 'SM_WallPainting_1'):
        continue
    transforms = list(group.get_editor_property('source_transforms'))
    # Repetition is grouped by original prop then cell; first 16 are one per room.
    chosen = transforms[:16]
    assert len(chosen) == 16
    group.set_editor_property('source_transforms', transforms[16:])
    for index, t in enumerate(chosen):
        a = api.spawn_actor_from_class(u.StaticMeshActor, t.translation)
        a.set_actor_label('Loop_Change_%s_%02d' % (name, index+1))
        a.set_folder_path('CircularLoop/Escape/SpatialChanges')
        c = a.static_mesh_component
        c.set_mobility(u.ComponentMobility.MOVABLE)
        c.set_static_mesh(mesh)
        for slot, mat in enumerate(group.get_editor_property('materials')): c.set_material(slot, mat)
        c.set_collision_profile_name('BlockAll' if group.get_editor_property('collision_enabled') else 'NoCollision')
        c.set_editor_property('generate_overlap_events', False)
        a.set_actor_transform(t, False, True)
        extracted.append(a)
        changed = u.Transform(location=t.translation, scale=t.scale3d)
        # Wall artwork rolls in its own plane (normal is local Y), not off the wall.
        delta = u.Rotator(yaw=22 if index % 2 else -22) if name == 'SM_Chair' else u.Rotator(pitch=90 if index % 2 else -90)
        changed.rotation = t.rotation * delta.quaternion()
        change = u.ShowcaseSpatialChange()
        change.set_editor_property('target', a)
        change.set_editor_property('changed_transform', changed)
        change.set_editor_property('stage', 1 if name == 'SM_Chair' else 2)
        changes.append(change)
        if name == 'SM_Chair':
            final = u.Transform(location=t.translation, scale=t.scale3d)
            final.rotation = t.rotation * u.Rotator(yaw=160 if index%2 else -160).quaternion()
            change = u.ShowcaseSpatialChange()
            change.set_editor_property('target', a)
            change.set_editor_property('changed_transform', final)
            change.set_editor_property('stage', 3)
            changes.append(change)
assert len(extracted) == 32 and len(changes) == 48
ext.set_editor_property('mesh_groups', groups)
ext.rebuild_extension()
assert ext.get_repeated_instance_count() == 2336
director.set_editor_property('spatial_changes', changes)
director.set_editor_property('lamps', sorted([a for a in actors if a.get_actor_label().startswith('Loop_Lamp_')], key=lambda a:a.get_actor_label()))

signs = []
for index in range(8):
    s = index*2400+900
    # TextRender text runs along local -Y. On the outer wall, > is therefore
    # the positive loop tangent, matching the initial walking direction.
    a = api.spawn_actor_from_class(u.TextRenderActor, pos(s,-365,315), u.Rotator(yaw=math.degrees(s/radius)+90))
    a.set_actor_label('Loop_ExitDirection_%02d' % (index+1))
    a.set_folder_path('CircularLoop/Escape/Wayfinding')
    c = a.get_component_by_class(u.TextRenderComponent)
    c.set_mobility(u.ComponentMobility.MOVABLE)
    c.set_text('EXIT  >')
    c.set_world_size(23)
    c.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER)
    c.set_text_render_color(u.Color(182,216,202,255))
    signs.append(a)
director.set_editor_property('direction_signs', signs)

# Self-contained destination: no assumptions about the project's next level.
destination = api.spawn_actor_from_class(u.TargetPoint, u.Vector(0,-9000,112))
destination.set_actor_label('Showcase1_EscapeDestination')
destination.set_folder_path('CircularLoop/Escape/Destination')
cube = u.load_asset('/Engine/BasicShapes/Cube')
room_parts = []
def box(name, location, size, mat):
    a = api.spawn_actor_from_class(u.StaticMeshActor, u.Vector(*location))
    a.set_actor_label(name)
    a.set_folder_path('CircularLoop/Escape/Destination')
    c = a.static_mesh_component
    c.set_static_mesh(cube)
    c.set_material(0, mat)
    a.set_actor_scale3d(u.Vector(*(v/100 for v in size)))
    c.set_collision_profile_name('BlockAll')
    room_parts.append(a)
    return a
box('Escape_Floor',(350,-9000,0),(1800,840,40),room_mat)
box('Escape_Ceiling',(350,-9000,410),(1800,840,40),room_mat)
box('Escape_LeftWall',(350,-9430,205),(1800,40,410),room_mat)
box('Escape_RightWall',(350,-8570,205),(1800,40,410),room_mat)
box('Escape_BackWall',(-570,-9000,205),(40,900,410),frame_mat)
box('Escape_FarWall',(1270,-9000,205),(40,900,410),room_mat)
panel = box('Escape_Window',(1246,-9000,215),(3,400,240),glow_mat)
panel.static_mesh_component.set_cast_shadow(False)
for index,x in enumerate((-150,450,1050)):
    a = api.spawn_actor_from_class(u.PointLight, u.Vector(x,-9000,320))
    a.set_actor_label('Escape_SoftLight_%02d'%index)
    a.set_folder_path('CircularLoop/Escape/Destination')
    c = a.get_component_by_class(u.PointLightComponent)
    c.set_mobility(u.ComponentMobility.MOVABLE)
    c.set_intensity(1.5)
    c.set_attenuation_radius(850)
    c.set_light_color(u.LinearColor(.82,.89,1.0,1.0))
    c.set_cast_shadows(False)

door = api.spawn_actor_from_class(u.ShowcaseEscapeDoor, pos(1500), u.Rotator(yaw=math.degrees(1500/radius)))
door.set_actor_label('Showcase1_ReversalExitDoor')
door.set_folder_path('CircularLoop/Escape/System')
for key,value in [('destination',destination),('frame_material',frame_mat),('leaf_material',leaf_mat),('threshold_material',glow_mat)]:
    door.set_editor_property(key,value)
# Keep the editor walkthrough uncluttered too; RevealAt unhides it during play.
door.set_actor_hidden_in_game(True)
door.set_actor_enable_collision(False)
door.set_is_temporarily_hidden_in_editor(True)
director.set_editor_property('escape_door',door)
assert levels.save_current_level()
report = {'map': MAP, 'hism_instances':ext.get_repeated_instance_count(), 'movable_props':len(extracted),
          'spatial_changes':len(changes),'signs':len(signs),'stages_m':[60,130,205],
          'backtrack_m':5,'door_ahead_m':18,'destination':[0,-9000,112],
          'materials':[m.get_path_name() for m in (frame_mat,leaf_mat,glow_mat,room_mat)]}
(out/'escape_install.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log_warning('SHOWCASE_ESCAPE_INSTALLED '+json.dumps(report))
