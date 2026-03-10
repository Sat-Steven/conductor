zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_BEHAVIOR_ALT_TAB_SMART
  src/behaviors/behavior_alt_tab_smart.c
)
