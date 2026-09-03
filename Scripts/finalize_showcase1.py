"""Attach the persistent runtime draw-distance policy to the added fixtures."""
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
extension = next(a for a in actors if isinstance(a, unreal.ShowcaseRepeatExtension))
fixtures = [a for a in actors if a.get_actor_label().startswith(('Endless_Cable_', 'Endless_Lamp_'))]
assert len(fixtures) == 392
sources = [a for a in actors if str(a.get_folder_path()) == '3' and
           ('BP_Cablespline' in a.get_class().get_name() or 'Ceilinglamp' in a.get_class().get_name())]
for fixture in fixtures:
    source = next(a for a in sources if a.get_class() == fixture.get_class())
    index = int(fixture.get_actor_label().rsplit('_', 1)[1]) - 5
    transform = source.get_actor_transform()
    p = transform.translation
    p.x += extension.get_actor_location().x - 1980.0 + index * 1200.0
    transform.translation = p
    fixture.set_actor_transform(transform, False, True)
extension.set_editor_property('repeated_fixtures', fixtures)
extension.apply_fixture_distance_limits()
assert levels.save_current_level()
unreal.log_warning('SHOWCASE_FIXTURE_POLICY_SAVED')
