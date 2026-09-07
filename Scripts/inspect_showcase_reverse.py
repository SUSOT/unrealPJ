"""Read-only layout/asset inspection for a reverse-direction table vignette."""
import json, math
from pathlib import Path
import unreal as u
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
api=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
actors=list(api.get_all_level_actors())
d=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector))
ext=next(a for a in actors if isinstance(a,u.ShowcaseRepeatExtension))
r=d.get_editor_property('loop_radius')
center=d.get_editor_property('loop_center')
def summary(t):
    p=t.translation-center
    return {'s':math.atan2(p.x,-p.y)*r,'radial':math.hypot(p.x,p.y)-r,'z':t.translation.z,'transform':str(t)}
rows=[]
for a in actors:
    if a.get_actor_label()=='Loop_Change_SM_Chair_16': rows.append({'chair':a.get_actor_label(),**summary(a.get_actor_transform()),'bounds':str(a.get_actor_bounds(False))})
for gi,g in enumerate(ext.get_editor_property('mesh_groups')):
    mesh=g.get_editor_property('mesh')
    for ti,t in enumerate(g.get_editor_property('source_transforms')):
        s=summary(t)
        if -1450<s['s']<-650 and mesh.get_name() in ('SM_Table','SM_Chair','SM_Plate','SM_Plate2','SM_CoffeMug','SM_Knife','SM_Fork'):
            rows.append({'group':gi,'index':ti,'mesh':mesh.get_name(),'bounds':str(mesh.get_bounds()),**s})
for lamp in d.get_editor_property('lamps'):
    s=summary(lamp.get_actor_transform())
    if -2400<s['s']<0:
        rows.append({'lamp':lamp.get_actor_label(),**s,'lights':[{k:str(c.get_editor_property(k)) for k in ('intensity','light_color')} for c in lamp.get_components_by_class(u.LightComponent)]})
out=Path(u.Paths.project_saved_dir())/'ShowcaseExpansion/reverse_layout.json'
out.write_text(json.dumps(rows,indent=2),encoding='utf-8')
u.log_warning('REVERSE_LAYOUT '+str(out))
