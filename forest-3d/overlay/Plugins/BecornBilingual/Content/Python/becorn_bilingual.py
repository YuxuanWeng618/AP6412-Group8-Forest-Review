"""Editor-only, repeatable authoring. Saved assets work without Python at runtime."""
import json
import os
from hashlib import sha256
from pathlib import Path


import unreal

PLUGIN=Path(__file__).resolve().parents[2]
QUEST='/Game/Becorn/Data/Quest_Data/DA_Quest_Becorn'
QUEST_WIDGET='/Game/Becorn/Blueprints/Widgets/WBP_Quest'
INPUT_WIDGET='/Game/Becorn/Blueprints/Widgets/WBP_Input'

def catalog():
    return json.loads((PLUGIN/'Resources/translations.json').read_text(encoding='utf-8'))

def combined(row):
    return row['en']+'\n'+row['zh']

def guide_text(data):
    return '\n\n'.join(data['keyboard_guide'])


def widget_object(bp, name):
    obj=unreal.find_object(None,bp.get_path_name()+':WidgetTree.'+name)
    if not obj: raise RuntimeError('Missing widget object '+name)
    return obj

def font_filename():
    override = os.environ.get('BECORN_CJK_FONT')
    if override:
        filename = Path(override)
    else:
        engine_content = unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_content_dir())
        filename = Path(engine_content) / 'Slate/Fonts/DroidSansFallback.ttf'
    if not filename.is_absolute() or filename.suffix.lower() not in {'.ttf', '.otf'} or not filename.is_file():
        raise RuntimeError(f'CJK font unavailable at {filename}. Set BECORN_CJK_FONT to an absolute path to a local .ttf or .otf and restart Unreal Editor.')
    return filename


def import_font():
    font_path='/BecornBilingual/Fonts/DroidSansFallback_Font'
    if unreal.EditorAssetLibrary.does_asset_exist(font_path):
        font = unreal.load_asset(font_path)
        if not isinstance(font, unreal.Font):
            raise RuntimeError('Expected a UFont at ' + font_path)
        return font
    factory=unreal.FontFileImportFactory()
    factory.set_editor_property('batch_create_font_asset',unreal.BatchCreateFontAsset.YES)
    task=unreal.AssetImportTask()
    task.filename=str(font_filename())
    task.destination_path='/BecornBilingual/Fonts'
    task.destination_name='DroidSansFallback'
    task.automated=True
    task.replace_existing=False
    task.save=True
    task.factory=factory
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    font=unreal.load_asset(font_path)
    if not font:raise RuntimeError('CJK font import did not create '+font_path)
    return font

def set_font(text,font,size):
    info=text.get_editor_property('font')
    info.set_editor_property('font_object',font)
    info.set_editor_property('typeface_font_name','Default')
    info.set_editor_property('size',size)
    text.set_editor_property('font',info)

def structure(rows):
    return [{k:r.get_editor_property(k) for k in ['tag','amount','next_quest_tag','previous_quest_tag']} for r in rows]

def apply():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError('Stop Play In Editor before applying or saving translations')
    data=catalog();translations={r['tag']:r for r in data['quests']}
    quest=unreal.load_asset(QUEST)
    line=quest.get_editor_property('quest_line')
    rows=list(line.get_editor_property('quest_line_data'))
    before=structure(rows)
    if len(rows)!=9 or set(r.get_editor_property('tag') for r in rows)!=set(translations):
        raise RuntimeError('Quest structure changed; inspect before applying translations')
    for row in rows:
        entry=translations[row.get_editor_property('tag')]
        current = row.get_editor_property('desc')
        current_digest = sha256(current.encode('utf-8')).hexdigest()
        known_digests = {entry['source_sha256'], entry.get('previous_sha256')}
        if current != combined(entry) and current_digest not in known_digests:
            raise RuntimeError('Unexpected quest text; refusing to overwrite '+entry['tag'])
    font=import_font()
    quest_bp=unreal.load_asset(QUEST_WIDGET)
    input_bp=unreal.load_asset(INPUT_WIDGET)
    start_bp=unreal.load_asset('/Game/Becorn/Blueprints/Widgets/WBP_StartMenu')
    # The start button is an image in the original screen. Keep its design but label keyboard input.
    for obj in unreal.ObjectIterator(unreal.Image):
        if not obj.get_path_name().startswith(start_bp.get_path_name()+':WidgetTree.'):continue
        resource=obj.get_editor_property('brush').get_editor_property('resource_object')
        if resource and resource.get_name()=='T_UI_A':obj.set_visibility(unreal.SlateVisibility.COLLAPSED)
    for obj in unreal.ObjectIterator(unreal.TextBlock):
        if not obj.get_path_name().startswith(start_bp.get_path_name()+':WidgetTree.'):continue
        if str(obj.get_text()).strip().lower()=='press':
            obj.set_text(unreal.Text('SPACE\n开始 / Start'))
            set_font(obj,font,24)
    text=widget_object(quest_bp,'QUest_Desc')
    return_text=widget_object(input_bp,'TextBlock_194')
    set_font(text,font,24)
    text.set_editor_property('wrap_text_at',576.0)
    text.set_editor_property('auto_wrap_text',True)
    text.set_editor_property('margin',unreal.Margin(20,16,20,16))
    text.set_editor_property('text',unreal.Text(combined(data['quests'][0])))
    # Replace the old LB/RB texture at its existing layout slot.
    icon=widget_object(quest_bp,'Image_0')
    icon.set_visibility(unreal.SlateVisibility.COLLAPSED)
    shortcut=unreal.find_object(None,quest_bp.get_path_name()+':WidgetTree.KeyboardQuestShortcut')
    if not shortcut:
        shortcut=unreal.new_object(unreal.TextBlock,outer=unreal.find_object(None,quest_bp.get_path_name()+':WidgetTree'),name='KeyboardQuestShortcut')
        parent=icon.get_parent()
        parent.add_child(shortcut)
    shortcut.set_editor_property('text',unreal.Text('TAB\n任务'))
    set_font(shortcut,font,22)
    # Match the original icon's canvas layout when the parent is a CanvasPanel.
    icon_slot=icon.get_editor_property('slot');shortcut_slot=shortcut.get_editor_property('slot')
    if isinstance(shortcut_slot,unreal.CanvasPanelSlot):
        shortcut_slot.set_layout(icon_slot.get_layout())
        shortcut_slot.set_auto_size(True)
    background=widget_object(quest_bp,'Image_275')
    brush=background.get_editor_property('brush')
    size=brush.get_editor_property('image_size')
    size.set_editor_property('x',712.0)
    size.set_editor_property('y',200.0)
    brush.set_editor_property('image_size',size)
    background.set_editor_property('brush',brush)
    for row in rows:row.set_editor_property('desc',combined(translations[row.get_editor_property('tag')]))
    assert structure(rows)==before
    line.set_editor_property('quest_line_data',rows)
    quest.set_editor_property('quest_line',line)
    return_text.set_editor_property('text',unreal.Text(combined(data['return_to_game'])))
    set_font(return_text,font,22)
    return_text.set_editor_property('wrap_text_at',900.0)
    return_text.set_editor_property('auto_wrap_text',True)
    slot=return_text.get_editor_property('slot')
    slot.set_auto_size(True)
    slot.set_alignment(unreal.Vector2D(.5,0))
    slot.set_position(unreal.Vector2D(0,0))
    slot.set_anchors(unreal.Anchors(unreal.Vector2D(.5,.78),unreal.Vector2D(.5,.78)))
    # The original texture contains baked-in French. Preserve its asset, but
    # replace its displayed content with editable, localizable UMG text.
    widget_object(input_bp,'Image_137').set_visibility(unreal.SlateVisibility.COLLAPSED)
    widget_object(input_bp,'BackgroundBlur_166').set_editor_property('blur_strength',30.0)
    guide=unreal.find_object(None,input_bp.get_path_name()+':WidgetTree.BilingualControllerGuide')
    if not guide:
        tree=unreal.find_object(None,input_bp.get_path_name()+':WidgetTree')
        guide=unreal.new_object(unreal.TextBlock,outer=tree,name='BilingualControllerGuide')
        canvas=widget_object(input_bp,'CanvasPanel_13')
        canvas.add_child_to_canvas(guide)
    guide.set_editor_property('text',unreal.Text(guide_text(data)))
    set_font(guide,font,20)
    guide.set_editor_property('wrap_text_at',1100.0)
    guide.set_editor_property('auto_wrap_text',True)
    guide.set_editor_property('shadow_offset',unreal.Vector2D(2,2))
    guide.set_editor_property('shadow_color_and_opacity',unreal.LinearColor(0,0,0,1))
    guide_slot=guide.get_editor_property('slot')
    guide_slot.set_auto_size(True)
    guide_slot.set_alignment(unreal.Vector2D(.5,.5))
    guide_slot.set_position(unreal.Vector2D(0,0))
    guide_slot.set_anchors(unreal.Anchors(unreal.Vector2D(.5,.40),unreal.Vector2D(.5,.40)))
    for bp in [quest_bp,input_bp,start_bp]:unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    for asset in [quest,quest_bp,input_bp,start_bp,font]:
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset,False):raise RuntimeError('Save failed: '+asset.get_path_name())
    result={'passed':True,'quest_count':len(rows),'structure_unchanged':structure(rows)==before,'font':font.get_path_name(),'quests':[{'tag':r.get_editor_property('tag'),'text':r.get_editor_property('desc')} for r in rows],'return_to_game':str(return_text.get_text())}
    report=Path(unreal.Paths.project_saved_dir())/'BilingualAudit/apply-result.json'
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.log('[BecornBilingual] Saved 9 bilingual quests, return prompt, and shared-keyboard guide')
    return result

def register_menu():
    menus=unreal.ToolMenus.get()
    menu=menus.extend_menu('LevelEditor.MainMenu.Tools')
    entry=unreal.ToolMenuEntry(name='BecornBilingual.Apply',type=unreal.MultiBlockType.MENU_ENTRY)
    entry.set_label('forest: Apply English / 中文 Hints')
    entry.set_tool_tip('Stop PIE first. Reapply reviewed bilingual hints, keyboard guide, and CJK font to saved assets.')
    entry.set_string_command(unreal.ToolMenuStringCommandType.PYTHON,'','import becorn_bilingual; becorn_bilingual.apply()')
    menu.add_menu_entry('BecornBilingual',entry)
    menus.refresh_all_widgets()
