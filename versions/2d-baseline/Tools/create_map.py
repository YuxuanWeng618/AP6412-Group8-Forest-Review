import unreal

# Creates the actual blank level used by the fully planar game mode.
# All gameplay entities are 2D data rendered by the HUD, not projected meshes.
path = '/Game/Maps/Forest'
subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    subsystem.new_level(path)
else:
    subsystem.load_level(path)
subsystem.save_current_level()
unreal.log('FOREST_2D_MAP_SAVED ' + path)
