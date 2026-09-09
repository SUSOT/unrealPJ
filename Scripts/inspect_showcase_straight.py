"""Read current map settings and candidate assets; no save."""
import unreal as u, json
from pathlib import Path
l=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert l.load_level('/Game/Developers/MOON/Level/Showcase1')
actors=list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
d=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector))
e=next(a for a in actors if isinstance(a,u.ShowcaseRepeatExtension))
def t(v): return {'p':[v.translation.x,v.translation.y,v.translation.z],'q':[v.rotation.x,v.rotation.y,v.rotation.z,v.rotation.w],'s':[v.scale3d.x,v.scale3d.y,v.scale3d.z]}
report={'groups':[{'mesh':g.get_editor_property('mesh').get_path_name(),'materials':[m.get_path_name() if m else None for m in g.get_editor_property('materials')],'transforms':[t(v) for v in g.get_editor_property('source_transforms')]} for g in e.get_editor_property('mesh_groups')],
 'actors':[{'label':a.get_actor_label(),'class':a.get_class().get_name(),'folder':str(a.get_folder_path()),'transform':t(a.get_actor_transform())} for a in actors],
 'atmosphere':[]}
for a in actors:
    if isinstance(a,u.PostProcessVolume): report['atmosphere'].append({'label':a.get_actor_label(),'settings':str(a.get_editor_property('settings'))})
    if isinstance(a,u.ExponentialHeightFog):
        c=a.get_component_by_class(u.ExponentialHeightFogComponent)
        report['atmosphere'].append({'label':a.get_actor_label(),**{k:str(c.get_editor_property(k)) for k in ('fog_density','fog_height_falloff','fog_inscattering_luminance','fog_max_opacity','start_distance','volumetric_fog')}})
Path(u.Paths.project_saved_dir()+'ShowcaseExpansion/straight_before.json').write_text(json.dumps(report,indent=2))
u.log_warning('SHOWCASE_STRAIGHT_INSPECTED')
u.SystemLibrary.quit_editor()
