"""Polish task-owned signs and exit-room brightness; preserve original lighting."""
import math
import unreal as u

levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
api=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
d=next(a for a in api.get_all_level_actors() if isinstance(a,u.ShowcaseLoopDirector))
r=d.get_editor_property('loop_radius')
signs=d.get_editor_property('direction_signs')
assert len(signs)==8
text_path='/Game/Developers/MOON/Materials/Escape/M_Escape_Text'
if u.EditorAssetLibrary.does_asset_exist(text_path):
    text_mat=u.load_asset(text_path)
else:
    text_mat=u.EditorAssetLibrary.duplicate_asset('/Engine/EngineMaterials/DefaultTextMaterialOpaque',text_path)
    assert text_mat
    mel=u.MaterialEditingLibrary
    source=mel.get_material_property_input_node(text_mat,u.MaterialProperty.MP_BASE_COLOR)
    output=mel.get_material_property_input_node_output_name(text_mat,u.MaterialProperty.MP_BASE_COLOR)
    assert source
    glow=mel.create_material_expression(text_mat,u.MaterialExpressionMultiply,400,0)
    glow.set_editor_property('const_b',.04)
    mel.connect_material_expressions(source,output,glow,'A')
    mel.connect_material_property(glow,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    text_mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    mel.recompile_material(text_mat)
    assert u.EditorAssetLibrary.save_loaded_asset(text_mat)
for index,a in enumerate(signs):
    angle=(index*2400+900)/r
    a.set_actor_location(u.Vector((r+415)*math.sin(angle),r-(r+415)*math.cos(angle)+50,315),False,True)
    a.set_actor_rotation(u.Rotator(yaw=math.degrees(angle)+90),True)
    c=a.get_component_by_class(u.TextRenderComponent)
    c.set_text('EXIT  >')
    c.set_text_material(text_mat)
    tangent=u.Vector(math.cos(angle),math.sin(angle),0)
    arrow=a.get_actor_rotation().quaternion().rotate_vector(u.Vector(0,-1,0))
    assert arrow.dot(tangent)>.999
door=d.get_editor_property('escape_door')
door.get_component_by_class(u.TextRenderComponent).set_text_material(text_mat)
for a in api.get_all_level_actors():
    if a.get_actor_label().startswith('Escape_SoftLight_'):
        a.get_component_by_class(u.PointLightComponent).set_intensity(1.5)
assert levels.save_current_level()
u.log_warning('ESCAPE_WAYFINDING_VERIFIED: all 8 arrows point forwards at start')
