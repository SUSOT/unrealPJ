"""Read saved map pacing and fall geometry without modifying levels."""
import unreal as u
import json
from pathlib import Path
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
api = u.get_editor_subsystem(u.EditorActorSubsystem)
report = {}
for path in ['/Game/Developers/MOON/Level/Showcase1', '/Game/Developers/LCM/Levels/TestLevel']:
    assert levels.load_level(path)
    rows = []
    for a in api.get_all_level_actors():
        label = a.get_actor_label()
        if isinstance(a, u.ShowcaseLoopDirector):
            rows.append({'label': label, 'pacing': [a.get_editor_property(k) for k in ['first_change_distance', 'second_change_distance', 'turn_back_unlock_distance']]})
        elif isinstance(a, u.PlayerStart) or any(s in label.lower() for s in ['abyss', 'landscape', 'road', 'pit']):
            center, extent = a.get_actor_bounds(False)
            rows.append({'label': label, 'class': a.get_class().get_name(), 'location': str(a.get_actor_location()), 'center': str(center), 'extent': str(extent)})
    report[path] = rows
Path(u.Paths.project_saved_dir(), 'pacing_fall_inspection.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
u.SystemLibrary.quit_editor()
