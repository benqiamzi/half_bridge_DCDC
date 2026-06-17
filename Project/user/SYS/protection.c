#include "protection.h"

/* 保护句柄全局实例 */
protection_handle_t protect_handle = {0};

/*===========================================================================
 * 内部辅助：保护触发后的统一动作（停 PWM、切状态）
 *===========================================================================*/
static void protect_trigger_action(void)
{
    OLED_Clear();
    ctrState.pwm_output_flag = 0;
    ctr_pwm_stop();
    ctrState.SMFlag = Err;
    ctrState.run_flag = RUN_MODE_STOP;
}

/*===========================================================================
 * protect_handle_reset()
 * 根据当前控制模式（Buck / Boost）填充保护阈值
 *===========================================================================*/
void protect_handle_reset(void)
{
    if (ctrState.ctr_mode == CTR_BUCK) {
        protect_handle.Vout_ovp_threshold = BUCK_VOUT_OVP_THRESHOLD;
        protect_handle.vout_ocp_threshold = BUCK_VOUT_OCP_THRESHOLD;
        protect_handle.Vin_ovp_threshold  = BUCK_VIN_OVP_THRESHOLD;
        protect_handle.Vin_uvp_threshold  = BUCK_VIN_UVP_THRESHOLD;
    } else if (ctrState.ctr_mode == CTR_BOOST) {
        protect_handle.Vout_ovp_threshold = BOOST_VOUT_OVP_THRESHOLD;
        protect_handle.vout_ocp_threshold = BOOST_VOUT_OCP_THRESHOLD;
        protect_handle.Vin_ovp_threshold  = BOOST_VIN_OVP_THRESHOLD;
        protect_handle.Vin_uvp_threshold  = BOOST_VIN_UVP_THRESHOLD;
    }

    protect_handle.short_i_threshold = CUR_SHORT_MIN;
    protect_handle.short_v_threshold = VOL_SHORT_MIN;

    /* 清除所有标志 */
    protect_handle.Vout_ovp_flag = 0;
    protect_handle.Vin_ovp_flag  = 0;
    protect_handle.Vin_uvp_flag  = 0;
    protect_handle.ocp_flag      = 0;
    protect_handle.short_flag    = 0;
}

/*===========================================================================
 * 过压/欠压检测内联宏
 *   展开为各自独立的 static 状态变量，避免被不同保护函数共享
 *===========================================================================*/
#define DEFINE_PROTECT_FUNC(name, threshold_field, flag_field, debounce, is_ovp) \
CCMRAM uint8_t name(protection_handle_t *protect, uint8_t run_flag, float value) \
{ \
    static uint8_t  state = 0; \
    static int32_t  cnt   = 0; \
    float th = protect->threshold_field; \
    switch (state) { \
    case 0: /* RELEASE */ \
        if (run_flag) { \
            _Bool trip = is_ovp ? (value > th) : (value < th); \
            if (trip) { state = 1; cnt = 0; } \
        } \
        break; \
    case 1: /* HOLD */ \
        if (run_flag) { \
            _Bool trip = is_ovp ? (value > th) : (value < th); \
            if (trip) { \
                cnt++; \
                if (cnt > debounce) { \
                    cnt = 0; state = 0; \
                    protect->flag_field = 1; \
                    protect_trigger_action(); \
                    return PROTECT_TRIGGERED; \
                } \
            } else { \
                cnt = 0; state = 0; \
                return PROTECT_RECOVERY; \
            } \
        } \
        break; \
    } \
    return PROTECT_OK; \
}

/* 展开为 3 个独立的保护函数，各有各的 static 变量 */
DEFINE_PROTECT_FUNC(protect_VoutOVP, Vout_ovp_threshold, Vout_ovp_flag, OVP_DEBOUNCE, 1)
DEFINE_PROTECT_FUNC(protect_VinOVP,  Vin_ovp_threshold,  Vin_ovp_flag,  OVP_DEBOUNCE, 1)
DEFINE_PROTECT_FUNC(protect_VinUVP,  Vin_uvp_threshold,  Vin_uvp_flag,  UVP_DEBOUNCE, 0)

/*===========================================================================
 * protect_VoutOCP() — 输出过流保护（独立 static 变量）
 *===========================================================================*/
CCMRAM uint8_t protect_VoutOCP(protection_handle_t *protect, uint8_t run_flag, float current)
{
    static uint8_t state = 0;
    static int32_t cnt   = 0;
    float th = protect->vout_ocp_threshold;

    switch (state) {
    case 0:
        if (run_flag && (current > th)) { state = 1; cnt = 0; }
        break;
    case 1:
        if (run_flag && (current > th)) {
            cnt++;
            if (cnt > (int32_t)OCP_DEBOUNCE) {
                cnt = 0; state = 0;
                protect->ocp_flag = 1;
                protect_trigger_action();
                return PROTECT_TRIGGERED;
            }
        } else {
            cnt = 0; state = 0;
            return PROTECT_RECOVERY;
        }
        break;
    }
    return PROTECT_OK;
}

/*===========================================================================
 * protect_ShortCircuit() — 短路保护（独立 static 变量）
 *   判据：电压 < short_v_threshold AND 电流 > short_i_threshold
 *===========================================================================*/
CCMRAM uint8_t protect_ShortCircuit(protection_handle_t *protect,
                                     uint8_t run_flag,
                                     float voltage,
                                     float current)
{
    static uint8_t state = 0;
    static int32_t cnt   = 0;

    switch (state) {
    case 0:
        if (run_flag &&
            (voltage < protect->short_v_threshold) &&
            (current > protect->short_i_threshold)) {
            state = 1; cnt = 0;
        }
        break;
    case 1:
        if (run_flag &&
            (voltage < protect->short_v_threshold) &&
            (current > protect->short_i_threshold)) {
            cnt++;
            if (cnt > (int32_t)SHORT_DEBOUNCE) {
                cnt = 0; state = 0;
                protect->short_flag = 1;
                protect_trigger_action();
                return PROTECT_TRIGGERED;
            }
        } else {
            cnt = 0; state = 0;
            return PROTECT_RECOVERY;
        }
        break;
    }
    return PROTECT_OK;
}
