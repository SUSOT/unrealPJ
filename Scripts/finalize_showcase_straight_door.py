"""Keep the impossible doorway recognizable in the darkest part of the corridor."""
import unreal as u
l=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert l.load_level('/Game/Developers/MOON/Level/Showcase1')
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
d=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector)); door=d.get_editor_property('escape_door')
root='/Game/Developers/MOON/Materials/StraightInstanced'
for property_name,name,color in [('leaf_material','M_StraightExit_Door',(.018,.026,.03)),('frame_material','M_StraightExit_Frame',(.028,.038,.034))]:
    path=root+'/'+name
    if u.EditorAssetLibrary.does_asset_exist(path): mat=u.load_asset(path)
    else:
        mat=u.EditorAssetLibrary.duplicate_asset(door.get_editor_property(property_name).get_path_name(),path)
        mel=u.MaterialEditingLibrary
        node=mel.create_material_expression(mat,u.MaterialExpressionConstant3Vector,-300,300)
        node.set_editor_property('constant',u.LinearColor(*color,1))
        mel.connect_material_property(node,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
        mel.recompile_material(mat); u.EditorAssetLibrary.save_loaded_asset(mat)
    door.set_editor_property(property_name,mat)
# Native OnConstruction may not run for a Python property update.
for c in door.get_components_by_class(u.StaticMeshComponent):
    if c.get_name()=='DoorLeaf': c.set_material(0,door.get_editor_property('leaf_material'))
    elif c.get_name() in ('LeftJamb','RightJamb','Lintel'): c.set_material(0,door.get_editor_property('frame_material'))
assert l.save_current_level()
u.log_warning('SHOWCASE_STRAIGHT_DOOR_POLISHED')
u.SystemLibrary.quit_editor()
