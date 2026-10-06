"""Apply only pacing and the TestLevel safety plane; preserve other authored actors."""
import unreal as u
from pathlib import Path
import shutil
import datetime

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
backup = Path(u.Paths.project_saved_dir()) / 'PacingFallBackup' / datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
backup.mkdir(parents=True)
maps = ['/Game/Developers/MOON/Level/Showcase1', '/Game/Developers/LCM/Levels/TestLevel']
for path in maps:
    source = Path(u.Paths.project_content_dir()) / (path.removeprefix('/Game/') + '.umap')
    shutil.copy2(source, backup / source.name)
    assert levels.load_level(path)
    if path == maps[0]:
        directors = [a for a in actors.get_all_level_actors() if isinstance(a, u.ShowcaseLoopDirector)]
        assert len(directors) == 1
        for key, value in [('first_change_distance', 3600.0), ('second_change_distance', 7600.0), ('turn_back_unlock_distance', 12000.0)]:
            directors[0].set_editor_property(key, value)
    else:
        guards = [a for a in actors.get_all_level_actors() if isinstance(a, u.FallRestartVolume)]
        assert len(guards) <= 1
        guard = guards[0] if guards else actors.spawn_actor_from_class(u.FallRestartVolume, u.Vector(0, 0, -1500))
        guard.set_actor_label('TestLevel_FallRestart')
        guard.set_editor_property('restart_below_z', -1500.0)
    assert levels.save_current_level()
u.log_warning('PACING_FALL_APPLIED backup=' + str(backup))
u.SystemLibrary.quit_editor()
