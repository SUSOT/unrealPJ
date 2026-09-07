"""Targeted visual polish after the initial install; no shared asset changes."""
import unreal as u
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
api=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
actors=list(api.get_all_level_actors())
lamp=next(a for a in actors if a.get_actor_label()=='Loop_Reverse_TableLight')
lamp.get_component_by_class(u.LightComponent).set_intensity(28.0)
ext=next(a for a in actors if isinstance(a,u.ShowcaseRepeatExtension))
d=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector))
chair=d.get_editor_property('reverse_chair_tucked_pose').translation
tables=[t for g in ext.get_editor_property('mesh_groups') if g.get_editor_property('mesh').get_name()=='SM_Table' for t in g.get_editor_property('source_transforms')]
table=min(tables,key=lambda t:(t.translation-chair).length())
for name,x,y in [('Plate',-34,0),('Fork',-34,-21),('Knife',-34,21),('Mug',-12,22)]:
    actor=next(a for a in actors if a.get_actor_label()=='Loop_Reverse_'+name)
    loc=table.transform_location(u.Vector(x,y,0)); loc.z=actor.get_actor_location().z
    actor.set_actor_location(loc,False,True)
assert levels.save_current_level()
u.log_warning('SHOWCASE_REVERSE_POLISH_SAVED')
u.SystemLibrary.quit_editor()
