#ifndef APOLLO3_ADC_LIB_H
#define APOLLO3_ADC_LIB_H

#include <Arduino.h>
#include "am_bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ADC_A0 = AM_HAL_ADC_SLOT_CHSEL_SE1,
  ADC_A1 = AM_HAL_ADC_SLOT_CHSEL_SE2,
  ADC_A2 = AM_HAL_ADC_SLOT_CHSEL_SE6,
  ADC_A3 = AM_HAL_ADC_SLOT_CHSEL_SE5,
  ADC_A4 = AM_HAL_ADC_SLOT_CHSEL_SE0,
  ADC_A5 = AM_HAL_ADC_SLOT_CHSEL_SE3
}adc_pin_t;

typedef enum {
  OSR_1 = AM_HAL_ADC_SLOT_AVG_1,
  OSR_2 = AM_HAL_ADC_SLOT_AVG_2,
  OSR_4 = AM_HAL_ADC_SLOT_AVG_4,
  OSR_8 = AM_HAL_ADC_SLOT_AVG_8,
  OSR_16 = AM_HAL_ADC_SLOT_AVG_16,
  OSR_32 = AM_HAL_ADC_SLOT_AVG_32,
  OSR_64 = AM_HAL_ADC_SLOT_AVG_64,
  OSR_128 = AM_HAL_ADC_SLOT_AVG_128
}osr_t;

typedef enum {
  ADC_8BIT = AM_HAL_ADC_SLOT_8BIT,
  ADC_10BIT = AM_HAL_ADC_SLOT_10BIT,
  ADC_12BIT = AM_HAL_ADC_SLOT_12BIT,
  ADC_14BIT = AM_HAL_ADC_SLOT_14BIT
}adc_resolution_bits_t;

typedef struct adc_handle_t adc_handle_t;

adc_handle_t* adc_get_handle(void);
int8_t adc_setup(struct adc_handle_t *handle, int32_t *smpl_buffer);
int8_t adc_software_trigger(struct adc_handle_t *handle, int32_t *smpl_buffer);
int8_t adc_smpl_status(struct adc_handle_t *handle);
int8_t adc_config(struct adc_handle_t *handle, uint32_t smpl_frq, uint32_t smpl_size, adc_pin_t pin, osr_t osr, adc_resolution_bits_t resolution);
int8_t adc_clear_status(struct adc_handle_t *handle);
int8_t adc_transfer_data(struct adc_handle_t *handle, int32_t *smpl_buffer);
int8_t adc_get_true_smpl_frq(struct adc_handle_t *handle, float *true_smpl_frq);

#ifdef __cplusplus
}
#endif

#endif




