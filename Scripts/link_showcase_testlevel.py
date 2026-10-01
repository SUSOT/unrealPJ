import unreal as u
from pathlib import Path
import shutil
import datetime

path = '/Game/Developers/MOON/Level/Showcase1'
target = '/Game/Developers/LCM/Levels/TestLevel.TestLevel'
assert u.EditorAssetLibrary.does_asset_exist(target)
backup = Path(u.Paths.project_saved_dir()) / 'DoorTravelBackup' / datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
backup.mkdir(parents=True)
shutil.copy2(Path(u.Paths.project_content_dir()) / 'Developers/MOON/Level/Showcase1.umap', backup / 'Showcase1.umap')
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level(path)
actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
doors = [a for a in actors if isinstance(a, u.ShowcaseEscapeDoor)]
assert len(doors) == 1
destination_world = u.load_object(None, target)
assert destination_world
doors[0].set_editor_property('destination_level', destination_world)
assert levels.save_current_level()
u.log_warning('SHOWCASE_DESTINATION_SAVED TestLevel')
u.SystemLibrary.quit_editor()
