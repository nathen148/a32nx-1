#include "Autothrust.h"
#include "Autothrust_private.h"
#include "look1_binlxpw.h"
#include "look2_binlcpw.h"
#include "look2_binlxpw.h"

const uint8_T Autothrust_IN_InAir = 1U;
const uint8_T Autothrust_IN_OnGround = 2U;
void AutothrustModelClass::Autothrust_TimeSinceCondition(real_T rtu_time, boolean_T rtu_condition, real_T *rty_y,
  rtDW_TimeSinceCondition_Autothrust_T *localDW)
{
  if (!localDW->eventTime_not_empty) {
    localDW->eventTime = rtu_time;
    localDW->eventTime_not_empty = true;
  }

  if ((!rtu_condition) || (localDW->eventTime == 0.0)) {
    localDW->eventTime = rtu_time;
  }

  *rty_y = rtu_time - localDW->eventTime;
}

void AutothrustModelClass::Autothrust_ThrustMode1(real_T rtu_u, real_T *rty_y)
{
  if (rtu_u < 0.0) {
    *rty_y = 1.0;
  } else if (rtu_u == 0.0) {
    *rty_y = 2.0;
  } else if ((rtu_u > 0.0) && (rtu_u < 25.0)) {
    *rty_y = 3.0;
  } else if ((rtu_u >= 25.0) && (rtu_u < 35.0)) {
    *rty_y = 4.0;
  } else if ((rtu_u >= 35.0) && (rtu_u < 45.0)) {
    *rty_y = 5.0;
  } else if (rtu_u == 45.0) {
    *rty_y = 6.0;
  } else {
    *rty_y = 0.0;
  }
}

void AutothrustModelClass::Autothrust_TLAComputation1(const athr_out *rtu_in, real_T rtu_TLA, real_T *rty_N1c, boolean_T
  *rty_inReverse)
{
  real_T N1_begin;
  real_T N1_end;
  real_T u0;
  int32_T TLA_begin;
  int32_T TLA_end;
  u0 = rtu_TLA;
  if ((!rtu_in->data.on_ground) && (0.0 > rtu_TLA)) {
    u0 = 0.0;
  }

  *rty_inReverse = (u0 < 0.0);
  if (u0 >= 0.0) {
    if (u0 <= 25.0) {
      TLA_begin = 0;
      N1_begin = rtu_in->input.thrust_limit_IDLE_percent;
      TLA_end = 25;
      N1_end = rtu_in->input.thrust_limit_CLB_percent;
    } else if (u0 <= 35.0) {
      TLA_begin = 25;
      N1_begin = rtu_in->input.thrust_limit_CLB_percent;
      TLA_end = 35;
      if (rtu_in->data_computed.is_FLX_active) {
        N1_end = rtu_in->input.thrust_limit_FLEX_percent;
      } else {
        N1_end = rtu_in->input.thrust_limit_MCT_percent;
      }
    } else {
      TLA_begin = 35;
      if (rtu_in->data_computed.is_FLX_active) {
        N1_begin = rtu_in->input.thrust_limit_FLEX_percent;
      } else {
        N1_begin = rtu_in->input.thrust_limit_MCT_percent;
      }

      TLA_end = 45;
      N1_end = rtu_in->input.thrust_limit_TOGA_percent;
    }
  } else {
    u0 = std::abs(u0);
    if (u0 <= 6.0) {
      u0 = 6.0;
    }

    TLA_begin = 6;
    N1_begin = std::abs(rtu_in->input.thrust_limit_IDLE_percent + 1.0);
    TLA_end = 20;
    N1_end = std::abs(rtu_in->input.thrust_limit_REV_percent);
  }

  *rty_N1c = (N1_end - N1_begin) / static_cast<real_T>(TLA_end - TLA_begin) * (u0 - static_cast<real_T>(TLA_begin)) +
    N1_begin;
}

void AutothrustModelClass::Autothrust_RateLimiterwithThreshold(real_T rtu_U, real_T rtu_up, real_T rtu_lo, real_T rtu_Ts,
  real_T rtu_init, real_T rtu_threshold, real_T *rty_Y, rtDW_RateLimiterwithThreshold_Autothrust_T *localDW)
{
  real_T u0;
  real_T u1;
  if (!localDW->pY_not_empty) {
    localDW->pY = rtu_init;
    localDW->pY_not_empty = true;
  }

  u0 = rtu_U - localDW->pY;
  u1 = std::abs(rtu_up) * rtu_Ts;
  if (u0 < u1) {
    u1 = u0;
  }

  u0 = -std::abs(rtu_lo) * rtu_Ts;
  if (u1 > u0) {
    u0 = u1;
  }

  *rty_Y = u0 + localDW->pY;
  if (std::abs(rtu_U - *rty_Y) > rtu_threshold) {
    *rty_Y = rtu_U;
  }

  localDW->pY = *rty_Y;
}

real_T AutothrustModelClass::Autothrust_timeSinceConditionArmedActive(real_T in_time_simulation_time, athr_status status)
{
  if (!Autothrust_DWork.eventTime_not_empty_hl) {
    Autothrust_DWork.eventTime_c = in_time_simulation_time;
    Autothrust_DWork.eventTime_not_empty_hl = true;
  }

  if ((!(status != athr_status_DISENGAGED)) || (Autothrust_DWork.eventTime_c == 0.0)) {
    Autothrust_DWork.eventTime_c = in_time_simulation_time;
  }

  return in_time_simulation_time - Autothrust_DWork.eventTime_c;
}

void AutothrustModelClass::step()
{
  athr_out rtb_BusAssignment_n;
  real_T result_tmp[9];
  real_T result[3];
  real_T x[3];
  real_T Phi_rad;
  real_T Theta_rad;
  real_T ca;
  real_T denom_tmp;
  real_T rtb_Divide;
  real_T rtb_Gain2;
  real_T rtb_Gain3;
  real_T rtb_Max_g;
  real_T rtb_OATCornerPoint;
  real_T rtb_Saturation;
  real_T rtb_Sum_ew;
  real_T rtb_Sum_h;
  real_T rtb_Switch1_k;
  real_T rtb_Switch2_k;
  real_T rtb_Switch_dx;
  real_T rtb_Switch_m;
  real_T rtb_y_b4;
  real_T rtb_y_e;
  int32_T i;
  int32_T rtb_on_ground;
  boolean_T ATHR_ENGAGED_tmp;
  boolean_T ATHR_ENGAGED_tmp_0;
  boolean_T ATHR_ENGAGED_tmp_1;
  boolean_T condition02;
  boolean_T condition_AP_FD_ATHR_Specific;
  boolean_T condition_AlphaFloor;
  boolean_T condition_AlphaFloor_tmp;
  boolean_T condition_TOGA;
  boolean_T rtb_Compare_e;
  boolean_T rtb_NOT;
  boolean_T rtb_NOT1_m;
  boolean_T rtb_out;
  boolean_T rtb_y_k;
  boolean_T tmp;
  athr_mode mode;
  athr_status status;
  rtb_Gain2 = Autothrust_P.Gain2_Gain * Autothrust_U.in.data.Theta_deg;
  rtb_Gain3 = Autothrust_P.Gain3_Gain * Autothrust_U.in.data.Phi_deg;
  Theta_rad = 0.017453292519943295 * rtb_Gain2;
  Phi_rad = 0.017453292519943295 * rtb_Gain3;
  rtb_Saturation = std::cos(Theta_rad);
  Theta_rad = std::sin(Theta_rad);
  rtb_y_e = std::sin(Phi_rad);
  Phi_rad = std::cos(Phi_rad);
  result_tmp[0] = rtb_Saturation;
  result_tmp[3] = 0.0;
  result_tmp[6] = -Theta_rad;
  result_tmp[1] = rtb_y_e * Theta_rad;
  result_tmp[4] = Phi_rad;
  result_tmp[7] = rtb_Saturation * rtb_y_e;
  result_tmp[2] = Phi_rad * Theta_rad;
  result_tmp[5] = 0.0 - rtb_y_e;
  result_tmp[8] = Phi_rad * rtb_Saturation;
  for (i = 0; i < 3; i++) {
    result[i] = result_tmp[i + 6] * Autothrust_U.in.data.bz_m_s2 + (result_tmp[i + 3] * Autothrust_U.in.data.by_m_s2 +
      result_tmp[i] * Autothrust_U.in.data.bx_m_s2);
  }

  rtb_Saturation = Autothrust_P.Gain_Gain_p * Autothrust_U.in.data.gear_strut_compression_1 -
    Autothrust_P.Constant1_Value_d;
  if (rtb_Saturation > Autothrust_P.Saturation_UpperSat) {
    rtb_Saturation = Autothrust_P.Saturation_UpperSat;
  } else if (rtb_Saturation < Autothrust_P.Saturation_LowerSat) {
    rtb_Saturation = Autothrust_P.Saturation_LowerSat;
  }

  Phi_rad = Autothrust_P.Gain1_Gain * Autothrust_U.in.data.gear_strut_compression_2 - Autothrust_P.Constant1_Value_d;
  if (Phi_rad > Autothrust_P.Saturation1_UpperSat) {
    Phi_rad = Autothrust_P.Saturation1_UpperSat;
  } else if (Phi_rad < Autothrust_P.Saturation1_LowerSat) {
    Phi_rad = Autothrust_P.Saturation1_LowerSat;
  }

  if (Autothrust_DWork.is_active_c5_Autothrust == 0U) {
    Autothrust_DWork.is_active_c5_Autothrust = 1U;
    Autothrust_DWork.is_c5_Autothrust = Autothrust_IN_OnGround;
    rtb_on_ground = 1;
  } else if (Autothrust_DWork.is_c5_Autothrust == 1) {
    if ((rtb_Saturation > 0.05) || (Phi_rad > 0.05)) {
      Autothrust_DWork.is_c5_Autothrust = Autothrust_IN_OnGround;
      rtb_on_ground = 1;
    } else {
      rtb_on_ground = 0;
    }
  } else if ((rtb_Saturation == 0.0) && (Phi_rad == 0.0)) {
    Autothrust_DWork.is_c5_Autothrust = Autothrust_IN_InAir;
    rtb_on_ground = 0;
  } else {
    rtb_on_ground = 1;
  }

  rtb_Saturation = 15.0 - 0.0019812 * Autothrust_U.in.data.H_ft;
  if (rtb_Saturation <= -56.5) {
    rtb_Saturation = -56.5;
  }

  Phi_rad = (Autothrust_U.in.data.engine_N1_1_percent + Autothrust_U.in.data.commanded_engine_N1_1_percent) -
    Autothrust_U.in.data.corrected_engine_N1_1_percent;
  Theta_rad = (Autothrust_U.in.data.engine_N1_2_percent + Autothrust_U.in.data.commanded_engine_N1_2_percent) -
    Autothrust_U.in.data.corrected_engine_N1_2_percent;
  Autothrust_RateLimiterwithThreshold(Autothrust_U.in.input.thrust_limit_IDLE_percent + look2_binlxpw(static_cast<real_T>
    (Autothrust_U.in.input.is_anti_ice_engine_1_active || Autothrust_U.in.input.is_anti_ice_engine_2_active),
    static_cast<real_T>(Autothrust_U.in.input.is_anti_ice_wing_active), Autothrust_P.uDLookupTable_bp01Data,
    Autothrust_P.uDLookupTable_bp02Data, Autothrust_P.uDLookupTable_tableData, Autothrust_P.uDLookupTable_maxIndex, 2U),
    Autothrust_P.RateLimiterThresholdVariableTs_up, Autothrust_P.RateLimiterThresholdVariableTs_lo,
    Autothrust_U.in.time.dt, Autothrust_P.RateLimiterThresholdVariableTs_InitialCondition,
    Autothrust_P.RateLimiterThresholdVariableTs_Threshold, &rtb_y_e, &Autothrust_DWork.sf_RateLimiterwithThreshold_d);
  Autothrust_RateLimiterwithThreshold(look2_binlxpw(Autothrust_U.in.data.TAT_degC, Autothrust_U.in.data.H_ft,
    Autothrust_P.MaximumClimb_bp01Data, Autothrust_P.MaximumClimb_bp02Data, Autothrust_P.MaximumClimb_tableData,
    Autothrust_P.MaximumClimb_maxIndex, 26U) * look1_binlxpw(Autothrust_U.in.data.V_mach,
    Autothrust_P.ScheduledGain_BreakpointsForDimension1, Autothrust_P.ScheduledGain_Table, 2U),
    Autothrust_P.RateLimiterThresholdVariableTs2_up, Autothrust_P.RateLimiterThresholdVariableTs2_lo,
    Autothrust_U.in.time.dt, Autothrust_P.RateLimiterThresholdVariableTs2_InitialCondition,
    Autothrust_P.RateLimiterThresholdVariableTs2_Threshold, &rtb_y_b4, &Autothrust_DWork.sf_RateLimiterwithThreshold_h);
  rtb_Switch2_k = look2_binlcpw(Autothrust_U.in.data.TAT_degC, Autothrust_U.in.data.H_ft,
    Autothrust_P.OATCornerPoint_bp01Data, Autothrust_P.OATCornerPoint_bp02Data, Autothrust_P.OATCornerPoint_tableData,
    Autothrust_P.OATCornerPoint_maxIndex, 26U);
  Autothrust_RateLimiterwithThreshold((look2_binlxpw(static_cast<real_T>
    (Autothrust_U.in.input.is_anti_ice_engine_1_active || Autothrust_U.in.input.is_anti_ice_engine_2_active),
    rtb_Switch2_k, Autothrust_P.AntiIceEngine_bp01Data, Autothrust_P.AntiIceEngine_bp02Data,
    Autothrust_P.AntiIceEngine_tableData, Autothrust_P.AntiIceEngine_maxIndex, 2U) + look2_binlxpw(static_cast<real_T>
    (Autothrust_U.in.input.is_anti_ice_wing_active), rtb_Switch2_k, Autothrust_P.AntiIceWing_bp01Data,
    Autothrust_P.AntiIceWing_bp02Data, Autothrust_P.AntiIceWing_tableData, Autothrust_P.AntiIceWing_maxIndex, 2U)) +
    look2_binlxpw(static_cast<real_T>(Autothrust_U.in.input.is_air_conditioning_1_active ||
    Autothrust_U.in.input.is_air_conditioning_2_active), rtb_Switch2_k, Autothrust_P.AirConditioning_bp01Data,
                  Autothrust_P.AirConditioning_bp02Data, Autothrust_P.AirConditioning_tableData,
                  Autothrust_P.AirConditioning_maxIndex, 2U), Autothrust_P.RateLimiterThresholdVariableTs1_up,
    Autothrust_P.RateLimiterThresholdVariableTs1_lo, Autothrust_U.in.time.dt,
    Autothrust_P.RateLimiterThresholdVariableTs1_InitialCondition,
    Autothrust_P.RateLimiterThresholdVariableTs1_Threshold, &rtb_Switch_m, &Autothrust_DWork.sf_RateLimiterwithThreshold);
  rtb_Sum_ew = rtb_y_b4 + rtb_Switch_m;
  Autothrust_RateLimiterwithThreshold((look1_binlxpw(static_cast<real_T>
    (Autothrust_U.in.input.is_anti_ice_engine_1_active || Autothrust_U.in.input.is_anti_ice_engine_2_active),
    Autothrust_P.AntiIceEngine_bp01Data_d, Autothrust_P.AntiIceEngine_tableData_l, 1U) + look1_binlxpw
    (static_cast<real_T>(Autothrust_U.in.input.is_anti_ice_wing_active), Autothrust_P.AntiIceWing_bp01Data_n,
     Autothrust_P.AntiIceWing_tableData_g, 1U)) + look1_binlxpw(static_cast<real_T>
    (Autothrust_U.in.input.is_air_conditioning_1_active || Autothrust_U.in.input.is_air_conditioning_2_active),
    Autothrust_P.AirConditioning_bp01Data_d, Autothrust_P.AirConditioning_tableData_p, 1U),
    Autothrust_P.RateLimiterThresholdVariableTs_up_d, Autothrust_P.RateLimiterThresholdVariableTs_lo_k,
    Autothrust_U.in.time.dt, Autothrust_P.RateLimiterThresholdVariableTs_InitialCondition_p,
    Autothrust_P.RateLimiterThresholdVariableTs_Threshold_b, &rtb_y_b4, &Autothrust_DWork.sf_RateLimiterwithThreshold_m);
  if (Autothrust_U.in.input.flex_temperature_degC < rtb_Saturation + 55.0) {
    rtb_Max_g = Autothrust_U.in.input.flex_temperature_degC;
  } else {
    rtb_Max_g = rtb_Saturation + 55.0;
  }

  if (rtb_Max_g <= rtb_Saturation + 29.0) {
    rtb_Max_g = rtb_Saturation + 29.0;
  }

  if (rtb_Max_g <= Autothrust_U.in.data.OAT_degC) {
    rtb_Max_g = Autothrust_U.in.data.OAT_degC;
  }

  rtb_Max_g = look2_binlxpw(look2_binlxpw(Autothrust_U.in.data.H_ft, rtb_Max_g, Autothrust_P.Right_bp01Data,
    Autothrust_P.Right_bp02Data, Autothrust_P.Right_tableData, Autothrust_P.Right_maxIndex, 10U),
    Autothrust_U.in.data.TAT_degC, Autothrust_P.Left_bp01Data, Autothrust_P.Left_bp02Data, Autothrust_P.Left_tableData,
    Autothrust_P.Left_maxIndex, 2U) + rtb_y_b4;
  if (rtb_Max_g <= rtb_Sum_ew) {
    rtb_Max_g = rtb_Sum_ew;
  }

  Autothrust_RateLimiterwithThreshold(look2_binlxpw(Autothrust_U.in.data.TAT_degC, Autothrust_U.in.data.H_ft,
    Autothrust_P.MaximumContinuous_bp01Data, Autothrust_P.MaximumContinuous_bp02Data,
    Autothrust_P.MaximumContinuous_tableData, Autothrust_P.MaximumContinuous_maxIndex, 26U),
    Autothrust_P.RateLimiterThresholdVariableTs2_up_j, Autothrust_P.RateLimiterThresholdVariableTs2_lo_b,
    Autothrust_U.in.time.dt, Autothrust_P.RateLimiterThresholdVariableTs2_InitialCondition_e,
    Autothrust_P.RateLimiterThresholdVariableTs2_Threshold_m, &rtb_y_b4,
    &Autothrust_DWork.sf_RateLimiterwithThreshold_ma);
  rtb_Switch1_k = look2_binlcpw(Autothrust_U.in.data.TAT_degC, Autothrust_U.in.data.H_ft,
    Autothrust_P.OATCornerPoint_bp01Data_a, Autothrust_P.OATCornerPoint_bp02Data_i,
    Autothrust_P.OATCornerPoint_tableData_n, Autothrust_P.OATCornerPoint_maxIndex_i, 26U);
  rtb_Switch2_k = (Autothrust_U.in.input.is_air_conditioning_1_active ||
                   Autothrust_U.in.input.is_air_conditioning_2_active);
  rtb_Switch_m = look2_binlxpw(rtb_Switch2_k, rtb_Switch1_k, Autothrust_P.AirConditioning_bp01Data_l,
    Autothrust_P.AirConditioning_bp02Data_c, Autothrust_P.AirConditioning_tableData_l,
    Autothrust_P.AirConditioning_maxIndex_g, 2U);
  Autothrust_RateLimiterwithThreshold((look2_binlxpw(static_cast<real_T>
    (Autothrust_U.in.input.is_anti_ice_engine_1_active || Autothrust_U.in.input.is_anti_ice_engine_2_active),
    rtb_Switch1_k, Autothrust_P.AntiIceEngine_bp01Data_l, Autothrust_P.AntiIceEngine_bp02Data_e,
    Autothrust_P.AntiIceEngine_tableData_d, Autothrust_P.AntiIceEngine_maxIndex_e, 2U) + look2_binlxpw
    (static_cast<real_T>(Autothrust_U.in.input.is_anti_ice_wing_active), rtb_Switch1_k,
     Autothrust_P.AntiIceWing_bp01Data_b, Autothrust_P.AntiIceWing_bp02Data_n, Autothrust_P.AntiIceWing_tableData_a,
     Autothrust_P.AntiIceWing_maxIndex_d, 2U)) + rtb_Switch_m, Autothrust_P.RateLimiterThresholdVariableTs1_up_j,
    Autothrust_P.RateLimiterThresholdVariableTs1_lo_d, Autothrust_U.in.time.dt,
    Autothrust_P.RateLimiterThresholdVariableTs1_InitialCondition_c,
    Autothrust_P.RateLimiterThresholdVariableTs1_Threshold_e, &rtb_Switch_m,
    &Autothrust_DWork.sf_RateLimiterwithThreshold_o);
  rtb_Switch_m += rtb_y_b4;
  Autothrust_RateLimiterwithThreshold(look2_binlxpw(Autothrust_U.in.data.TAT_degC, Autothrust_U.in.data.H_ft,
    Autothrust_P.MaximumTakeOff_bp01Data, Autothrust_P.MaximumTakeOff_bp02Data, Autothrust_P.MaximumTakeOff_tableData,
    Autothrust_P.MaximumTakeOff_maxIndex, 36U), Autothrust_P.RateLimiterThresholdVariableTs2_up_g,
    Autothrust_P.RateLimiterThresholdVariableTs2_lo_e, Autothrust_U.in.time.dt,
    Autothrust_P.RateLimiterThresholdVariableTs2_InitialCondition_b,
    Autothrust_P.RateLimiterThresholdVariableTs2_Threshold_i, &rtb_y_b4, &Autothrust_DWork.sf_RateLimiterwithThreshold_n);
  rtb_OATCornerPoint = look2_binlcpw(Autothrust_U.in.data.TAT_degC, Autothrust_U.in.data.H_ft,
    Autothrust_P.OATCornerPoint_bp01Data_j, Autothrust_P.OATCornerPoint_bp02Data_g,
    Autothrust_P.OATCornerPoint_tableData_f, Autothrust_P.OATCornerPoint_maxIndex_m, 36U);
  if (Autothrust_U.in.data.H_ft <= Autothrust_P.CompareToConstant_const) {
    rtb_Switch_dx = look2_binlxpw(static_cast<real_T>(Autothrust_U.in.input.is_anti_ice_engine_1_active ||
      Autothrust_U.in.input.is_anti_ice_engine_2_active), rtb_OATCornerPoint, Autothrust_P.AntiIceEngine8000_bp01Data,
      Autothrust_P.AntiIceEngine8000_bp02Data, Autothrust_P.AntiIceEngine8000_tableData,
      Autothrust_P.AntiIceEngine8000_maxIndex, 2U);
    rtb_Switch1_k = look2_binlxpw(static_cast<real_T>(Autothrust_U.in.input.is_anti_ice_wing_active), rtb_OATCornerPoint,
      Autothrust_P.AntiIceWing8000_bp01Data, Autothrust_P.AntiIceWing8000_bp02Data,
      Autothrust_P.AntiIceWing8000_tableData, Autothrust_P.AntiIceWing8000_maxIndex, 2U);
    rtb_Switch2_k = look2_binlxpw(rtb_Switch2_k, rtb_OATCornerPoint, Autothrust_P.AirConditioning8000_bp01Data,
      Autothrust_P.AirConditioning8000_bp02Data, Autothrust_P.AirConditioning8000_tableData,
      Autothrust_P.AirConditioning8000_maxIndex, 2U);
  } else {
    rtb_Switch_dx = look2_binlxpw(static_cast<real_T>(Autothrust_U.in.input.is_anti_ice_engine_1_active ||
      Autothrust_U.in.input.is_anti_ice_engine_2_active), rtb_OATCornerPoint, Autothrust_P.AntiIceEngine8000_bp01Data_m,
      Autothrust_P.AntiIceEngine8000_bp02Data_i, Autothrust_P.AntiIceEngine8000_tableData_d,
      Autothrust_P.AntiIceEngine8000_maxIndex_a, 2U);
    rtb_Switch1_k = look2_binlxpw(static_cast<real_T>(Autothrust_U.in.input.is_anti_ice_wing_active), rtb_OATCornerPoint,
      Autothrust_P.AntiIceWing8000_bp01Data_d, Autothrust_P.AntiIceWing8000_bp02Data_e,
      Autothrust_P.AntiIceWing8000_tableData_k, Autothrust_P.AntiIceWing8000_maxIndex_c, 2U);
    rtb_Switch2_k = look2_binlxpw(rtb_Switch2_k, rtb_OATCornerPoint, Autothrust_P.AirConditioning8000_bp01Data_p,
      Autothrust_P.AirConditioning8000_bp02Data_l, Autothrust_P.AirConditioning8000_tableData_f,
      Autothrust_P.AirConditioning8000_maxIndex_o, 2U);
  }

  Autothrust_RateLimiterwithThreshold((rtb_Switch_dx + rtb_Switch1_k) + rtb_Switch2_k,
    Autothrust_P.RateLimiterThresholdVariableTs1_up_m, Autothrust_P.RateLimiterThresholdVariableTs1_lo_a,
    Autothrust_U.in.time.dt, Autothrust_P.RateLimiterThresholdVariableTs1_InitialCondition_m,
    Autothrust_P.RateLimiterThresholdVariableTs1_Threshold_n, &rtb_Switch_dx,
    &Autothrust_DWork.sf_RateLimiterwithThreshold_k);
  rtb_Switch1_k = rtb_y_b4 + rtb_Switch_dx;
  rtb_Switch2_k = rtb_y_e;
  Autothrust_TimeSinceCondition(Autothrust_U.in.time.simulation_time, Autothrust_U.in.input.ATHR_disconnect, &rtb_y_e,
    &Autothrust_DWork.sf_TimeSinceCondition_o);
  Autothrust_DWork.Memory_PreviousInput = Autothrust_P.Logic_table[(((static_cast<uint32_T>(rtb_y_e >=
    Autothrust_P.CompareToConstant_const_k) << 1) + Autothrust_U.in.input.ATHR_reset_disable) << 1) +
    Autothrust_DWork.Memory_PreviousInput];
  if (!Autothrust_DWork.eventTime_not_empty_gf) {
    Autothrust_DWork.eventTime_i = Autothrust_U.in.time.simulation_time;
    Autothrust_DWork.eventTime_not_empty_gf = true;
  }

  if ((Autothrust_U.in.input.ATHR_push != Autothrust_P.CompareToConstant1_const) || (Autothrust_DWork.eventTime_i == 0.0))
  {
    Autothrust_DWork.eventTime_i = Autothrust_U.in.time.simulation_time;
  }

  Autothrust_DWork.Memory_PreviousInput_m = Autothrust_P.Logic_table_m[(((Autothrust_U.in.time.simulation_time -
    Autothrust_DWork.eventTime_i >= Autothrust_P.CompareToConstant2_const) + (static_cast<uint32_T>
    (Autothrust_DWork.Delay_DSTATE_a) << 1)) << 1) + Autothrust_DWork.Memory_PreviousInput_m];
  if (Autothrust_U.in.data.is_engine_operative_1 && Autothrust_U.in.data.is_engine_operative_2) {
    rtb_out = ((Autothrust_U.in.input.TLA_1_deg >= 0.0) && (Autothrust_U.in.input.TLA_1_deg <= 25.0) &&
               (Autothrust_U.in.input.TLA_2_deg >= 0.0) && (Autothrust_U.in.input.TLA_2_deg <= 25.0));
  } else {
    rtb_out = ((Autothrust_U.in.data.is_engine_operative_1 && (Autothrust_U.in.input.TLA_1_deg >= 0.0) &&
                (Autothrust_U.in.input.TLA_1_deg <= 35.0)) || (Autothrust_U.in.data.is_engine_operative_2 &&
                (Autothrust_U.in.input.TLA_2_deg >= 0.0) && (Autothrust_U.in.input.TLA_2_deg <= 35.0)));
  }

  rtb_Compare_e = ((Autothrust_U.in.input.flex_temperature_degC > Autothrust_U.in.data.TAT_degC) &&
                   (Autothrust_U.in.input.flex_temperature_degC != 0.0) && (Autothrust_U.in.input.flight_phase < 3.0));
  Autothrust_DWork.latch = ((rtb_Compare_e && (rtb_on_ground != 0) && (Autothrust_U.in.input.TLA_1_deg == 35.0) &&
    (Autothrust_U.in.input.TLA_2_deg == 35.0)) || Autothrust_DWork.latch);
  Autothrust_DWork.latch = (((!Autothrust_DWork.latch) || (((Autothrust_U.in.input.TLA_1_deg != 25.0) ||
    (Autothrust_U.in.input.TLA_2_deg != 25.0)) && ((Autothrust_U.in.input.TLA_1_deg != 45.0) ||
    (Autothrust_U.in.input.TLA_2_deg != 45.0)))) && Autothrust_DWork.latch);
  rtb_y_k = ((rtb_Compare_e && (rtb_on_ground != 0)) || ((rtb_on_ground == 0) && Autothrust_DWork.latch));
  Autothrust_DWork.Delay_DSTATE_a = (static_cast<int32_T>(Autothrust_U.in.input.ATHR_push) > static_cast<int32_T>
    (Autothrust_P.CompareToConstant_const_j));
  rtb_NOT1_m = (Autothrust_DWork.Delay_DSTATE_a && (!Autothrust_DWork.Memory_PreviousInput_m));
  Autothrust_TimeSinceCondition(Autothrust_U.in.time.simulation_time, rtb_on_ground != 0, &rtb_y_e,
    &Autothrust_DWork.sf_TimeSinceCondition1);
  if (!Autothrust_DWork.eventTime_not_empty_f) {
    Autothrust_DWork.eventTime_b = Autothrust_U.in.time.simulation_time;
    Autothrust_DWork.eventTime_not_empty_f = true;
  }

  tmp = !Autothrust_U.in.input.is_TCAS_active;
  if (tmp || (Autothrust_DWork.eventTime_b == 0.0)) {
    Autothrust_DWork.eventTime_b = Autothrust_U.in.time.simulation_time;
  }

  Autothrust_DWork.sInhibit = (((!Autothrust_DWork.prev_TCAS_active) && Autothrust_U.in.input.is_TCAS_active &&
    (Autothrust_U.in.input.TLA_1_deg <= 25.0) && (Autothrust_U.in.input.TLA_2_deg <= 25.0)) || Autothrust_DWork.sInhibit);
  Autothrust_DWork.sInhibit = (((!Autothrust_DWork.sInhibit) || (Autothrust_U.in.time.simulation_time -
    Autothrust_DWork.eventTime_b <= 5.0) || ((Autothrust_U.in.input.TLA_1_deg >= 25.0) &&
    (Autothrust_U.in.input.TLA_2_deg >= 25.0))) && Autothrust_DWork.sInhibit);
  Autothrust_DWork.sInhibit = (Autothrust_U.in.input.is_TCAS_active && Autothrust_DWork.sInhibit);
  Autothrust_DWork.prev_TCAS_active = Autothrust_U.in.input.is_TCAS_active;
  rtb_BusAssignment_n.time = Autothrust_U.in.time;
  rtb_BusAssignment_n.data.nz_g = Autothrust_U.in.data.nz_g;
  rtb_BusAssignment_n.data.Theta_deg = rtb_Gain2;
  rtb_BusAssignment_n.data.Phi_deg = rtb_Gain3;
  rtb_BusAssignment_n.data.V_ias_kn = Autothrust_U.in.data.V_ias_kn;
  rtb_BusAssignment_n.data.V_tas_kn = Autothrust_U.in.data.V_tas_kn;
  rtb_BusAssignment_n.data.V_mach = Autothrust_U.in.data.V_mach;
  rtb_BusAssignment_n.data.V_gnd_kn = Autothrust_U.in.data.V_gnd_kn;
  rtb_BusAssignment_n.data.alpha_deg = Autothrust_U.in.data.alpha_deg;
  rtb_BusAssignment_n.data.H_ft = Autothrust_U.in.data.H_ft;
  rtb_BusAssignment_n.data.H_ind_ft = Autothrust_U.in.data.H_ind_ft;
  rtb_BusAssignment_n.data.H_radio_ft = Autothrust_U.in.data.H_radio_ft;
  rtb_BusAssignment_n.data.H_dot_fpm = Autothrust_U.in.data.H_dot_fpm;
  rtb_BusAssignment_n.data.ax_m_s2 = result[0];
  rtb_BusAssignment_n.data.ay_m_s2 = result[1];
  rtb_BusAssignment_n.data.az_m_s2 = result[2];
  rtb_BusAssignment_n.data.bx_m_s2 = Autothrust_U.in.data.bx_m_s2;
  rtb_BusAssignment_n.data.by_m_s2 = Autothrust_U.in.data.by_m_s2;
  rtb_BusAssignment_n.data.bz_m_s2 = Autothrust_U.in.data.bz_m_s2;
  rtb_BusAssignment_n.data.on_ground = (rtb_on_ground != 0);
  rtb_BusAssignment_n.data.flap_handle_index = Autothrust_U.in.data.flap_handle_index;
  rtb_BusAssignment_n.data.is_engine_operative_1 = Autothrust_U.in.data.is_engine_operative_1;
  rtb_BusAssignment_n.data.is_engine_operative_2 = Autothrust_U.in.data.is_engine_operative_2;
  rtb_BusAssignment_n.data.commanded_engine_N1_1_percent = Phi_rad;
  rtb_BusAssignment_n.data.commanded_engine_N1_2_percent = Theta_rad;
  rtb_BusAssignment_n.data.engine_N1_1_percent = Autothrust_U.in.data.engine_N1_1_percent;
  rtb_BusAssignment_n.data.engine_N1_2_percent = Autothrust_U.in.data.engine_N1_2_percent;
  rtb_BusAssignment_n.data.TAT_degC = Autothrust_U.in.data.TAT_degC;
  rtb_BusAssignment_n.data.OAT_degC = Autothrust_U.in.data.OAT_degC;
  rtb_BusAssignment_n.data.ISA_degC = rtb_Saturation;
  rtb_BusAssignment_n.input.ATHR_push = Autothrust_U.in.input.ATHR_push;
  rtb_BusAssignment_n.input.ATHR_disconnect = Autothrust_U.in.input.ATHR_disconnect;
  rtb_BusAssignment_n.input.TLA_1_deg = Autothrust_U.in.input.TLA_1_deg;
  rtb_BusAssignment_n.input.TLA_2_deg = Autothrust_U.in.input.TLA_2_deg;
  rtb_BusAssignment_n.input.V_c_kn = Autothrust_U.in.input.V_c_kn;
  rtb_BusAssignment_n.input.V_LS_kn = Autothrust_U.in.input.V_LS_kn;
  rtb_BusAssignment_n.input.V_MAX_kn = Autothrust_U.in.input.V_MAX_kn;
  rtb_BusAssignment_n.input.thrust_limit_REV_percent = Autothrust_U.in.input.thrust_limit_REV_percent;
  rtb_BusAssignment_n.input.thrust_limit_IDLE_percent = rtb_Switch2_k;
  rtb_BusAssignment_n.input.thrust_limit_CLB_percent = rtb_Sum_ew;
  rtb_BusAssignment_n.input.thrust_limit_MCT_percent = rtb_Switch_m;
  rtb_BusAssignment_n.input.thrust_limit_FLEX_percent = rtb_Max_g;
  rtb_BusAssignment_n.input.thrust_limit_TOGA_percent = rtb_Switch1_k;
  rtb_BusAssignment_n.input.flex_temperature_degC = Autothrust_U.in.input.flex_temperature_degC;
  rtb_BusAssignment_n.input.mode_requested = Autothrust_U.in.input.mode_requested;
  rtb_BusAssignment_n.input.is_mach_mode_active = Autothrust_U.in.input.is_mach_mode_active;
  rtb_BusAssignment_n.input.alpha_floor_condition = Autothrust_U.in.input.alpha_floor_condition;
  rtb_BusAssignment_n.input.is_approach_mode_active = Autothrust_U.in.input.is_approach_mode_active;
  rtb_BusAssignment_n.input.is_SRS_TO_mode_active = Autothrust_U.in.input.is_SRS_TO_mode_active;
  rtb_BusAssignment_n.input.is_SRS_GA_mode_active = Autothrust_U.in.input.is_SRS_GA_mode_active;
  rtb_BusAssignment_n.input.is_LAND_mode_active = Autothrust_U.in.input.is_LAND_mode_active;
  rtb_BusAssignment_n.input.thrust_reduction_altitude = Autothrust_U.in.input.thrust_reduction_altitude;
  rtb_BusAssignment_n.input.thrust_reduction_altitude_go_around =
    Autothrust_U.in.input.thrust_reduction_altitude_go_around;
  rtb_BusAssignment_n.input.flight_phase = Autothrust_U.in.input.flight_phase;
  rtb_BusAssignment_n.input.is_alt_soft_mode_active = Autothrust_U.in.input.is_alt_soft_mode_active;
  rtb_BusAssignment_n.input.is_anti_ice_wing_active = Autothrust_U.in.input.is_anti_ice_wing_active;
  rtb_BusAssignment_n.input.is_anti_ice_engine_1_active = Autothrust_U.in.input.is_anti_ice_engine_1_active;
  rtb_BusAssignment_n.input.is_anti_ice_engine_2_active = Autothrust_U.in.input.is_anti_ice_engine_2_active;
  rtb_BusAssignment_n.input.is_air_conditioning_1_active = Autothrust_U.in.input.is_air_conditioning_1_active;
  rtb_BusAssignment_n.input.is_air_conditioning_2_active = Autothrust_U.in.input.is_air_conditioning_2_active;
  rtb_BusAssignment_n.input.FD_active = Autothrust_U.in.input.FD_active;
  rtb_BusAssignment_n.input.ATHR_reset_disable = Autothrust_U.in.input.ATHR_reset_disable;
  rtb_BusAssignment_n.input.is_TCAS_active = Autothrust_U.in.input.is_TCAS_active;
  rtb_BusAssignment_n.input.target_TCAS_RA_rate_fpm = Autothrust_U.in.input.target_TCAS_RA_rate_fpm;
  rtb_BusAssignment_n.output = Autothrust_P.athr_out_MATLABStruct.output;
  if (Autothrust_U.in.data.is_engine_operative_1 && Autothrust_U.in.data.is_engine_operative_2) {
    rtb_BusAssignment_n.data_computed.TLA_in_active_range = ((Autothrust_U.in.input.TLA_1_deg >= 0.0) &&
      (Autothrust_U.in.input.TLA_1_deg <= 25.0) && (Autothrust_U.in.input.TLA_2_deg >= 0.0) &&
      (Autothrust_U.in.input.TLA_2_deg <= 25.0));
  } else {
    rtb_BusAssignment_n.data_computed.TLA_in_active_range = ((Autothrust_U.in.data.is_engine_operative_1 &&
      (Autothrust_U.in.input.TLA_1_deg >= 0.0) && (Autothrust_U.in.input.TLA_1_deg <= 35.0)) ||
      (Autothrust_U.in.data.is_engine_operative_2 && (Autothrust_U.in.input.TLA_2_deg >= 0.0) &&
       (Autothrust_U.in.input.TLA_2_deg <= 35.0)));
  }

  rtb_BusAssignment_n.data_computed.is_FLX_active = ((rtb_Compare_e && (rtb_on_ground != 0)) || ((rtb_on_ground == 0) &&
    Autothrust_DWork.latch));
  rtb_BusAssignment_n.data_computed.ATHR_push = rtb_NOT1_m;
  rtb_BusAssignment_n.data_computed.ATHR_disabled = Autothrust_DWork.Memory_PreviousInput;
  rtb_BusAssignment_n.data_computed.time_since_touchdown = rtb_y_e;
  rtb_BusAssignment_n.data_computed.alpha_floor_inhibited = Autothrust_DWork.sInhibit;
  Autothrust_TLAComputation1(&rtb_BusAssignment_n, Autothrust_U.in.input.TLA_1_deg, &rtb_y_b4, &rtb_Compare_e);
  Autothrust_TLAComputation1(&rtb_BusAssignment_n, Autothrust_U.in.input.TLA_2_deg, &rtb_y_e, &rtb_NOT1_m);
  if (!Autothrust_DWork.prev_TLA_1_not_empty) {
    Autothrust_DWork.prev_TLA_1 = Autothrust_U.in.input.TLA_1_deg;
    Autothrust_DWork.prev_TLA_1_not_empty = true;
  }

  if (!Autothrust_DWork.prev_TLA_2_not_empty) {
    Autothrust_DWork.prev_TLA_2 = Autothrust_U.in.input.TLA_2_deg;
    Autothrust_DWork.prev_TLA_2_not_empty = true;
  }

  condition_AP_FD_ATHR_Specific = !Autothrust_DWork.Memory_PreviousInput;
  condition02 = (Autothrust_U.in.input.is_SRS_TO_mode_active || Autothrust_U.in.input.is_SRS_GA_mode_active);
  Autothrust_DWork.condition_TOGA_latch = (((!Autothrust_DWork.prev_SRS_TO_GA_mode_active) && condition02) ||
    Autothrust_DWork.condition_TOGA_latch);
  if (!Autothrust_DWork.eventTime_not_empty_g) {
    Autothrust_DWork.eventTime_o = Autothrust_U.in.time.simulation_time;
    Autothrust_DWork.eventTime_not_empty_g = true;
  }

  if ((!Autothrust_DWork.condition_TOGA_latch) || (Autothrust_DWork.eventTime_o == 0.0)) {
    Autothrust_DWork.eventTime_o = Autothrust_U.in.time.simulation_time;
  }

  if (Autothrust_U.in.time.simulation_time - Autothrust_DWork.eventTime_o >= 0.3) {
    condition_TOGA = true;
    Autothrust_DWork.condition_TOGA_latch = false;
  } else {
    condition_TOGA = false;
  }

  condition_AlphaFloor_tmp = !Autothrust_DWork.sInhibit;
  condition_AlphaFloor = (condition_AlphaFloor_tmp && Autothrust_U.in.input.alpha_floor_condition &&
    (!Autothrust_DWork.prev_condition_AlphaFloor));
  ATHR_ENGAGED_tmp = !Autothrust_DWork.ATHR_ENGAGED;
  ATHR_ENGAGED_tmp_0 = ((Autothrust_DWork.prev_TLA_1 <= 0.0) || (Autothrust_U.in.input.TLA_1_deg != 0.0));
  ATHR_ENGAGED_tmp_1 = ((Autothrust_DWork.prev_TLA_2 <= 0.0) || (Autothrust_U.in.input.TLA_2_deg != 0.0));
  rtb_NOT = !Autothrust_U.in.input.ATHR_disconnect;
  Autothrust_DWork.ATHR_ENGAGED = ((condition_AP_FD_ATHR_Specific && (((rtb_on_ground == 0) && ATHR_ENGAGED_tmp &&
    rtb_BusAssignment_n.data_computed.ATHR_push) || condition_TOGA || condition_AlphaFloor ||
    (Autothrust_U.in.input.is_TCAS_active && (!Autothrust_DWork.prev_condition_TCAS)))) ||
    (condition_AP_FD_ATHR_Specific && ((!rtb_BusAssignment_n.data_computed.ATHR_push) || ATHR_ENGAGED_tmp ||
    Autothrust_U.in.input.is_LAND_mode_active) && rtb_NOT && ((ATHR_ENGAGED_tmp_0 || ATHR_ENGAGED_tmp_1) &&
    (ATHR_ENGAGED_tmp_0 || (Autothrust_U.in.input.TLA_2_deg != 0.0)) && (ATHR_ENGAGED_tmp_1 ||
    (Autothrust_U.in.input.TLA_1_deg != 0.0))) && Autothrust_DWork.ATHR_ENGAGED));
  condition_AP_FD_ATHR_Specific = (Autothrust_DWork.ATHR_ENGAGED && (rtb_out || condition_AlphaFloor));
  if (Autothrust_DWork.ATHR_ENGAGED && condition_AP_FD_ATHR_Specific) {
    status = athr_status_ENGAGED_ACTIVE;
  } else if (Autothrust_DWork.ATHR_ENGAGED && (!condition_AP_FD_ATHR_Specific)) {
    status = athr_status_ENGAGED_ARMED;
  } else {
    status = athr_status_DISENGAGED;
  }

  Autothrust_DWork.prev_TLA_1 = Autothrust_U.in.input.TLA_1_deg;
  Autothrust_DWork.prev_TLA_2 = Autothrust_U.in.input.TLA_2_deg;
  Autothrust_DWork.prev_condition_AlphaFloor = (Autothrust_U.in.input.alpha_floor_condition && condition_AlphaFloor_tmp);
  Autothrust_DWork.prev_condition_TCAS = Autothrust_U.in.input.is_TCAS_active;
  Autothrust_DWork.prev_SRS_TO_GA_mode_active = condition02;
  if (Autothrust_U.in.input.TLA_1_deg > Autothrust_U.in.input.TLA_2_deg) {
    rtb_OATCornerPoint = Autothrust_U.in.input.TLA_1_deg;
  } else {
    rtb_OATCornerPoint = Autothrust_U.in.input.TLA_2_deg;
  }

  Autothrust_DWork.pConditionAlphaFloor = (Autothrust_DWork.prev_condition_AlphaFloor || ((!(status ==
    athr_status_DISENGAGED)) && Autothrust_DWork.pConditionAlphaFloor));
  if (status == athr_status_DISENGAGED) {
    Autothrust_DWork.pMode = athr_mode_NONE;
  } else if (Autothrust_DWork.prev_condition_AlphaFloor) {
    Autothrust_DWork.pMode = athr_mode_A_FLOOR;
  } else if (Autothrust_DWork.pConditionAlphaFloor) {
    Autothrust_DWork.pMode = athr_mode_TOGA_LK;
  } else if ((status == athr_status_ENGAGED_ARMED) && ((Autothrust_U.in.input.TLA_1_deg == 45.0) ||
              (Autothrust_U.in.input.TLA_2_deg == 45.0))) {
    Autothrust_DWork.pMode = athr_mode_MAN_TOGA;
  } else if ((status == athr_status_ENGAGED_ARMED) && rtb_y_k && (rtb_OATCornerPoint == 35.0)) {
    Autothrust_DWork.pMode = athr_mode_MAN_FLEX;
  } else if ((status == athr_status_ENGAGED_ARMED) && ((Autothrust_U.in.input.TLA_1_deg == 35.0) ||
              (Autothrust_U.in.input.TLA_2_deg == 35.0))) {
    Autothrust_DWork.pMode = athr_mode_MAN_MCT;
  } else {
    condition_AP_FD_ATHR_Specific = (Autothrust_U.in.data.is_engine_operative_1 &&
      (!Autothrust_U.in.data.is_engine_operative_2));
    condition_AlphaFloor_tmp = (Autothrust_U.in.data.is_engine_operative_2 &&
      (!Autothrust_U.in.data.is_engine_operative_1));
    if ((status == athr_status_ENGAGED_ACTIVE) && (Autothrust_U.in.input.mode_requested == 3.0) &&
        ((condition_AP_FD_ATHR_Specific && (Autothrust_U.in.input.TLA_1_deg == 35.0) && (Autothrust_U.in.input.TLA_2_deg
           <= 35.0)) || (condition_AlphaFloor_tmp && (Autothrust_U.in.input.TLA_2_deg == 35.0) &&
                         (Autothrust_U.in.input.TLA_1_deg <= 35.0)))) {
      Autothrust_DWork.pMode = athr_mode_THR_MCT;
    } else if ((status == athr_status_ENGAGED_ACTIVE) && (Autothrust_U.in.input.mode_requested == 3.0) &&
               Autothrust_U.in.data.is_engine_operative_1 && Autothrust_U.in.data.is_engine_operative_2 &&
               (rtb_OATCornerPoint == 25.0)) {
      Autothrust_DWork.pMode = athr_mode_THR_CLB;
    } else {
      condition_TOGA = (Autothrust_U.in.data.is_engine_operative_1 && Autothrust_U.in.data.is_engine_operative_2);
      if ((status == athr_status_ENGAGED_ACTIVE) && (Autothrust_U.in.input.mode_requested == 3.0) && ((condition_TOGA &&
            (Autothrust_U.in.input.TLA_1_deg < 25.0) && (Autothrust_U.in.input.TLA_2_deg < 25.0)) ||
           (condition_AP_FD_ATHR_Specific && (Autothrust_U.in.input.TLA_1_deg < 35.0)) || (condition_AlphaFloor_tmp &&
            (Autothrust_U.in.input.TLA_2_deg < 35.0)))) {
        Autothrust_DWork.pMode = athr_mode_THR_LVR;
      } else if ((status == athr_status_ENGAGED_ARMED) && ((condition_TOGA && (rtb_OATCornerPoint > 25.0) &&
                   (rtb_OATCornerPoint < 35.0)) || ((rtb_OATCornerPoint > 35.0) && (rtb_OATCornerPoint < 45.0)))) {
        Autothrust_DWork.pMode = athr_mode_MAN_THR;
      } else if ((status == athr_status_ENGAGED_ACTIVE) && (Autothrust_U.in.input.mode_requested == 2.0)) {
        Autothrust_DWork.pMode = athr_mode_THR_IDLE;
      } else if ((status == athr_status_ENGAGED_ACTIVE) && (Autothrust_U.in.input.mode_requested == 1.0) &&
                 (!Autothrust_U.in.input.is_mach_mode_active)) {
        Autothrust_DWork.pMode = athr_mode_SPEED;
      } else if ((status == athr_status_ENGAGED_ACTIVE) && (Autothrust_U.in.input.mode_requested == 1.0) &&
                 Autothrust_U.in.input.is_mach_mode_active) {
        Autothrust_DWork.pMode = athr_mode_MACH;
      }
    }
  }

  if (Autothrust_timeSinceConditionArmedActive(Autothrust_U.in.time.simulation_time, status) >= 0.1) {
    mode = Autothrust_DWork.pMode;
  } else {
    mode = athr_mode_NONE;
  }

  Autothrust_DWork.inhibitAboveThrustReductionAltitude = ((Autothrust_U.in.input.is_SRS_TO_mode_active &&
    (!Autothrust_DWork.was_SRS_TO_active) && (Autothrust_U.in.data.H_ind_ft >
    Autothrust_U.in.input.thrust_reduction_altitude)) || (Autothrust_U.in.input.is_SRS_GA_mode_active &&
    (!Autothrust_DWork.was_SRS_GA_active) && (Autothrust_U.in.data.H_ind_ft >
    Autothrust_U.in.input.thrust_reduction_altitude_go_around)) || (condition02 &&
    Autothrust_DWork.inhibitAboveThrustReductionAltitude));
  condition_TOGA = (Autothrust_U.in.data.is_engine_operative_1 && Autothrust_U.in.data.is_engine_operative_2);
  condition_AlphaFloor = (Autothrust_U.in.data.is_engine_operative_1 && (!Autothrust_U.in.data.is_engine_operative_2));
  ATHR_ENGAGED_tmp = (Autothrust_U.in.data.is_engine_operative_2 && (!Autothrust_U.in.data.is_engine_operative_1));
  Autothrust_DWork.condition_THR_LK = (((status == athr_status_DISENGAGED) && (Autothrust_DWork.pStatus !=
    athr_status_DISENGAGED) && rtb_NOT && ((condition_TOGA && (Autothrust_U.in.input.TLA_1_deg == 25.0) &&
    (Autothrust_U.in.input.TLA_2_deg == 25.0)) || (condition_AlphaFloor && (Autothrust_U.in.input.TLA_1_deg == 35.0) &&
    (Autothrust_U.in.input.TLA_2_deg <= 35.0)) || (ATHR_ENGAGED_tmp && (Autothrust_U.in.input.TLA_2_deg == 35.0) &&
    (Autothrust_U.in.input.TLA_1_deg <= 35.0)))) || (((!Autothrust_DWork.condition_THR_LK) || ((!(status !=
    athr_status_DISENGAGED)) && ((Autothrust_U.in.input.TLA_1_deg == 25.0) || (Autothrust_U.in.input.TLA_2_deg == 25.0) ||
    (Autothrust_U.in.input.TLA_1_deg == 35.0) || (Autothrust_U.in.input.TLA_2_deg == 35.0)))) &&
    Autothrust_DWork.condition_THR_LK));
  Autothrust_DWork.pStatus = status;
  Autothrust_DWork.was_SRS_TO_active = Autothrust_U.in.input.is_SRS_TO_mode_active;
  Autothrust_DWork.was_SRS_GA_active = Autothrust_U.in.input.is_SRS_GA_mode_active;
  if ((rtb_on_ground == 0) && (0.0 > rtb_OATCornerPoint)) {
    rtb_OATCornerPoint = 0.0;
  }

  condition_AP_FD_ATHR_Specific = !Autothrust_U.in.data.is_engine_operative_1;
  condition_AlphaFloor_tmp = !Autothrust_U.in.data.is_engine_operative_2;
  if ((rtb_on_ground == 0) || (condition_AP_FD_ATHR_Specific && condition_AlphaFloor_tmp)) {
    if ((mode == athr_mode_A_FLOOR) || (mode == athr_mode_TOGA_LK)) {
      Autothrust_Y.out.output.thrust_limit_type = athr_thrust_limit_type_TOGA;
      Autothrust_Y.out.output.thrust_limit_percent = rtb_Switch1_k;
    } else if (rtb_OATCornerPoint > 35.0) {
      Autothrust_Y.out.output.thrust_limit_type = athr_thrust_limit_type_TOGA;
      Autothrust_Y.out.output.thrust_limit_percent = rtb_Switch1_k;
    } else if (rtb_OATCornerPoint > 25.0) {
      if (!rtb_y_k) {
        Autothrust_Y.out.output.thrust_limit_type = athr_thrust_limit_type_MCT;
        Autothrust_Y.out.output.thrust_limit_percent = rtb_Switch_m;
      } else {
        Autothrust_Y.out.output.thrust_limit_type = athr_thrust_limit_type_FLEX;
        Autothrust_Y.out.output.thrust_limit_percent = rtb_Max_g;
      }
    } else if (rtb_OATCornerPoint >= 0.0) {
      Autothrust_Y.out.output.thrust_limit_type = athr_thrust_limit_type_CLB;
      Autothrust_Y.out.output.thrust_limit_percent = rtb_Sum_ew;
    } else if (rtb_OATCornerPoint < 0.0) {
      Autothrust_Y.out.output.thrust_limit_type = athr_thrust_limit_type_REVERSE;
      Autothrust_Y.out.output.thrust_limit_percent = Autothrust_U.in.input.thrust_limit_REV_percent;
    } else {
      Autothrust_Y.out.output.thrust_limit_type = athr_thrust_limit_type_NONE;
      Autothrust_Y.out.output.thrust_limit_percent = 0.0;
    }
  } else if (rtb_OATCornerPoint >= 0.0) {
    if ((!rtb_y_k) || (rtb_OATCornerPoint > 35.0)) {
      Autothrust_Y.out.output.thrust_limit_type = athr_thrust_limit_type_TOGA;
      Autothrust_Y.out.output.thrust_limit_percent = rtb_Switch1_k;
    } else {
      Autothrust_Y.out.output.thrust_limit_type = athr_thrust_limit_type_FLEX;
      Autothrust_Y.out.output.thrust_limit_percent = rtb_Max_g;
    }
  } else if (rtb_OATCornerPoint < 0.0) {
    Autothrust_Y.out.output.thrust_limit_type = athr_thrust_limit_type_REVERSE;
    Autothrust_Y.out.output.thrust_limit_percent = Autothrust_U.in.input.thrust_limit_REV_percent;
  } else {
    Autothrust_Y.out.output.thrust_limit_type = athr_thrust_limit_type_NONE;
    Autothrust_Y.out.output.thrust_limit_percent = 0.0;
  }

  if (!Autothrust_DWork.eventTime_not_empty) {
    Autothrust_DWork.eventTime = Autothrust_U.in.time.simulation_time;
    Autothrust_DWork.eventTime_not_empty = true;
  }

  if (((Autothrust_U.in.input.TLA_1_deg != 35.0) && (Autothrust_U.in.input.TLA_2_deg != 35.0)) ||
      (Autothrust_DWork.eventTime == 0.0)) {
    Autothrust_DWork.eventTime = Autothrust_U.in.time.simulation_time;
  }

  if (!Autothrust_DWork.eventTime_not_empty_h) {
    Autothrust_DWork.eventTime_p = Autothrust_U.in.time.simulation_time;
    Autothrust_DWork.eventTime_not_empty_h = true;
  }

  if ((((Autothrust_U.in.input.TLA_1_deg < 25.0) || (Autothrust_U.in.input.TLA_1_deg >= 35.0)) &&
       ((Autothrust_U.in.input.TLA_2_deg < 25.0) || (Autothrust_U.in.input.TLA_2_deg >= 35.0))) ||
      (Autothrust_DWork.eventTime_p == 0.0)) {
    Autothrust_DWork.eventTime_p = Autothrust_U.in.time.simulation_time;
  }

  condition02 = (Autothrust_U.in.time.simulation_time - Autothrust_DWork.eventTime_p >= 4.0);
  ATHR_ENGAGED_tmp_0 = ((status == athr_status_ENGAGED_ACTIVE) && (((Autothrust_U.in.input.TLA_1_deg <= 35.0) &&
    (Autothrust_U.in.input.TLA_2_deg <= 35.0)) || Autothrust_U.in.input.alpha_floor_condition));
  ATHR_ENGAGED_tmp_1 = !ATHR_ENGAGED_tmp_0;
  Autothrust_DWork.pThrustMemoActive = ((((Autothrust_U.in.input.ATHR_push && (status != athr_status_DISENGAGED)) ||
    (ATHR_ENGAGED_tmp_1 && Autothrust_DWork.pUseAutoThrustControl && rtb_NOT)) && ((condition_TOGA &&
    (Autothrust_U.in.input.TLA_1_deg == 25.0) && (Autothrust_U.in.input.TLA_2_deg == 25.0)) || (condition_AlphaFloor &&
    (Autothrust_U.in.input.TLA_1_deg == 35.0) && (Autothrust_U.in.input.TLA_2_deg <= 35.0)) || (ATHR_ENGAGED_tmp &&
    (Autothrust_U.in.input.TLA_2_deg == 35.0) && (Autothrust_U.in.input.TLA_1_deg <= 35.0)))) || (ATHR_ENGAGED_tmp_1 &&
    ((Autothrust_U.in.input.TLA_1_deg == 25.0) || (Autothrust_U.in.input.TLA_1_deg == 35.0) ||
     (Autothrust_U.in.input.TLA_2_deg == 25.0) || (Autothrust_U.in.input.TLA_2_deg == 35.0)) &&
    Autothrust_DWork.pThrustMemoActive));
  Autothrust_DWork.pUseAutoThrustControl = ATHR_ENGAGED_tmp_0;
  rtb_NOT = ((!(status == Autothrust_P.CompareToConstant_const_d)) || ((mode == Autothrust_P.CompareToConstant2_const_c)
              || (mode == Autothrust_P.CompareToConstant3_const_k)));
  if (Autothrust_U.in.data.is_engine_operative_1 && Autothrust_U.in.data.is_engine_operative_2) {
    rtb_Switch_m = rtb_Sum_ew;
  }

  rtb_Switch_dx = Autothrust_P.Gain1_Gain_c * Autothrust_U.in.data.alpha_deg;
  x[0] = Autothrust_U.in.input.V_LS_kn;
  x[1] = Autothrust_U.in.input.V_c_kn;
  x[2] = Autothrust_U.in.input.V_MAX_kn;
  if (Autothrust_U.in.input.V_LS_kn < Autothrust_U.in.input.V_c_kn) {
    if (Autothrust_U.in.input.V_c_kn < Autothrust_U.in.input.V_MAX_kn) {
      i = 1;
    } else if (Autothrust_U.in.input.V_LS_kn < Autothrust_U.in.input.V_MAX_kn) {
      i = 2;
    } else {
      i = 0;
    }
  } else if (Autothrust_U.in.input.V_LS_kn < Autothrust_U.in.input.V_MAX_kn) {
    i = 0;
  } else if (Autothrust_U.in.input.V_c_kn < Autothrust_U.in.input.V_MAX_kn) {
    i = 2;
  } else {
    i = 1;
  }

  rtb_Sum_h = x[i] - Autothrust_U.in.data.V_ias_kn;
  rtb_Switch_dx = (result[2] * std::sin(rtb_Switch_dx) + std::cos(rtb_Switch_dx) * result[0]) * Autothrust_P.Gain_Gain_h
    * Autothrust_P.Gain_Gain_b * look1_binlxpw(static_cast<real_T>(Autothrust_U.in.input.is_approach_mode_active),
    Autothrust_P.ScheduledGain2_BreakpointsForDimension1, Autothrust_P.ScheduledGain2_Table, 1U) + rtb_Sum_h;
  rtb_OATCornerPoint = Autothrust_P.DiscreteDerivativeVariableTs_Gain * rtb_Switch_dx;
  rtb_Divide = (rtb_OATCornerPoint - Autothrust_DWork.Delay_DSTATE) / Autothrust_U.in.time.dt;
  if ((!Autothrust_DWork.pY_not_empty) || (!Autothrust_DWork.pU_not_empty)) {
    Autothrust_DWork.pU = rtb_Divide;
    Autothrust_DWork.pU_not_empty = true;
    Autothrust_DWork.pY = rtb_Divide;
    Autothrust_DWork.pY_not_empty = true;
  }

  denom_tmp = Autothrust_U.in.time.dt * Autothrust_P.LagFilter_C1;
  ca = denom_tmp / (denom_tmp + 2.0);
  Autothrust_DWork.pY = (2.0 - denom_tmp) / (denom_tmp + 2.0) * Autothrust_DWork.pY + (rtb_Divide * ca +
    Autothrust_DWork.pU * ca);
  Autothrust_DWork.pU = rtb_Divide;
  if (!Autothrust_DWork.eventTime_not_empty_a) {
    Autothrust_DWork.eventTime_op = Autothrust_U.in.time.simulation_time;
    Autothrust_DWork.eventTime_not_empty_a = true;
  }

  rtb_Divide = std::abs(rtb_Sum_h);
  if (rtb_Divide > 5.0) {
    Autothrust_DWork.eventTime_op = Autothrust_U.in.time.simulation_time;
  } else if (Autothrust_DWork.eventTime_op == 0.0) {
    Autothrust_DWork.eventTime_op = Autothrust_U.in.time.simulation_time;
  }

  if (Autothrust_U.in.input.mode_requested > Autothrust_P.Saturation_UpperSat_l) {
    denom_tmp = Autothrust_P.Saturation_UpperSat_l;
  } else if (Autothrust_U.in.input.mode_requested < Autothrust_P.Saturation_LowerSat_i) {
    denom_tmp = Autothrust_P.Saturation_LowerSat_i;
  } else {
    denom_tmp = Autothrust_U.in.input.mode_requested;
  }

  switch (static_cast<int32_T>(denom_tmp)) {
   case 0:
    rtb_Switch_dx = Autothrust_P.Constant1_Value;
    break;

   case 1:
    if (10.0 < rtb_Divide) {
      rtb_Divide = 10.0;
    }

    denom_tmp = Autothrust_U.in.time.simulation_time - Autothrust_DWork.eventTime_op;
    if (0.0 > denom_tmp) {
      denom_tmp = 0.0;
    }

    if (1.0 > rtb_Divide) {
      rtb_Divide = 1.0;
    }

    if (15.0 < denom_tmp) {
      denom_tmp = 15.0;
    }

    rtb_Switch_dx = ((1.0 - (rtb_Divide - 1.0) * 0.1111111111111111) * 0.5 * (0.066666666666666666 * denom_tmp) *
                     rtb_Sum_h + (Autothrust_DWork.pY * look1_binlxpw(static_cast<real_T>
      (Autothrust_U.in.input.is_approach_mode_active), Autothrust_P.ScheduledGain1_BreakpointsForDimension1,
      Autothrust_P.ScheduledGain1_Table, 1U) + rtb_Switch_dx * look1_binlxpw(static_cast<real_T>
      (Autothrust_U.in.input.is_approach_mode_active), Autothrust_P.ScheduledGain_BreakpointsForDimension1_p,
      Autothrust_P.ScheduledGain_Table_b, 1U))) * look1_binlxpw(static_cast<real_T>
      (Autothrust_U.in.input.is_alt_soft_mode_active), Autothrust_P.ScheduledGain4_BreakpointsForDimension1,
      Autothrust_P.ScheduledGain4_Table, 1U);
    break;

   case 2:
    if (Phi_rad > Theta_rad) {
      rtb_Sum_h = Phi_rad;
    } else {
      rtb_Sum_h = Theta_rad;
    }

    rtb_Switch_dx = (rtb_Switch2_k - rtb_Sum_h) * Autothrust_P.Gain_Gain;
    break;

   default:
    if (Phi_rad > Theta_rad) {
      rtb_Sum_h = Phi_rad;
    } else {
      rtb_Sum_h = Theta_rad;
    }

    rtb_Switch_dx = (rtb_Sum_ew - rtb_Sum_h) * Autothrust_P.Gain_Gain_m;
    break;
  }

  rtb_Switch_dx = Autothrust_P.DiscreteTimeIntegratorVariableTsLimit_Gain * rtb_Switch_dx * Autothrust_U.in.time.dt;
  Autothrust_DWork.icLoad = (rtb_NOT || Autothrust_DWork.icLoad);
  if (Autothrust_DWork.icLoad) {
    if (Phi_rad > Theta_rad) {
      rtb_Sum_h = Phi_rad;
    } else {
      rtb_Sum_h = Theta_rad;
    }

    Autothrust_DWork.Delay_DSTATE_k = rtb_Sum_h - rtb_Switch_dx;
  }

  Autothrust_DWork.Delay_DSTATE_k += rtb_Switch_dx;
  if (Autothrust_DWork.Delay_DSTATE_k > rtb_Switch_m) {
    Autothrust_DWork.Delay_DSTATE_k = rtb_Switch_m;
  } else if (Autothrust_DWork.Delay_DSTATE_k < rtb_Switch2_k) {
    Autothrust_DWork.Delay_DSTATE_k = rtb_Switch2_k;
  }

  Autothrust_DWork.icLoad_c = (rtb_NOT || Autothrust_DWork.icLoad_c);
  if (Autothrust_DWork.icLoad_c) {
    Autothrust_DWork.Delay_DSTATE_j = Autothrust_DWork.Delay_DSTATE_k;
  }

  rtb_Switch_m = Autothrust_DWork.Delay_DSTATE_k - Autothrust_DWork.Delay_DSTATE_j;
  rtb_Divide = Autothrust_P.Constant2_Value * Autothrust_U.in.time.dt;
  if (rtb_Switch_m < rtb_Divide) {
    rtb_Divide = rtb_Switch_m;
  }

  denom_tmp = Autothrust_U.in.time.dt * Autothrust_P.Constant3_Value;
  if (rtb_Divide > denom_tmp) {
    denom_tmp = rtb_Divide;
  }

  Autothrust_DWork.Delay_DSTATE_j += denom_tmp;
  if (Autothrust_DWork.pUseAutoThrustControl) {
    if ((mode == Autothrust_P.CompareToConstant2_const_h) || (mode == Autothrust_P.CompareToConstant3_const)) {
      if (rtb_Switch1_k < rtb_y_b4) {
        rtb_Sum_h = rtb_y_b4;
      } else {
        rtb_Sum_h = rtb_Switch1_k;
      }

      if (rtb_Switch1_k < rtb_y_e) {
        rtb_Divide = rtb_y_e;
      } else {
        rtb_Divide = rtb_Switch1_k;
      }
    } else {
      if (Autothrust_DWork.Delay_DSTATE_j > rtb_y_b4) {
        rtb_Sum_h = rtb_y_b4;
      } else if (Autothrust_DWork.Delay_DSTATE_j < rtb_Switch2_k) {
        rtb_Sum_h = rtb_Switch2_k;
      } else {
        rtb_Sum_h = Autothrust_DWork.Delay_DSTATE_j;
      }

      if (Autothrust_DWork.Delay_DSTATE_j > rtb_y_e) {
        rtb_Divide = rtb_y_e;
      } else if (Autothrust_DWork.Delay_DSTATE_j < rtb_Switch2_k) {
        rtb_Divide = rtb_Switch2_k;
      } else {
        rtb_Divide = Autothrust_DWork.Delay_DSTATE_j;
      }
    }
  } else if (Autothrust_DWork.pThrustMemoActive) {
    rtb_Sum_h = Phi_rad;
    rtb_Divide = Theta_rad;
  } else {
    rtb_Sum_h = rtb_y_b4;
    rtb_Divide = rtb_y_e;
  }

  Autothrust_Y.out.output.N1_TLA_1_percent = rtb_y_b4;
  Autothrust_Y.out.output.N1_TLA_2_percent = rtb_y_e;
  rtb_Switch_m = rtb_Sum_h - Phi_rad;
  if (rtb_Compare_e) {
    Autothrust_DWork.Delay_DSTATE_n = Autothrust_P.DiscreteTimeIntegratorVariableTs_InitialCondition;
  }

  Autothrust_DWork.Delay_DSTATE_n += Autothrust_P.Gain_Gain_d * rtb_Switch_m *
    Autothrust_P.DiscreteTimeIntegratorVariableTs_Gain * Autothrust_U.in.time.dt;
  if (Autothrust_DWork.Delay_DSTATE_n > Autothrust_P.DiscreteTimeIntegratorVariableTs_UpperLimit) {
    Autothrust_DWork.Delay_DSTATE_n = Autothrust_P.DiscreteTimeIntegratorVariableTs_UpperLimit;
  } else if (Autothrust_DWork.Delay_DSTATE_n < Autothrust_P.DiscreteTimeIntegratorVariableTs_LowerLimit) {
    Autothrust_DWork.Delay_DSTATE_n = Autothrust_P.DiscreteTimeIntegratorVariableTs_LowerLimit;
  }

  if (!rtb_Compare_e) {
    Autothrust_DWork.Delay_DSTATE_l = Autothrust_P.DiscreteTimeIntegratorVariableTs1_InitialCondition;
  }

  Autothrust_DWork.Delay_DSTATE_l += Autothrust_P.Gain1_Gain_h * rtb_Switch_m *
    Autothrust_P.DiscreteTimeIntegratorVariableTs1_Gain * Autothrust_U.in.time.dt;
  if (Autothrust_DWork.Delay_DSTATE_l > Autothrust_P.DiscreteTimeIntegratorVariableTs1_UpperLimit) {
    Autothrust_DWork.Delay_DSTATE_l = Autothrust_P.DiscreteTimeIntegratorVariableTs1_UpperLimit;
  } else if (Autothrust_DWork.Delay_DSTATE_l < Autothrust_P.DiscreteTimeIntegratorVariableTs1_LowerLimit) {
    Autothrust_DWork.Delay_DSTATE_l = Autothrust_P.DiscreteTimeIntegratorVariableTs1_LowerLimit;
  }

  Autothrust_ThrustMode1(Autothrust_U.in.input.TLA_1_deg, &rtb_y_b4);
  rtb_Switch_dx = rtb_Divide - Theta_rad;
  if (rtb_NOT1_m) {
    Autothrust_DWork.Delay_DSTATE_lz = Autothrust_P.DiscreteTimeIntegratorVariableTs_InitialCondition_n;
  }

  Autothrust_DWork.Delay_DSTATE_lz += Autothrust_P.Gain_Gain_bf * rtb_Switch_dx *
    Autothrust_P.DiscreteTimeIntegratorVariableTs_Gain_k * Autothrust_U.in.time.dt;
  if (Autothrust_DWork.Delay_DSTATE_lz > Autothrust_P.DiscreteTimeIntegratorVariableTs_UpperLimit_p) {
    Autothrust_DWork.Delay_DSTATE_lz = Autothrust_P.DiscreteTimeIntegratorVariableTs_UpperLimit_p;
  } else if (Autothrust_DWork.Delay_DSTATE_lz < Autothrust_P.DiscreteTimeIntegratorVariableTs_LowerLimit_e) {
    Autothrust_DWork.Delay_DSTATE_lz = Autothrust_P.DiscreteTimeIntegratorVariableTs_LowerLimit_e;
  }

  if (!rtb_NOT1_m) {
    Autothrust_DWork.Delay_DSTATE_h = Autothrust_P.DiscreteTimeIntegratorVariableTs1_InitialCondition_e;
  }

  Autothrust_DWork.Delay_DSTATE_h += Autothrust_P.Gain1_Gain_g * rtb_Switch_dx *
    Autothrust_P.DiscreteTimeIntegratorVariableTs1_Gain_l * Autothrust_U.in.time.dt;
  if (Autothrust_DWork.Delay_DSTATE_h > Autothrust_P.DiscreteTimeIntegratorVariableTs1_UpperLimit_o) {
    Autothrust_DWork.Delay_DSTATE_h = Autothrust_P.DiscreteTimeIntegratorVariableTs1_UpperLimit_o;
  } else if (Autothrust_DWork.Delay_DSTATE_h < Autothrust_P.DiscreteTimeIntegratorVariableTs1_LowerLimit_h) {
    Autothrust_DWork.Delay_DSTATE_h = Autothrust_P.DiscreteTimeIntegratorVariableTs1_LowerLimit_h;
  }

  Autothrust_ThrustMode1(Autothrust_U.in.input.TLA_2_deg, &rtb_y_e);
  Autothrust_Y.out.time = Autothrust_U.in.time;
  Autothrust_Y.out.data.nz_g = Autothrust_U.in.data.nz_g;
  Autothrust_Y.out.data.Theta_deg = rtb_Gain2;
  Autothrust_Y.out.data.Phi_deg = rtb_Gain3;
  Autothrust_Y.out.data.V_ias_kn = Autothrust_U.in.data.V_ias_kn;
  Autothrust_Y.out.data.V_tas_kn = Autothrust_U.in.data.V_tas_kn;
  Autothrust_Y.out.data.V_mach = Autothrust_U.in.data.V_mach;
  Autothrust_Y.out.data.V_gnd_kn = Autothrust_U.in.data.V_gnd_kn;
  Autothrust_Y.out.data.alpha_deg = Autothrust_U.in.data.alpha_deg;
  Autothrust_Y.out.data.H_ft = Autothrust_U.in.data.H_ft;
  Autothrust_Y.out.data.H_ind_ft = Autothrust_U.in.data.H_ind_ft;
  Autothrust_Y.out.data.H_radio_ft = Autothrust_U.in.data.H_radio_ft;
  Autothrust_Y.out.data.H_dot_fpm = Autothrust_U.in.data.H_dot_fpm;
  Autothrust_Y.out.data.ax_m_s2 = result[0];
  Autothrust_Y.out.data.ay_m_s2 = result[1];
  Autothrust_Y.out.data.az_m_s2 = result[2];
  Autothrust_Y.out.data.bx_m_s2 = Autothrust_U.in.data.bx_m_s2;
  Autothrust_Y.out.data.by_m_s2 = Autothrust_U.in.data.by_m_s2;
  Autothrust_Y.out.data.bz_m_s2 = Autothrust_U.in.data.bz_m_s2;
  Autothrust_Y.out.data.on_ground = (rtb_on_ground != 0);
  Autothrust_Y.out.data.flap_handle_index = Autothrust_U.in.data.flap_handle_index;
  Autothrust_Y.out.data.is_engine_operative_1 = Autothrust_U.in.data.is_engine_operative_1;
  Autothrust_Y.out.data.is_engine_operative_2 = Autothrust_U.in.data.is_engine_operative_2;
  Autothrust_Y.out.data.commanded_engine_N1_1_percent = Phi_rad;
  Autothrust_Y.out.data.commanded_engine_N1_2_percent = Theta_rad;
  Autothrust_Y.out.data.engine_N1_1_percent = Autothrust_U.in.data.engine_N1_1_percent;
  Autothrust_Y.out.data.engine_N1_2_percent = Autothrust_U.in.data.engine_N1_2_percent;
  Autothrust_Y.out.data.TAT_degC = Autothrust_U.in.data.TAT_degC;
  Autothrust_Y.out.data.OAT_degC = Autothrust_U.in.data.OAT_degC;
  Autothrust_Y.out.data.ISA_degC = rtb_Saturation;
  Autothrust_Y.out.data_computed.TLA_in_active_range = rtb_out;
  Autothrust_Y.out.data_computed.is_FLX_active = rtb_y_k;
  Autothrust_Y.out.data_computed.ATHR_push = rtb_BusAssignment_n.data_computed.ATHR_push;
  Autothrust_Y.out.data_computed.ATHR_disabled = Autothrust_DWork.Memory_PreviousInput;
  Autothrust_Y.out.data_computed.time_since_touchdown = rtb_BusAssignment_n.data_computed.time_since_touchdown;
  Autothrust_Y.out.data_computed.alpha_floor_inhibited = Autothrust_DWork.sInhibit;
  Autothrust_Y.out.input.ATHR_push = Autothrust_U.in.input.ATHR_push;
  Autothrust_Y.out.input.ATHR_disconnect = Autothrust_U.in.input.ATHR_disconnect;
  Autothrust_Y.out.input.TLA_1_deg = Autothrust_U.in.input.TLA_1_deg;
  Autothrust_Y.out.input.TLA_2_deg = Autothrust_U.in.input.TLA_2_deg;
  Autothrust_Y.out.input.V_c_kn = Autothrust_U.in.input.V_c_kn;
  Autothrust_Y.out.input.V_LS_kn = Autothrust_U.in.input.V_LS_kn;
  Autothrust_Y.out.input.V_MAX_kn = Autothrust_U.in.input.V_MAX_kn;
  Autothrust_Y.out.input.thrust_limit_REV_percent = Autothrust_U.in.input.thrust_limit_REV_percent;
  Autothrust_Y.out.input.thrust_limit_IDLE_percent = rtb_Switch2_k;
  Autothrust_Y.out.input.thrust_limit_CLB_percent = rtb_Sum_ew;
  Autothrust_Y.out.input.thrust_limit_MCT_percent = rtb_BusAssignment_n.input.thrust_limit_MCT_percent;
  Autothrust_Y.out.input.thrust_limit_FLEX_percent = rtb_Max_g;
  Autothrust_Y.out.input.thrust_limit_TOGA_percent = rtb_Switch1_k;
  Autothrust_Y.out.input.flex_temperature_degC = Autothrust_U.in.input.flex_temperature_degC;
  Autothrust_Y.out.input.mode_requested = Autothrust_U.in.input.mode_requested;
  Autothrust_Y.out.input.is_mach_mode_active = Autothrust_U.in.input.is_mach_mode_active;
  Autothrust_Y.out.input.alpha_floor_condition = Autothrust_U.in.input.alpha_floor_condition;
  Autothrust_Y.out.input.is_approach_mode_active = Autothrust_U.in.input.is_approach_mode_active;
  Autothrust_Y.out.input.is_SRS_TO_mode_active = Autothrust_U.in.input.is_SRS_TO_mode_active;
  Autothrust_Y.out.input.is_SRS_GA_mode_active = Autothrust_U.in.input.is_SRS_GA_mode_active;
  Autothrust_Y.out.input.is_LAND_mode_active = Autothrust_U.in.input.is_LAND_mode_active;
  Autothrust_Y.out.input.thrust_reduction_altitude = Autothrust_U.in.input.thrust_reduction_altitude;
  Autothrust_Y.out.input.thrust_reduction_altitude_go_around = Autothrust_U.in.input.thrust_reduction_altitude_go_around;
  Autothrust_Y.out.input.flight_phase = Autothrust_U.in.input.flight_phase;
  Autothrust_Y.out.input.is_alt_soft_mode_active = Autothrust_U.in.input.is_alt_soft_mode_active;
  Autothrust_Y.out.input.is_anti_ice_wing_active = Autothrust_U.in.input.is_anti_ice_wing_active;
  Autothrust_Y.out.input.is_anti_ice_engine_1_active = Autothrust_U.in.input.is_anti_ice_engine_1_active;
  Autothrust_Y.out.input.is_anti_ice_engine_2_active = Autothrust_U.in.input.is_anti_ice_engine_2_active;
  Autothrust_Y.out.input.is_air_conditioning_1_active = Autothrust_U.in.input.is_air_conditioning_1_active;
  Autothrust_Y.out.input.is_air_conditioning_2_active = Autothrust_U.in.input.is_air_conditioning_2_active;
  Autothrust_Y.out.input.FD_active = Autothrust_U.in.input.FD_active;
  Autothrust_Y.out.input.ATHR_reset_disable = Autothrust_U.in.input.ATHR_reset_disable;
  Autothrust_Y.out.input.is_TCAS_active = Autothrust_U.in.input.is_TCAS_active;
  Autothrust_Y.out.input.target_TCAS_RA_rate_fpm = Autothrust_U.in.input.target_TCAS_RA_rate_fpm;
  if (!rtb_Compare_e) {
    Autothrust_Y.out.output.sim_throttle_lever_1_pos = Autothrust_DWork.Delay_DSTATE_n;
  } else {
    Autothrust_Y.out.output.sim_throttle_lever_1_pos = Autothrust_DWork.Delay_DSTATE_l;
  }

  if (!rtb_NOT1_m) {
    Autothrust_Y.out.output.sim_throttle_lever_2_pos = Autothrust_DWork.Delay_DSTATE_lz;
  } else {
    Autothrust_Y.out.output.sim_throttle_lever_2_pos = Autothrust_DWork.Delay_DSTATE_h;
  }

  Autothrust_Y.out.output.sim_thrust_mode_1 = rtb_y_b4;
  Autothrust_Y.out.output.sim_thrust_mode_2 = rtb_y_e;
  Autothrust_Y.out.output.is_in_reverse_1 = rtb_Compare_e;
  Autothrust_Y.out.output.is_in_reverse_2 = rtb_NOT1_m;
  Autothrust_Y.out.output.N1_c_1_percent = rtb_Sum_h;
  Autothrust_Y.out.output.N1_c_2_percent = rtb_Divide;
  Autothrust_Y.out.output.status = status;
  Autothrust_Y.out.output.mode = mode;
  rtb_out = !Autothrust_DWork.inhibitAboveThrustReductionAltitude;
  condition_TOGA = (((!Autothrust_U.in.input.is_SRS_TO_mode_active) || ((Autothrust_U.in.data.H_ind_ft >=
    Autothrust_U.in.input.thrust_reduction_altitude) && rtb_out)) && ((!Autothrust_U.in.input.is_SRS_GA_mode_active) ||
    ((Autothrust_U.in.data.H_ind_ft >= Autothrust_U.in.input.thrust_reduction_altitude_go_around) && rtb_out)) &&
                    ((Autothrust_U.in.data.H_radio_ft > 400.0) || (Autothrust_U.in.input.flight_phase > 2.0)) && (tmp ||
    (Autothrust_U.in.input.target_TCAS_RA_rate_fpm <= 500.0) || (Autothrust_U.in.input.TLA_1_deg <= 25.0) ||
    (Autothrust_U.in.input.TLA_2_deg <= 25.0)));
  if ((status != athr_status_DISENGAGED) && (Autothrust_DWork.pMode != athr_mode_A_FLOOR) && (Autothrust_DWork.pMode !=
       athr_mode_TOGA_LK) && (rtb_on_ground == 0) && (condition_TOGA || (Autothrust_U.in.input.is_TCAS_active &&
        (Autothrust_U.in.data.H_ind_ft < Autothrust_U.in.input.thrust_reduction_altitude) &&
        ((Autothrust_U.in.data.V_ias_kn > Autothrust_U.in.input.V_MAX_kn - 20.0) ||
         (Autothrust_U.in.input.target_TCAS_RA_rate_fpm <= 500.0)))) && Autothrust_U.in.data.is_engine_operative_1 &&
      Autothrust_U.in.data.is_engine_operative_2 && (((Autothrust_U.in.input.TLA_1_deg < 25.0) &&
        (Autothrust_U.in.input.TLA_2_deg < 25.0)) || (Autothrust_U.in.input.TLA_1_deg > 25.0) ||
       (Autothrust_U.in.input.TLA_2_deg > 25.0))) {
    Autothrust_Y.out.output.mode_message = athr_mode_message_LVR_CLB;
  } else if ((status != athr_status_DISENGAGED) && (Autothrust_DWork.pMode != athr_mode_A_FLOOR) &&
             (Autothrust_DWork.pMode != athr_mode_TOGA_LK) && (rtb_on_ground == 0) && condition_TOGA &&
             (condition_AP_FD_ATHR_Specific || condition_AlphaFloor_tmp) && (Autothrust_U.in.input.TLA_1_deg != 35.0) &&
             (Autothrust_U.in.input.TLA_2_deg != 35.0)) {
    Autothrust_Y.out.output.mode_message = athr_mode_message_LVR_MCT;
  } else if ((status == athr_status_ENGAGED_ACTIVE) && Autothrust_U.in.data.is_engine_operative_1 &&
             Autothrust_U.in.data.is_engine_operative_2 && (((Autothrust_U.in.input.TLA_1_deg == 25.0) &&
               (Autothrust_U.in.input.TLA_2_deg != 25.0)) || ((Autothrust_U.in.input.TLA_2_deg == 25.0) &&
               (Autothrust_U.in.input.TLA_1_deg != 25.0)))) {
    Autothrust_Y.out.output.mode_message = athr_mode_message_LVR_ASYM;
  } else if (Autothrust_DWork.condition_THR_LK) {
    Autothrust_Y.out.output.mode_message = athr_mode_message_THR_LK;
  } else {
    Autothrust_Y.out.output.mode_message = athr_mode_message_NONE;
  }

  Autothrust_Y.out.output.thrust_lever_warning_flex = (rtb_BusAssignment_n.data_computed.is_FLX_active && condition02);
  Autothrust_Y.out.output.thrust_lever_warning_toga = ((!rtb_BusAssignment_n.data_computed.is_FLX_active) &&
    ((Autothrust_U.in.time.simulation_time - Autothrust_DWork.eventTime >= 3.0) || condition02));
  Autothrust_DWork.Delay_DSTATE = rtb_OATCornerPoint;
  Autothrust_DWork.icLoad = false;
  Autothrust_DWork.icLoad_c = false;
}

void AutothrustModelClass::initialize()
{
  Autothrust_DWork.Memory_PreviousInput = Autothrust_P.SRFlipFlop_initial_condition;
  Autothrust_DWork.Delay_DSTATE_a = Autothrust_P.Delay_InitialCondition;
  Autothrust_DWork.Memory_PreviousInput_m = Autothrust_P.SRFlipFlop_initial_condition_g;
  Autothrust_DWork.Delay_DSTATE = Autothrust_P.DiscreteDerivativeVariableTs_InitialCondition;
  Autothrust_DWork.icLoad = true;
  Autothrust_DWork.icLoad_c = true;
  Autothrust_DWork.Delay_DSTATE_n = Autothrust_P.DiscreteTimeIntegratorVariableTs_InitialCondition;
  Autothrust_DWork.Delay_DSTATE_l = Autothrust_P.DiscreteTimeIntegratorVariableTs1_InitialCondition;
  Autothrust_DWork.Delay_DSTATE_lz = Autothrust_P.DiscreteTimeIntegratorVariableTs_InitialCondition_n;
  Autothrust_DWork.Delay_DSTATE_h = Autothrust_P.DiscreteTimeIntegratorVariableTs1_InitialCondition_e;
}

void AutothrustModelClass::terminate()
{
}

AutothrustModelClass::AutothrustModelClass() :
  Autothrust_DWork(),
  Autothrust_U(),
  Autothrust_Y()
{
}

AutothrustModelClass::~AutothrustModelClass()
{
}
