"""Install the one-shot reverse-table encounter. Run once after a map backup."""
from pathlib import Path
import math
import unreal as u
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
api=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
actors=list(api.get_all_level_actors())
d=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector))
ext=next(a for a in actors if isinstance(a,u.ShowcaseRepeatExtension))
assert not d.get_editor_property('reverse_chair'),'Reverse encounter already installed; do not overwrite user edits.'
chair=next(a for a in actors if a.get_actor_label()=='Loop_Change_SM_Chair_16')
original=chair.get_actor_transform()
center=d.get_editor_property('loop_center'); r=d.get_editor_property('loop_radius')
offset=original.translation-center
angle=math.atan2(offset.x,-offset.y)
tangent=u.Vector(math.cos(angle),math.sin(angle),0)
pulled=u.Transform(location=original.translation-tangent*65,scale=original.scale3d)
pulled.rotation=original.rotation
chair.set_actor_transform(pulled,False,True)
chair.set_folder_path('CircularLoop/Escape/ReverseEncounter')
changes=[c for c in d.get_editor_property('spatial_changes') if c.get_editor_property('target')!=chair]
assert len(changes)==31
d.set_editor_property('spatial_changes',changes)
d.set_editor_property('reverse_chair',chair)
d.set_editor_property('reverse_chair_tucked_pose',original)

# Reuse the existing central table and the empty chair opposite it.
table_candidates=[]; seat_candidates=[]
for g in ext.get_editor_property('mesh_groups'):
    mesh=g.get_editor_property('mesh')
    if mesh.get_name() not in ('SM_Table','SM_Chair'): continue
    for t in g.get_editor_property('source_transforms'):
        target=table_candidates if mesh.get_name()=='SM_Table' else seat_candidates
        target.append(((t.translation-original.translation).length(),t,mesh))
_,table,table_mesh=min(table_candidates,key=lambda row:row[0])
_,opposite,_=min(seat_candidates,key=lambda row:row[0])
d.set_editor_property('reverse_return_seat_location',opposite.translation+u.Vector(0,0,78))
bounds=table_mesh.get_bounds()
top=table.translation.z+(bounds.origin.z+bounds.box_extent.z)*table.scale3d.z

for name,mesh_name,x,y,yaw in [('Plate','SM_Plate',0,-7,0),('Mug','SM_CoffeMug',25,16,20),('Fork','SM_Fork',-24,-9,0),('Knife','SM_Knife',22,-8,0)]:
    mesh=u.load_asset('/Game/RestaurantScene/Meshes/'+mesh_name)
    assert mesh,mesh_name
    bounds=mesh.get_bounds()
    location=table.transform_location(u.Vector(x,y,0))
    location.z=top+.35-(bounds.origin.z-bounds.box_extent.z)
    actor=api.spawn_actor_from_class(u.StaticMeshActor,location)
    actor.set_actor_label('Loop_Reverse_'+name)
    actor.set_folder_path('CircularLoop/Escape/ReverseEncounter')
    c=actor.static_mesh_component
    c.set_static_mesh(mesh)
    c.set_mobility(u.ComponentMobility.MOVABLE)
    c.set_collision_profile_name('NoCollision')
    c.set_editor_property('generate_overlap_events',False)
    t=u.Transform(location=location)
    t.rotation=table.rotation*u.Rotator(yaw=yaw).quaternion()
    actor.set_actor_transform(t,False,True)

lamp=api.spawn_actor_from_class(u.SpotLight,table.translation+u.Vector(0,0,330))
lamp.set_actor_label('Loop_Reverse_TableLight')
lamp.set_folder_path('CircularLoop/Escape/ReverseEncounter')
lamp.set_actor_rotation(u.Rotator(pitch=-90),True)
light=lamp.get_component_by_class(u.SpotLightComponent)
light.set_mobility(u.ComponentMobility.MOVABLE)
light.set_editor_property('intensity_units',u.LightUnits.LUMENS)
light.set_intensity(320.0)
light.set_attenuation_radius(650)
light.set_inner_cone_angle(28)
light.set_outer_cone_angle(42)
light.set_editor_property('use_temperature',True)
light.set_temperature(3200)
light.set_cast_shadows(False)
d.set_editor_property('reverse_focus_light',lamp)

task=u.AssetImportTask()
for key,value in [('filename',str(Path(u.Paths.project_dir())/'SourceArt/ShowcaseHorror/Audio/SW_Loop_Cutlery.wav')),('destination_path','/Game/Developers/MOON/Audio/Horror'),('destination_name','SW_Loop_Cutlery'),('automated',True),('replace_existing',False),('save',True)]: task.set_editor_property(key,value)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound=u.load_asset('/Game/Developers/MOON/Audio/Horror/SW_Loop_Cutlery')
assert isinstance(sound,u.SoundWave)
sound.set_sound_asset_compression_type(u.SoundAssetCompressionType.PCM)
u.EditorAssetLibrary.save_loaded_asset(sound)
d.set_editor_property('reverse_cutlery_sound',sound)
assert levels.save_current_level()
u.log_warning('SHOWCASE_REVERSE_SAVED: 1 existing chair repurposed, 4 table props, 1 local spotlight. No blockers or damage added.')
u.SystemLibrary.quit_editor()
