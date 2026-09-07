"""Replace exaggerated props with grounded witness-dependent changes. Back up first.

Idempotent on the Showcase1 escape layout. Saves only this map and new sound assets.
"""
import math
from pathlib import Path
import unreal as u

levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
api=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
actors=list(api.get_all_level_actors())
d=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector))
ext=next(a for a in actors if isinstance(a,u.ShowcaseRepeatExtension))
booths=[a for a in actors if a.get_actor_label().startswith('Loop_Change_Booth_')]
assert ext.get_repeated_instance_count() in (2304,2336)
assert len(booths) in (0,32)
groups=list(ext.get_editor_property('mesh_groups'))
for actor in booths:
    c=actor.static_mesh_component
    materials=[c.get_material(i) for i in range(c.get_num_materials())]
    g=next(g for g in groups if g.get_editor_property('mesh')==c.static_mesh and list(g.get_editor_property('materials'))==materials)
    g.set_editor_property('source_transforms',list(g.get_editor_property('source_transforms'))+[actor.get_actor_transform()])
if booths:
    ext.set_editor_property('mesh_groups',groups)
    ext.rebuild_extension()

changes=[]
for actor in sorted(actors,key=lambda a:a.get_actor_label()):
    label=actor.get_actor_label()
    if not label.startswith(('Loop_Change_SM_Chair_','Loop_Change_SM_WallPainting_1_')): continue
    t=actor.get_actor_transform()
    changed=u.Transform(location=t.translation,scale=t.scale3d)
    if 'SM_Chair_' in label:
        # Pull slightly out from the table, still on the floor and off the aisle.
        center=d.get_editor_property('loop_center')
        v=t.translation-center
        distance=math.hypot(v.x,v.y)
        radial=u.Vector(v.x/distance,v.y/distance,0)
        inward=radial*(-1 if distance>d.get_editor_property('loop_radius') else 1)
        changed.translation=t.translation+inward*28
        changed.rotation=t.rotation*u.Rotator(yaw=65).quaternion()
        stage=1
    else:
        # The familiar painting turns to face the wall; no floating/stretching.
        changed.rotation=t.rotation*u.Rotator(yaw=180).quaternion()
        stage=2
    change=u.ShowcaseSpatialChange()
    change.set_editor_property('target',actor)
    change.set_editor_property('changed_transform',changed)
    change.set_editor_property('stage',stage)
    changes.append(change)
assert len(changes)==32
d.set_editor_property('spatial_changes',changes)
for actor in booths: assert api.destroy_actor(actor)
assert ext.get_repeated_instance_count()==2336

destination='/Game/Developers/MOON/Audio/Horror'
source=Path(u.Paths.project_dir())/'SourceArt/ShowcaseHorror/Audio'
tasks=[]
for wav in sorted(source.glob('*.wav')):
    task=u.AssetImportTask()
    for key,value in [('filename',str(wav)),('destination_path',destination),('destination_name',wav.stem),('automated',True),('replace_existing',True),('save',True)]: task.set_editor_property(key,value)
    tasks.append(task)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
def sound(name):
    asset=u.load_asset(destination+'/'+name)
    assert isinstance(asset,u.SoundWave),name
    asset.set_editor_property('looping',name=='SW_Loop_RoomTone')
    # Tiny latency-sensitive cues: avoid decoder warm-up on their first play.
    asset.set_sound_asset_compression_type(u.SoundAssetCompressionType.PCM)
    u.EditorAssetLibrary.save_loaded_asset(asset)
    return asset
d.set_editor_property('footstep_sounds',[sound('SW_Loop_Step_'+str(i)) for i in range(4)])
d.set_editor_property('chair_drag_sound',sound('SW_Loop_ChairDrag'))
d.set_editor_property('relay_sound',sound('SW_Loop_Relay'))
d.set_editor_property('room_tone_sound',sound('SW_Loop_RoomTone'))
d.set_editor_property('horror_volume',.8)
d.set_editor_property('enable_presence_audio',True)
d.set_editor_property('flicker_interval',12.0)
for actor in actors:
    if isinstance(actor,u.PostProcessVolume) and actor.get_actor_label()=='PostProcessVolume':
        pp=actor.get_editor_property('settings')
        for key,value in [('override_auto_exposure_min_brightness',True),('override_auto_exposure_max_brightness',True),('auto_exposure_min_brightness',-3.0),('auto_exposure_max_brightness',-3.0)]: pp.set_editor_property(key,value)
        actor.set_editor_property('settings',pp)
assert levels.save_current_level()
u.log_warning('SHOWCASE_HORROR_SAVED: 2336 HISM, 32 grounded props, seven mono sounds. Original furniture restored; only witness-dependent changes remain.')
u.SystemLibrary.quit_editor()
