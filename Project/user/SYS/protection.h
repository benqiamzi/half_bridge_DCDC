#ifndef __PROTECTION_H
#define __PROTECTION_H

#include "sys_bsp.h"
#include "function.h"

/*===========================================================================
 * Protection handle struct — stores float thresholds and fault flags
 *===========================================================================*/
typedef struct {
    /* 保护阈值（浮点数，单位为 V 或 A，便于直观设置） */
    float Vout_ovp_threshold;  // 输出过压阈值 (V)
    float Vin_ovp_threshold;   // 输入过压阈值 (V)
    float Vin_uvp_threshold;   // 输入欠压阈值 (V)
    float vout_ocp_threshold;  // 输出过流阈值 (A)
    float short_i_threshold;   // 短路电流阈值 (A)
    float short_v_threshold;   // 短路电压阈值 (V)

    /* 保护触发标志位 */
    _Bool Vout_ovp_flag;
    _Bool Vin_ovp_flag;
    _Bool Vin_uvp_flag;
    _Bool ocp_flag;
    _Bool short_flag;

} protection_handle_t;

extern protection_handle_t protect_handle;

/*===========================================================================
 * 保护阈值定义 —— 在此处统一修改，方便整定
 *===========================================================================*/

/* ---- Buck 模式 ---- */
#define BUCK_VOUT_OVP_THRESHOLD     15.0f     // 输出过压 (V)
#define BUCK_VOUT_OCP_THRESHOLD     5.1f      // 输出过流 (A)
#define BUCK_VIN_OVP_THRESHOLD      26.0f     // 输入过压 (V)
#define BUCK_VIN_UVP_THRESHOLD      18.0f     // 输入欠压 (V)

/* ---- Boost 模式 ---- */
#define BOOST_VOUT_OVP_THRESHOLD    26.0f     // 输出过压 (V)
#define BOOST_VOUT_OCP_THRESHOLD    2.0f      // 输出过流 (A)
#define BOOST_VIN_OVP_THRESHOLD     26.0f     // 输入过压 (V)
#define BOOST_VIN_UVP_THRESHOLD     10.0f     // 输入欠压 (V)

/* ---- 通用 ---- */
#define VOL_SHORT_MIN               4.0f      // 短路判据：电压低于此值 (V)
#define CUR_SHORT_MIN               2.0f      // 短路判据：电流高于此值 (A)

/* ---- 防抖计数（每个保护周期的累积次数，取决于调用频率 ~40kHz） ---- */
#define OVP_DEBOUNCE                20        // 过压防抖 ~1.25ms
#define UVP_DEBOUNCE                20        // 欠压防抖
#define OCP_DEBOUNCE                20        // 过流防抖
#define SHORT_DEBOUNCE              5        // 短路快速响应 ~0.25ms

/* ---- 保护函数返回值 ---- */
#define PROTECT_OK                  0         // 正常，未触发
#define PROTECT_RECOVERY            1         // 条件消失，恢复正常
#define PROTECT_TRIGGERED           2         // 保护条件满足，已触发动作

/*===========================================================================
 * 函数声明（所有保护函数均使用 float，直接与阈值比较）
 *===========================================================================*/

/* 根据当前模式初始化保护阈值 */
void protect_handle_reset(void);

/* 输出过压保护 —— 监测 Vy（Buck）或 Vx（Boost）*/
uint8_t protect_VoutOVP(protection_handle_t *protect, uint8_t run_flag, float voltage);

/* 输入过压保护 —— 监测 Vx（Buck）或 Vy（Boost）*/
uint8_t protect_VinOVP(protection_handle_t *protect, uint8_t run_flag, float voltage);

/* 输入欠压保护 */
uint8_t protect_VinUVP(protection_handle_t *protect, uint8_t run_flag, float voltage);

/* 输出过流保护 */
uint8_t protect_VoutOCP(protection_handle_t *protect, uint8_t run_flag, float current);

/* 短路保护 —— 电压低于阈值 & 电流高于阈值 */
uint8_t protect_ShortCircuit(protection_handle_t *protect, uint8_t run_flag, float voltage, float current);

#endif
