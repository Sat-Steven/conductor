zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_BEHAVIOR_SMART_HOLD_TRIGGER
  src/behaviors/behavior_smart_hold_trigger.c
)
