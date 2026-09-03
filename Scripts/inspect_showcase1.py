import json
from pathlib import Path
import unreal

MAP = '/Game/Developers/MOON/Level/Showcase1'
OUTPUT = Path(unreal.Paths.project_saved_dir()) / 'ShowcaseExpansion'
OUTPUT.mkdir(parents=True, exist_ok=True)

def vec(v):
    return [v.x, v.y, v.z]

def transform(t):
    return {'translation': vec(t.translation), 'rotation': [t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w], 'scale': vec(t.scale3d)}

def prop(obj, name):
    try:
        value = obj.get_editor_property(name)
        return value if isinstance(value, (int, float, str, bool)) else str(value)
    except Exception:
        return None

if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP):
    raise RuntimeError('Could not load Showcase1')

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
report = {'map': MAP, 'actors': []}
for actor in actors:
    origin, extent = actor.get_actor_bounds(False)
    record = {'name': actor.get_name(), 'label': actor.get_actor_label(), 'class': actor.get_class().get_path_name(),
              'folder': str(actor.get_folder_path()), 'transform': transform(actor.get_actor_transform()),
              'bounds': {'origin': vec(origin), 'extent': vec(extent)}, 'hidden': prop(actor, 'hidden'),
              'meshes': [], 'lights': []}
    for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh = comp.get_editor_property('static_mesh')
        item = {'name': comp.get_name(), 'class': comp.get_class().get_name(),
                'mesh': mesh.get_path_name() if mesh else None,
                'transform': transform(comp.get_world_transform()),
                'materials': [m.get_path_name() if m else None for m in comp.get_materials()],
                'collision_profile': str(comp.get_collision_profile_name()), 'collision': str(comp.get_collision_enabled()),
                'cast_shadow': prop(comp, 'cast_shadow'), 'mobility': prop(comp, 'mobility'),
                'visible': prop(comp, 'visible'), 'hidden_in_game': prop(comp, 'hidden_in_game')}
        if mesh:
            mb = mesh.get_bounds()
            item['mesh_bounds'] = {'origin': vec(mb.origin), 'extent': vec(mb.box_extent)}
        if isinstance(comp, unreal.InstancedStaticMeshComponent):
            item['instances'] = [transform(comp.get_instance_transform(i, world_space=True)) for i in range(comp.get_instance_count())]
        record['meshes'].append(item)
    for comp in actor.get_components_by_class(unreal.LightComponent):
        record['lights'].append({'name': comp.get_name(), 'class': comp.get_class().get_name(),
                                 'transform': transform(comp.get_world_transform()),
                                 **{key: prop(comp, key) for key in ['intensity', 'light_color', 'attenuation_radius', 'cast_shadows', 'mobility', 'temperature', 'use_temperature', 'source_radius', 'indirect_lighting_intensity', 'volumetric_scattering_intensity']}})
    if isinstance(actor, unreal.PostProcessVolume):
        record['post_process'] = str(actor.get_editor_property('settings'))
    report['actors'].append(record)

with (OUTPUT / 'inspection.json').open('w', encoding='utf-8') as output_file:
    json.dump(report, output_file, ensure_ascii=False, indent=2)
unreal.log_warning('SHOWCASE_INSPECTION_COMPLETE actors=%d output=%s' % (len(actors), OUTPUT))
