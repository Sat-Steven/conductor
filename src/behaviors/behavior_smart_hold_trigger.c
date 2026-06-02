#define DT_DRV_COMPAT zmk_behavior_smart_hold_trigger

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/events/keycode_state_changed.h>
#endif

struct smart_hold_trigger_config {
    int wait_time_ms;
};

struct smart_hold_trigger_data {
    bool active;
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    uint32_t hold_encoded;
    uint32_t trigger_encoded;
    struct k_work_delayable release_work;
#endif
};

static struct smart_hold_trigger_data smart_state;

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

static void release_hold_work_handler(struct k_work *work) {
    ARG_UNUSED(work);

    if (!smart_state.active) {
        return;
    }

    raise_zmk_keycode_state_changed_from_encoded(
        smart_state.hold_encoded, false, k_uptime_get());

    smart_state.active = false;
    smart_state.hold_encoded = 0;
    smart_state.trigger_encoded = 0;
}

#endif

static int smart_hold_trigger_press(struct zmk_behavior_binding *binding,
                                    struct zmk_behavior_binding_event event) {
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct smart_hold_trigger_config *config = dev->config;

    uint32_t hold_encoded = binding->param1;
    uint32_t trigger_encoded = binding->param2;

    if (!smart_state.active || smart_state.hold_encoded != hold_encoded) {
        if (smart_state.active) {
            raise_zmk_keycode_state_changed_from_encoded(
                smart_state.hold_encoded, false, event.timestamp);
        }

        smart_state.active = true;
        smart_state.hold_encoded = hold_encoded;
        smart_state.trigger_encoded = trigger_encoded;

        raise_zmk_keycode_state_changed_from_encoded(
            hold_encoded, true, event.timestamp);
    }

    raise_zmk_keycode_state_changed_from_encoded(
        trigger_encoded, true, event.timestamp);
    raise_zmk_keycode_state_changed_from_encoded(
        trigger_encoded, false, event.timestamp);

    k_work_reschedule(&smart_state.release_work, K_MSEC(config->wait_time_ms));
#else
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
#endif

    return ZMK_BEHAVIOR_OPAQUE;
}

static int smart_hold_trigger_release(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api smart_hold_trigger_driver_api = {
    .binding_pressed = smart_hold_trigger_press,
    .binding_released = smart_hold_trigger_release,
};

#define SMART_HOLD_TRIGGER_INIT(n)                                                  \
    static const struct smart_hold_trigger_config smart_hold_trigger_config_##n = { \
        .wait_time_ms = DT_INST_PROP(n, wait_time_ms),                              \
    };                                                                              \
                                                                                    \
    BEHAVIOR_DT_INST_DEFINE(n,                                                      \
                            NULL,                                                   \
                            NULL,                                                   \
                            &smart_state,                                           \
                            &smart_hold_trigger_config_##n,                         \
                            POST_KERNEL,                                            \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                    \
                            &smart_hold_trigger_driver_api);

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static int smart_hold_trigger_global_init(void) {
    smart_state.active = false;
    smart_state.hold_encoded = 0;
    smart_state.trigger_encoded = 0;
    k_work_init_delayable(&smart_state.release_work, release_hold_work_handler);
    return 0;
}
#else
static int smart_hold_trigger_global_init(void) {
    smart_state.active = false;
    return 0;
}
#endif

SYS_INIT(smart_hold_trigger_global_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

DT_INST_FOREACH_STATUS_OKAY(SMART_HOLD_TRIGGER_INIT)
