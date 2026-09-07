"""Stronger spatial changes and walking-speed pacing. Existing props only.

Run once on the 2336-instance escape map. Original actor poses remain unchanged
until runtime; additional booth props are extracted from HISM, not duplicated.
"""
import json
import math
from pathlib import Path
import unreal as u

levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
api=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
actors=list(api.get_all_level_actors())
d=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector))
ext=next(a for a in actors if isinstance(a,u.ShowcaseRepeatExtension))
assert ext.get_repeated_instance_count()==2336, 'Unexpected layout or tuning already installed.'
assert not any(a.get_actor_label().startswith('Loop_Change_Booth_') for a in actors)
for key,value in [('first_change_distance',4000),('second_change_distance',7000),('turn_back_unlock_distance',9000),('enable_light_flicker',True),('flicker_strength',1.0),('flicker_interval',10.0)]:
    d.set_editor_property(key,value)

changes=list(d.get_editor_property('spatial_changes'))
assert len(changes)==48
for change in changes:
    a=change.get_editor_property('target')
    t=a.get_actor_transform()
    index=int(a.get_actor_label().rsplit('_',1)[1])-1
    stage=change.get_editor_property('stage')
    result=u.Transform(location=t.translation,scale=t.scale3d)
    if 'SM_Chair' in a.get_actor_label():
        if stage==1:
            delta=u.Rotator(yaw=45 if index%2 else -55)
        else:
            # Visible levitation and tilt while keeping the original XY anchor.
            result.translation=t.translation+u.Vector(0,0,30+(index%3)*15)
            delta=u.Rotator(yaw=145 if index%2 else -150,pitch=18 if index%2 else -16,roll=10)
        result.rotation=t.rotation*delta.quaternion()
    else:
        result.rotation=t.rotation*u.Rotator(pitch=180 if index%2 else 165).quaternion()
        result.scale3d=t.scale3d*1.20
    change.set_editor_property('changed_transform',result)

# Select one booth on each side of each of the 16 rooms, preserving materials.
groups=list(ext.get_editor_property('mesh_groups'))
chosen={}
for gi,g in enumerate(groups):
    if g.get_editor_property('mesh').get_name()!='SM_Booth': continue
    for ti,t in enumerate(g.get_editor_property('source_transforms')):
        p=t.translation
        x,y=p.x,p.y-3105.7749073643904
        angle=math.atan2(x,-y)%(2*math.pi)
        cell=int(angle/(2*math.pi)*16)%16
        side='Outer' if math.hypot(x,y)>3055.7749073643904 else 'Inner'
        chosen.setdefault((cell,side),(gi,ti,t))
assert len(chosen)==32, str(chosen.keys())
removed={}
for (cell,side),(gi,ti,t) in sorted(chosen.items()):
    g=groups[gi]
    a=api.spawn_actor_from_class(u.StaticMeshActor,t.translation)
    a.set_actor_label('Loop_Change_Booth_%s_%02d'%(side,cell+1))
    a.set_folder_path('CircularLoop/Escape/SpatialChanges')
    c=a.static_mesh_component
    c.set_mobility(u.ComponentMobility.MOVABLE)
    c.set_static_mesh(g.get_editor_property('mesh'))
    for slot,mat in enumerate(g.get_editor_property('materials')): c.set_material(slot,mat)
    c.set_collision_profile_name('BlockAll')
    c.set_editor_property('generate_overlap_events',False)
    a.set_actor_transform(t,False,True)
    for stage,factor in ((2,1.35),(3,2.05 if cell%2 else 2.25)):
        changed=u.Transform(location=t.translation,scale=u.Vector(t.scale3d.x,t.scale3d.y,t.scale3d.z*factor))
        changed.rotation=t.rotation
        change=u.ShowcaseSpatialChange()
        change.set_editor_property('target',a)
        change.set_editor_property('changed_transform',changed)
        change.set_editor_property('stage',stage)
        changes.append(change)
    removed.setdefault(gi,set()).add(ti)
for gi,indices in removed.items():
    source=list(groups[gi].get_editor_property('source_transforms'))
    groups[gi].set_editor_property('source_transforms',[t for i,t in enumerate(source) if i not in indices])
ext.set_editor_property('mesh_groups',groups)
ext.rebuild_extension()
d.set_editor_property('spatial_changes',changes)
assert ext.get_repeated_instance_count()==2304 and len(changes)==112
assert levels.save_current_level()
report={'map':'/Game/Developers/MOON/Level/Showcase1','stages_m':[40,70,90],
        'intervals_m':[40,30,20],'hism_instances':2304,'movable_props':64,'spatial_changes':112,
        'flicker_interval_seconds':10,'max_flickering_fixtures':3,'backtrack_m':5,
        'chair_lift_cm':[30,45,60],'booth_vertical_scale':[1.35,2.05,2.25]}
out=Path(u.Paths.project_saved_dir())/'ShowcaseExpansion'
(out/'escape_tuning.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log_warning('SHOWCASE_ESCAPE_TUNED '+json.dumps(report))
