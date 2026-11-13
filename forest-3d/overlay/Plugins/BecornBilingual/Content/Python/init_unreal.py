import unreal
try:
    import becorn_bilingual
    becorn_bilingual.register_menu()
    unreal.log('[BecornBilingual] English / 中文 authoring menu registered')
except Exception as exc:
    unreal.log_warning('[BecornBilingual] Editor menu not initialized: '+str(exc))
