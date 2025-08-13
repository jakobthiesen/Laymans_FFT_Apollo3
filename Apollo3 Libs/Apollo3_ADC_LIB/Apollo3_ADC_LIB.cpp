#include "am_util_delay.h"
#include "am_util_stdio.h"
#include "am_hal_gpio.h"
#include "am_hal_interrupt.h"
#include "am_hal_adc.h"
#include <Arduino.h>
#include "am_bsp.h"
#include "Apollo3_ADC_LIB.h"


typedef struct adc_handle_t {
  struct cfg{
    struct adc_handle_t *selfAddr;
    void *hal_handle;
    volatile bool adc_dma_cmplt;
    volatile bool adc_dma_error;
    bool smpl_cmplt;
    am_hal_gpio_pincfg_t hal_adc_pin;
    uint32_t smpl_frq;
    float true_smpl_frq;
    osr_t osr_ratio;
    uint16_t smpl_size;
    int32_t dma_target_addr;
    adc_resolution_bits_t adc_resolution;
    adc_pin_t input_pin;
    int8_t (*get_timer_setting)(uint32_t, uint32_t*, uint32_t*, float*);
  }cfg;
  int8_t (*trig_adc)(void);
}adc_handle_t;



int8_t adc_cfg_dma(struct adc_handle_t *handle, int32_t *smpl_buffer);
int8_t adc_cfg(struct adc_handle_t *handle, int32_t *smpl_buffer);
int8_t get_timer_setting(struct adc_handle_t *handle, uint32_t smpl_frq, uint32_t *period, uint32_t *pulse, float *true_smpl_frq);
int8_t init_timer_A3_adc(struct adc_handle_t *hanlde);
void get_artemis_adc_pin(adc_pin_t pin, uint32_t *uFunc_pin, uint32_t *pad_sel);


static adc_handle_t adc_handler;



adc_handle_t* adc_get_handle(void){
  return &adc_handler;
}





extern "C" void am_adc_isr() {
  uint32_t ui32intMask;
  am_hal_adc_interrupt_status(adc_handler.cfg.hal_handle, &ui32intMask, 0);
  am_hal_adc_interrupt_clear(adc_handler.cfg.hal_handle, ui32intMask);
  if(ui32intMask & AM_HAL_ADC_INT_DCMP) {
    adc_handler.cfg.adc_dma_cmplt = 1;
  }
  if(ui32intMask & AM_HAL_ADC_INT_DERR) {
    adc_handler.cfg.adc_dma_error = 1;
  }
}

int8_t adc_config(struct adc_handle_t *handle, uint32_t smpl_frq, uint32_t smpl_size, adc_pin_t pin, osr_t osr, adc_resolution_bits_t resolution) {
  int8_t r = 0;
  if(handle != NULL) {
  handle->cfg.selfAddr = handle;
  handle->cfg.smpl_frq = smpl_frq;
  handle->cfg.smpl_size = smpl_size;
  handle->cfg.input_pin = pin;
  handle->cfg.osr_ratio = osr;
  handle->cfg.adc_resolution = resolution;
  } else r = -1;
  return r;
}

int8_t adc_transfer_data(struct adc_handle_t *handle, int32_t *smpl_buffer){
  int8_t r = 0;
  if(handle != NULL) {
    for(uint32_t i = 0; i < handle->cfg.smpl_size; i++) {
      smpl_buffer[i] >>= 6;
    }
  } else r = -1;
  return r;
}

int8_t adc_get_true_smpl_frq(struct adc_handle_t *handle, float *true_smpl_frq) {
  int8_t r = 0;
  if(handle != NULL) {
    *true_smpl_frq = handle->cfg.true_smpl_frq;
  } else r = -1;
  return r;
}

int8_t adc_smpl_status(struct adc_handle_t *handle){
  int8_t r = 0;
  if(handle != NULL) {
    if(handle -> cfg.adc_dma_cmplt) {
      r = 1;
    }
    if(handle -> cfg.adc_dma_error) {
      r = -2;
    }
  } else r = -1;
  return r;
}

int8_t adc_clear_status(struct adc_handle_t *handle) {
  int8_t r = 0;
  if(handle != NULL) {
    handle -> cfg.adc_dma_cmplt = 0;
    handle -> cfg.adc_dma_error = 0;
  } else r = -1;
  return r;
}

int8_t adc_cfg_dma(struct adc_handle_t *handle, int32_t *smpl_buffer){
  int8_t r = 0;
  if(handle != NULL) {
    am_hal_adc_dma_config_t adc_dma_cfg;

    adc_dma_cfg.bDynamicPriority = 1;
    adc_dma_cfg.ePriority = AM_HAL_ADC_PRIOR_SERVICE_IMMED;
    adc_dma_cfg.bDMAEnable = 1;
    adc_dma_cfg.ui32SampleCount = handle -> cfg.smpl_size;
    adc_dma_cfg.ui32TargetAddress = (int32_t)smpl_buffer;

    am_hal_adc_configure_dma(handle->cfg.hal_handle, &adc_dma_cfg);

    handle->cfg.adc_dma_cmplt = 0;
    handle->cfg.adc_dma_error = 0;
  } else r = -1;
  return r;
}

int8_t adc_cfg(struct adc_handle_t *handle, int32_t *smpl_buffer) {
  int8_t r = 0;
  if(handle != NULL) {
    am_hal_adc_config_t adc_cfg;
    am_hal_adc_slot_config_t adc_slot_cfg;

    am_hal_adc_initialize(0, &(handle->cfg.hal_handle));
    am_hal_adc_power_control(handle->cfg.hal_handle, AM_HAL_SYSCTRL_WAKE, 0);

    adc_cfg.eClock = AM_HAL_ADC_CLKSEL_HFRC;
    adc_cfg.ePolarity = AM_HAL_ADC_TRIGPOL_RISING;
    adc_cfg.eTrigger = AM_HAL_ADC_TRIGSEL_SOFTWARE;
    adc_cfg.eReference = AM_HAL_ADC_REFSEL_INT_2P0;
    adc_cfg.eClockMode = AM_HAL_ADC_CLKMODE_LOW_LATENCY;
    adc_cfg.ePowerMode = AM_HAL_ADC_LPMODE0;
    adc_cfg.eRepeat = AM_HAL_ADC_REPEATING_SCAN;

    am_hal_adc_configure(handle->cfg.hal_handle, &adc_cfg);
    

    adc_slot_cfg.eMeasToAvg = (am_hal_adc_meas_avg_e)(handle -> cfg.osr_ratio);
    adc_slot_cfg.ePrecisionMode = (am_hal_adc_slot_prec_e)(handle -> cfg.adc_resolution);
    adc_slot_cfg.eChannel = (am_hal_adc_slot_chan_e)(handle -> cfg.input_pin);
    adc_slot_cfg.bWindowCompare = 0;
    adc_slot_cfg.bEnabled = 1;

    am_hal_adc_configure_slot(handle->cfg.hal_handle, 0, &adc_slot_cfg);

    adc_cfg_dma(handle, smpl_buffer);
    am_hal_adc_interrupt_enable(handle->cfg.hal_handle, AM_HAL_ADC_INT_DERR | AM_HAL_ADC_INT_DCMP);
    am_hal_adc_enable(handle->cfg.hal_handle);    
  } else r = -1;
  return r;
}

int8_t get_timer_setting(struct adc_handle_t *handle, uint32_t smpl_frq, uint32_t *period, uint32_t *pulse, float *true_smpl_frq) {
  int8_t r = 0;
  if(handle != NULL) {
    uint32_t max_frq = 1200000;
    adc_resolution_bits_t adc_res = handle->cfg.adc_resolution;
    switch(adc_res) {
      case(ADC_8BIT):
        max_frq = 2400000;
        break;
      case(ADC_10BIT):
        max_frq = 2000000;
        break;
      case(ADC_12BIT):
        max_frq = 1500000;
        break;
      case(ADC_14BIT):
        max_frq = 1200000;
        break;
    }
    if(smpl_frq > max_frq) {
      smpl_frq = max_frq;
    }
    uint32_t k = ((uint32_t)(12000000.0/((float)smpl_frq)));
    *period = (k-1);
    *pulse = (uint32_t)(k/2.0);
    *true_smpl_frq = 12000000.0/((float)k);
  } else r = -1;
  return r;
}

int8_t init_timer_A3_adc(struct adc_handle_t *handle) {
  int8_t r = 0;
  if(handle != NULL) {
    uint32_t period = 0;
    uint32_t pulse = 0;

    get_timer_setting(handle,handle -> cfg.smpl_frq, &period, &pulse, &(handle->cfg.true_smpl_frq));

    am_hal_ctimer_config_single(3, AM_HAL_CTIMER_TIMERA,
    AM_HAL_CTIMER_HFRC_12MHZ | AM_HAL_CTIMER_FN_REPEAT | AM_HAL_CTIMER_INT_ENABLE);
    am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERA3);
    am_hal_ctimer_period_set(3, AM_HAL_CTIMER_TIMERA, period, pulse);
    am_hal_ctimer_adc_trigger_enable();
    am_hal_ctimer_start(3, AM_HAL_CTIMER_TIMERA);
  } else r = -1;
  return r;
}


void get_artemis_adc_pin(adc_pin_t pin, uint32_t *uFunc_pin, uint32_t *pad_sel) {
  uint32_t sel_pin = 0;
  uint32_t sel_pad = 0;
  switch(pin) {
    case(ADC_A0):
      sel_pin = AM_HAL_PIN_29_ADCSE1;
      sel_pad = 29;
      break;
    case(ADC_A1):
      sel_pin = AM_HAL_PIN_11_ADCSE2;
      sel_pad = 11;
      break;
    case(ADC_A2):
      sel_pin = AM_HAL_PIN_34_ADCSE6;
      sel_pad = 34;
      break;
    case(ADC_A3):
      sel_pin = AM_HAL_PIN_33_ADCSE5;
      sel_pad = 33;
      break;
    case(ADC_A4):
      sel_pin = AM_HAL_PIN_16_ADCSE0;
      sel_pad = 16;
      break;
    case(ADC_A5):
      sel_pin = AM_HAL_PIN_31_ADCSE3;
      sel_pad = 31;
      break;
  }
  *uFunc_pin = sel_pin;
  *pad_sel = sel_pad;
}


int8_t adc_setup(struct adc_handle_t *handle, int32_t *smpl_buffer){
  int8_t r = 0;
  if(handle != NULL) {
    uint32_t pin_cfg;
    uint32_t pad_cfg;
    get_artemis_adc_pin(handle->cfg.input_pin, &pin_cfg, &pad_cfg);
    handle->cfg.hal_adc_pin = {.uFuncSel = pin_cfg,};
    
    am_bsp_itm_printf_enable();
    init_timer_A3_adc(handle);
    NVIC_EnableIRQ(ADC_IRQn);
    am_hal_interrupt_master_enable();
    am_hal_gpio_pinconfig(pad_cfg, handle->cfg.hal_adc_pin);

    adc_cfg(handle, smpl_buffer);

    am_hal_adc_sw_trigger(handle->cfg.hal_handle);
    am_util_stdio_terminal_clear();
    am_util_delay_ms(10);
    
  } else r = -1;
  return r;
}

int8_t adc_software_trigger(struct adc_handle_t *handle, int32_t *smpl_buffer){
  int8_t r = 0;
  if(handle != NULL) {
    handle -> cfg.smpl_cmplt = 0;
    handle -> cfg.adc_dma_cmplt = 0;

    adc_cfg_dma(handle, smpl_buffer);

    am_hal_adc_interrupt_clear(handle->cfg.hal_handle, 0XFFFFFFFF);
    am_hal_adc_sw_trigger(handle->cfg.hal_handle);
  } else r = -1;
  return r;
}










