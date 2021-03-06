#define NRF_LOG_MODULE_NAME BATTERY

#include "src/service/battery.h"

#include <app_timer.h>
#include <app_timer_appsh.h>
#include <nrf_log.h>

#include "src/config.h"

static void _batteryLevelUpdate(void);

static void _batteryLevelMeasTimeoutHandler(void *p_context);

static void _batteryTimersInit();

static void _sensorSimulatorInit(void);

APP_TIMER_DEF(m_battery_timer_id);

static sensorsim_cfg_t m_battery_sim_cfg;
static sensorsim_state_t m_battery_sim_state;
static ble_bas_t m_bas;

ble_bas_t *battery_service() {
    return &m_bas;
}

void battery_init(void) {
    uint32_t err_code;
    ble_bas_init_t bas_init_obj;

    memset(&bas_init_obj, 0, sizeof(bas_init_obj));

    bas_init_obj.evt_handler = NULL;
    bas_init_obj.support_notification = true;
    bas_init_obj.p_report_ref = NULL;
    bas_init_obj.initial_batt_level = 100;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&bas_init_obj.battery_level_char_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&bas_init_obj.battery_level_char_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init_obj.battery_level_char_attr_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&bas_init_obj.battery_level_report_read_perm);

    err_code = ble_bas_init(&m_bas, &bas_init_obj);
    APP_ERROR_CHECK(err_code);

    _batteryTimersInit();
    _sensorSimulatorInit();
}

void battery_active(void) {
    APP_ERROR_CHECK(app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL));
}

void battery_deactive(void) {
    APP_ERROR_CHECK(app_timer_stop(m_battery_timer_id));
}

static void _batteryTimersInit(void) {
    APP_TIMER_APPSH_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, true);

    APP_ERROR_CHECK(app_timer_create(&m_battery_timer_id, APP_TIMER_MODE_REPEATED, _batteryLevelMeasTimeoutHandler));
}

static void _sensorSimulatorInit(void) {
    m_battery_sim_cfg.min = MIN_BATTERY_LEVEL;
    m_battery_sim_cfg.max = MAX_BATTERY_LEVEL;
    m_battery_sim_cfg.incr = BATTERY_LEVEL_INCREMENT;
    m_battery_sim_cfg.start_at_max = true;

    sensorsim_init(&m_battery_sim_state, &m_battery_sim_cfg);
}

static void _batteryLevelMeasTimeoutHandler(void *p_context) {
    UNUSED_PARAMETER(p_context);
    _batteryLevelUpdate();
}

static void _batteryLevelUpdate(void) {
    uint32_t err_code;
    uint8_t battery_level;

    battery_level = (uint8_t) sensorsim_measure(&m_battery_sim_state, &m_battery_sim_cfg);

    err_code = ble_bas_battery_level_update(&m_bas, battery_level);
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_PACKETS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
            ) {
        APP_ERROR_HANDLER(err_code);
    }
}