#ifndef LAYMANS_FFT_H
#define LAYMANS_FFT_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  RECTANGULAR,
  BARTLETT,
  HAMMING,
  HANN,
  IMPERIAL,
  HIGH_IMPERIAL,
  FLAT_TOP,
  BLACKMAN,
  BLACKMAN_HARRIS
} windows_t;

typedef struct fft_handle_t fft_handle_t;


fft_handle_t* fft_get_handle(void);

int8_t init_fft( fft_handle_t *handle);
int8_t fft_setup( fft_handle_t *handle, uint16_t smpl_size, windows_t window_type);
int8_t run_fft( fft_handle_t *handle, int32_t *smpl_data);
int8_t get_mag_db( fft_handle_t *handle, int32_t *fft_data, float *mag_data);
int8_t run_fft_w_mag_db( fft_handle_t *handle, int32_t *smpl_data, float *mag_data);
// void BARTLETT_WIN(int32_t *data, uint16_t N);
// void HAMMING_WIN(int32_t *data, uint16_t N, float m_pi);
// void HANN_WIN(int32_t *data, uint16_t N, float m_pi);
// void IMPERIAL_WIN(int32_t *data, uint16_t N, float m_pi);
// void FLAT_TOP_WIN(int32_t *data, uint16_t N, float m_pi);
// void BLACKMAN_WIN(int32_t *data, uint16_t N, float m_pi);
// void BLACKMAN_HARRIS_WIN(int32_t *data, uint16_t N, float m_pi);

#ifdef __cplusplus
}
#endif

#endif



















