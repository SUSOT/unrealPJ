"""Make nearby surfaces readable; use level-local HISM-ready material copies."""
import unreal as u, json
from pathlib import Path
l=u.get_editor_subsystem(u.LevelEditorSubsystem); api=u.get_editor_subsystem(u.EditorActorSubsystem)
assert l.load_level('/Game/Developers/MOON/Level/Showcase1')
actors=list(api.get_all_level_actors()); d=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector))
assert d.get_editor_property('straight_corridor')
for lamp in d.get_editor_property('lamps'):
    for c in lamp.get_components_by_class(u.LightComponent):
        c.set_mobility(u.ComponentMobility.MOVABLE)
        if isinstance(c,u.SpotLightComponent):
            c.set_intensity(18); c.set_attenuation_radius(1100)
            c.set_inner_cone_angle(38); c.set_outer_cone_angle(68)
        elif isinstance(c,u.PointLightComponent): c.set_intensity(1.2); c.set_attenuation_radius(800)
for a in actors:
    if isinstance(a,u.PostProcessVolume):
        a.set_editor_property('unbound',True); a.set_editor_property('enabled',True)
        pp=a.get_editor_property('settings')
        # Neutral shadow grading prevents the old crushed-black setting winning
        # over the new global grade. Exposure is fixed at the same saved -3 EV.
        for k,v in [('color_contrast_shadows',u.Vector4(1,1,1,1)),('color_gamma_shadows',u.Vector4(1,1,1,1)),('auto_exposure_bias',1.0)]:
            pp.set_editor_property('override_'+k,True); pp.set_editor_property(k,v)
        a.set_editor_property('settings',pp)
    if a.get_actor_label()=='Straight_DistantDarkness':
        c=a.get_component_by_class(u.ExponentialHeightFogComponent)
        c.set_editor_property('enable_volumetric_fog',False)
door=d.get_editor_property('escape_door')
for c in door.get_components_by_class(u.PointLightComponent): c.set_mobility(u.ComponentMobility.MOVABLE); c.set_intensity(20); c.set_attenuation_radius(700)
text_mat=u.load_asset('/Game/Developers/MOON/Materials/Escape/M_Escape_Text')
for sign in d.get_editor_property('direction_signs'): sign.get_component_by_class(u.TextRenderComponent).set_text_material(text_mat)
ext=next(a for a in actors if isinstance(a,u.ShowcaseRepeatExtension))
root='/Game/Developers/MOON/Materials/StraightInstanced'; copies={}
def instanced_material(material):
    if not material: return None
    path=material.get_path_name()
    if path.startswith(root): return material
    if path in copies: return copies[path]
    dest=root+'/'+material.get_name()
    result=u.load_asset(dest) if u.EditorAssetLibrary.does_asset_exist(dest) else u.EditorAssetLibrary.duplicate_asset(path,dest)
    assert result,path
    copies[path]=result
    if isinstance(result,u.MaterialInstanceConstant):
        parent=instanced_material(material.get_editor_property('parent'))
        u.MaterialEditingLibrary.set_material_instance_parent(result,parent)
    elif isinstance(result,u.Material):
        result.set_editor_property('used_with_instanced_static_meshes',True)
        u.MaterialEditingLibrary.recompile_material(result)
    assert u.EditorAssetLibrary.save_loaded_asset(result)
    return result
groups=list(ext.get_editor_property('mesh_groups'))
for g in groups: g.set_editor_property('materials',[instanced_material(m) for m in g.get_editor_property('materials')])
ext.set_editor_property('mesh_groups',groups); ext.rebuild_extension()
assert l.save_current_level()
Path(u.Paths.project_saved_dir()+'ShowcaseExpansion/straight_materials.json').write_text(json.dumps({k:v.get_path_name() for k,v in copies.items()},indent=2))
u.log_warning('SHOWCASE_STRAIGHT_POLISHED')
u.SystemLibrary.quit_editor()
