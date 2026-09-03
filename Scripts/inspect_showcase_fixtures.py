import json
from pathlib import Path
import unreal
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Developers/MOON/Level/Showcase1')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
report = []
for actor in actors:
    if ('Ceilinglamp' not in actor.get_class().get_name()
        or (actor.get_actor_label().startswith('Endless_') and actor.get_actor_label() not in ('Endless_Lamp_005', 'Endless_Lamp_018', 'Endless_Lamp_200'))):
        continue
    data = {'label': actor.get_actor_label(), 'location': str(actor.get_actor_location()), 'lights': []}
    for component in actor.get_components_by_class(unreal.LightComponent):
        props = ['intensity','attenuation_radius','light_color','mobility','visible','hidden_in_game',
                 'max_draw_distance','max_distance_fade_range','cast_shadows','absolute_location']
        item = {'class': component.get_class().get_name(), 'location': str(component.get_world_location()),
                'rotation': str(component.get_world_rotation())}
        for name in props:
            item[name] = str(component.get_editor_property(name))
        data['lights'].append(item)
    report.append(data)
path = Path(unreal.Paths.project_saved_dir()) / 'ShowcaseExpansion' / 'fixtures.json'
path.write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log_warning('SHOWCASE_FIXTURES_INSPECTED')
