#include "Laymans_FFT.h"
#include <Arduino.h>
#include <arm_math.h>
#include <stdint.h>
#include <math.h>


// typedef struct fft_handle_t fft_handle_t;
typedef struct fft_handle_t {
  struct cfg{
    struct fft_handle_t *self_addr;
    float m_pi;
    uint16_t lut_size;
    uint8_t lut_step_skip;
    uint8_t quarter_lut;
    uint16_t smpl_size;
    int16_t lut_step;
    uint8_t fft_level;
    uint16_t (*bit_reverse)(uint8_t, uint16_t);
    int8_t (*get_twiddle)(struct fft_handle_t*, uint16_t, int16_t*, int32_t*);
    int8_t (*get_twiddle_linear)(struct fft_handle_t*, uint16_t, int16_t*, int32_t*);
    int8_t (*fill_lut)(struct fft_handle_t*, int16_t*, uint16_t);
    uint16_t mask_a;
    uint16_t mask_b;
    windows_t win;
    float window_gain_correction;
  }cfg;
  // int8_t (*fft_setup)(struct fft_handle_t*, uint16_t);
  int8_t (*run_fft)(struct fft_handle_t*, int32_t*, int16_t*);
  int8_t (*run_fft_linear)(struct fft_handle_t*, int32_t*, int16_t*);
  int8_t (*fft_mag)(struct fft_handle_t*, int32_t*);
  int8_t (*fft_mag_db)(struct fft_handle_t*, int32_t*, float*);
  int8_t (*full_fft_w_mag)(struct fft_handle_t*, int32_t *, int16_t*, float*, windows_t, bool);
  int8_t (*fft_reorder)(struct fft_handle_t*, int32_t*);
  int8_t (*fft_window)(struct fft_handle_t*, int32_t*, windows_t);
}fft_handle_t;




// int8_t fft_setup(struct fft_handle_t *handle, uint16_t smpl_size);
int8_t fill_lut(struct fft_handle_t *handle, int16_t *LUT, uint16_t LUT_size);
int8_t get_twiddle(struct fft_handle_t *handle, uint16_t index, int16_t *LUT_Q15, int32_t *w_ri);
int8_t get_twiddle_linear(struct fft_handle_t *handle, uint16_t index, int16_t *LUT_Q15, int32_t *w_ri);
uint16_t bit_reverse(uint8_t l, uint16_t x);
int8_t fft_reorder(struct fft_handle_t *handle, int32_t *data);
int8_t fft_window(struct fft_handle_t *handle, int32_t *data, windows_t W);
int8_t run_fft_cb(struct fft_handle_t *handle, int32_t *data, int16_t *LUT);
int8_t run_fft_linear_cb(struct fft_handle_t *handle, int32_t *data, int16_t *LUT);
int8_t fft_mag_cb(struct fft_handle_t *handle, int32_t *data);
int8_t fft_mag_db_cb(struct fft_handle_t *handle, int32_t *data_i, float *data_o);
int8_t full_fft_w_mag_cb(struct fft_handle_t *handle, int32_t *data, int16_t *LUT, float *data_o, windows_t W, bool Linear);
void bartlett_win(int32_t *data, uint16_t N);
void hamming_win(int32_t *data, uint16_t N, float m_pi);
void hann_win(int32_t *data, uint16_t N, float m_pi);
void imperial_win(int32_t *data, uint16_t N, float m_pi);
void high_imperial_win(int32_t *data, uint16_t N, float m_pi);
void flat_top_win(int32_t *data, uint16_t N, float m_pi);
void blackman_win(int32_t *data, uint16_t N, float m_pi);
void blackman_harris_win(int32_t *data, uint16_t N, float m_pi);

int16_t tw_lut[512];

static fft_handle_t fft_handle;

fft_handle_t* fft_get_handle(void){
  return &fft_handle;
}

int8_t init_fft(struct fft_handle_t *handle) {
  int8_t r = 0;
  if(handle != NULL) {
    handle -> cfg.self_addr = handle;
    handle -> cfg.m_pi = 3.14159265;
    handle -> cfg.window_gain_correction = 1.0;
    handle -> cfg.win = RECTANGULAR;
    handle -> cfg.bit_reverse = bit_reverse;
    handle -> cfg.get_twiddle = get_twiddle;
    handle -> cfg.get_twiddle_linear = get_twiddle_linear;
    handle -> cfg.fill_lut = fill_lut;
    handle -> cfg.fill_lut(handle, tw_lut, 512);
    handle -> fft_reorder = fft_reorder;
    handle -> fft_window = fft_window;
    // handle -> fft_setup = fft_setup;
    handle -> run_fft = run_fft_linear_cb; 
    handle -> run_fft_linear = run_fft_linear_cb;
    handle -> fft_mag = fft_mag_cb;
    handle -> fft_mag_db = fft_mag_db_cb;
    handle -> full_fft_w_mag = full_fft_w_mag_cb;
  } else r = -1;
  return r;
}



int8_t fft_setup(struct fft_handle_t *handle, uint16_t smpl_size, windows_t window_type) {
  int8_t r = 0;
  if(handle != NULL) {
    int8_t level = log2(smpl_size);
    handle -> cfg.smpl_size = smpl_size;
    handle -> cfg.fft_level = level;
    handle -> cfg.win = window_type;
    handle -> cfg.mask_a = (0xFFFF>>(16-level));
    handle -> cfg.mask_b = (0xFFFF>>(17-level));
    handle -> cfg.quarter_lut = handle -> cfg.lut_size/4;
    if(smpl_size <= handle -> cfg.lut_size) {
      handle -> cfg.lut_step_skip = handle -> cfg.lut_size/smpl_size;
    } else {
      handle -> cfg.lut_step_skip = 1;
    }
    } else r = -1;
  return r;
} 

int8_t run_fft(struct fft_handle_t *handle, int32_t *smpl_data) {
  int8_t r = 0;
  if(handle != NULL) {
    handle->run_fft(handle, smpl_data, tw_lut);
  } else r = -1;
  return r;
}

int8_t get_mag_db(struct fft_handle_t *handle, int32_t *fft_data, float *mag_data) {
  int8_t r = 0;
  if(handle != NULL) {
    handle->fft_mag_db(handle, fft_data, mag_data);
  } else r = -1;
  return r;
}

int8_t run_fft_w_mag_db(struct fft_handle_t *handle, int32_t *smpl_data, float *mag_data) {
  int8_t r = 0;
  if(handle != NULL) {
    windows_t win = handle->cfg.win;
    handle->full_fft_w_mag(handle, smpl_data, tw_lut, mag_data, win, 1);
  } else r = 0;
  return r;
}


int8_t fill_lut(struct fft_handle_t *handle, int16_t *LUT, uint16_t LUT_size) {
  int8_t r = 0;
  if(handle != NULL) {
    float angle = 0;
    float step = (2*(handle -> cfg.m_pi))/(LUT_size);
    handle->cfg.lut_size = LUT_size;
    handle -> cfg.lut_step = (int16_t)(step*0x7FFF);
    float scalar = 2*(handle -> cfg.m_pi)/((float)LUT_size);
    for(uint16_t i = 0; i < (LUT_size); i++) {
      angle = scalar*i;
      LUT[i] = (int16_t)(sinf(angle)*0x7FFF);
    }
  } else r = 1;
  return r;
}


// int8_t get_twiddle(struct fft_handle_t *handle, uint16_t index, int16_t *LUT_Q15, int32_t *w_ri) {
//   int8_t r = 0;
//   float angle = 0;
//   float scalar = 2*(handle -> cfg.m_pi)/((float)handle->cfg.smpl_size);
//   angle = index*scalar;
//   float w_sin = sinf(angle);
//   float w_cos = cosf(angle);

//   int16_t w_i = w_sin*0x7FFF;
//   int16_t w_r = w_cos*0x7FFF;

//   *w_ri = __PKHBT((int16_t)w_r, (int16_t)w_i, 16);
//   return r;


// }



int8_t get_twiddle(struct fft_handle_t *handle, uint16_t index, int16_t *LUT_Q15, int32_t *w_ri) {
  int8_t r = 0;
  if(handle != NULL) {

    uint32_t N = handle -> cfg.smpl_size;
    uint32_t N_LUT = handle -> cfg.lut_size;

    if(N <= N_LUT) {
      uint8_t lut_step_skip = handle -> cfg.lut_step_skip;
      uint8_t quarter_lut = handle -> cfg.quarter_lut;
      uint16_t idx = (lut_step_skip * index) & (N_LUT-1);
      uint16_t cos_idx = (idx + quarter_lut) & (N_LUT-1);
      int16_t sin_val_Q15 = LUT_Q15[idx];
      int16_t cos_val_Q15 = LUT_Q15[cos_idx];
      *w_ri = __PKHBT((int16_t)cos_val_Q15, (int16_t)sin_val_Q15, 16);
    } else {


      int16_t lut_step_Q15 = handle -> cfg.lut_step;

      // Compute the Q15 fixed-point angle corresponding to the given FFT index.
      // The full circle (2π) is mapped to 0x7FFF in Q15. We scale the index accordingly.
      uint32_t angle_Q15 = ((uint32_t)(index * lut_step_Q15 * N_LUT)) / N;

      // Compute LUT index and delta (error between actual angle and LUT angle).
      uint16_t idx = angle_Q15/(handle->cfg.lut_step);
      int16_t delta_Q15 = angle_Q15 - (idx*lut_step_Q15);

      // Use sin(angle) = LUT[idx] and cos(angle) = LUT[(idx + N_LUT/4) % N_LUT]
      uint16_t idx_sin = idx % N_LUT;
      uint16_t idx_cos = (idx + (N_LUT/4)) % N_LUT;

      int16_t sin_val_Q15 = LUT_Q15[idx_sin];
      int16_t cos_val_Q15 = LUT_Q15[idx_cos];

      //Rescaling is performed to allow for higer precision. delta is saved as Q18 instead.
      //This can be allowed as the maximum error will be the LUT resolution i.e. 804 for 256 value LUT.
      //This is rescaled to 2*2*2*804 -> 6432.
      //This is the max error, this is then squared for delta2: 41370624, and 
      //right shifted 15 times, making it Q18*Q18/Q15 = Q21
      //This will have a value of 41370624/32767 = 1262 (Max error squared in Q21)
      delta_Q15 = ((int32_t)delta_Q15<<3);
      int32_t delta2 = ((int32_t)delta_Q15*delta_Q15) >>15;
      int32_t delta2_half = delta2 >> 1;     

      //We must account for the fact that delta_Q15 is currently in Q18, and cos_val is in Q15:
      //Their product is then Q18*Q15 = Q33, we remove the lower 18 bits getting Q15 back.
      //The same is done for delta2, this is however Q21, thus to rescale to Q15 we must remove 21
      //bits from the product - This allows for increased resolution without any overflow.
      int32_t w_i = sin_val_Q15 + ((int32_t)(delta_Q15*cos_val_Q15) >>18) - ((delta2_half * sin_val_Q15)>>21);
      int32_t w_r = cos_val_Q15 - ((int32_t)(delta_Q15*sin_val_Q15) >>18) - ((delta2_half * cos_val_Q15)>>21);

      w_i = __SSAT(w_i, 16);
      w_r = __SSAT(w_r, 16);

      *w_ri = __PKHBT((int16_t)w_r, (int16_t)w_i, 16);
    }
  } else r = -1;
  return r;
}

int8_t get_twiddle_linear(struct fft_handle_t *handle, uint16_t index, int16_t *LUT_Q15, int32_t *w_ri) {
  int8_t r = 0;
  if(handle != NULL) {

    uint32_t N = handle -> cfg.smpl_size;
    uint32_t N_LUT = handle -> cfg.lut_size;
    int16_t lut_step_Q15 = handle -> cfg.lut_step;

    // Compute the Q15 fixed-point angle corresponding to the given FFT index.
    // The full circle (2π) is mapped to 0x7FFF in Q15. We scale the index accordingly.
    uint32_t angle_Q15 = ((uint32_t)(index * lut_step_Q15 * N_LUT)) / N;

    // Compute LUT index and delta (error between actual angle and LUT angle).
    uint16_t idx = angle_Q15/(handle->cfg.lut_step);
    int16_t delta_Q15 = angle_Q15 - (idx*lut_step_Q15);

    // Use sin(angle) = LUT[idx] and cos(angle) = LUT[(idx + N_LUT/4) % N_LUT]
    uint16_t idx_sin = idx % N_LUT;
    uint16_t idx_cos = (idx + (N_LUT/4)) % N_LUT;

    int16_t sin_val_Q15 = LUT_Q15[idx_sin];
    int16_t cos_val_Q15 = LUT_Q15[idx_cos];

    if(N<= N_LUT) {
      *w_ri = __PKHBT((int16_t)cos_val_Q15, (int16_t)sin_val_Q15, 16);
    } else {

      //Rescaling is performed to allow for higer precision. delta is saved as Q18 instead.
      //This can be allowed as the maximum error will be the LUT resolution i.e. 804 for 256 value LUT.
      //This is rescaled to 2*2*2*804 -> 6432.
      //This is the max error, this is then squared for delta2: 41370624, and 
      //right shifted 15 times, making it Q18*Q18/Q15 = Q21
      //This will have a value of 41370624/32767 = 1262 (Max error squared in Q21)
      delta_Q15 = ((int32_t)delta_Q15<<3);
      // int32_t delta2 = ((int32_t)delta_Q15*delta_Q15) >>15;
      // int32_t delta2_half = delta2 >> 1;     

      //We must account for the fact that delta_Q15 is currently in Q18, and cos_val is in Q15:
      //Their product is then Q18*Q15 = Q33, we remove the lower 18 bits getting Q15 back.
      //The same is done for delta2, this is however Q21, thus to rescale to Q15 we must remove 21
      //bits from the product - This allows for increased resolution without any overflow.
      // int32_t w_i = sin_val_Q15 + ((int32_t)(delta_Q15*cos_val_Q15) >>18) - ((delta2_half * sin_val_Q15)>>21);
      // int32_t w_r = cos_val_Q15 - ((int32_t)(delta_Q15*sin_val_Q15) >>18) - ((delta2_half * cos_val_Q15)>>21);
      int32_t w_i = sin_val_Q15 + ((int32_t)(delta_Q15*cos_val_Q15)>>18);
      int32_t w_r = cos_val_Q15 - ((int32_t)(delta_Q15*sin_val_Q15)>>18);

      w_i = __SSAT(w_i, 16);
      w_r = __SSAT(w_r, 16);

      *w_ri = __PKHBT((int16_t)w_r, (int16_t)w_i, 16);
    }
  } else r = -1;
  return r;
}



uint16_t bit_reverse(uint8_t l, uint16_t x){
    x = ((x & 0x55555555) << 1) | ((x & 0xAAAAAAAA) >> 1);
    x = ((x & 0x33333333) << 2) | ((x & 0xCCCCCCCC) >> 2);
    x = ((x & 0x0F0F0F0F) << 4) | ((x & 0xF0F0F0F0) >> 4);
    x = ((x & 0x00FF00FF) << 8) | ((x & 0xFF00FF00) >> 8);
    x = x>>(16-l);
    return x;
}

int8_t fft_reorder(struct fft_handle_t *handle, int32_t*data) {
  //__RBIT(x) will return the bit reverse of the input, 32 bit. Can be used for ARM processors.
  int8_t r = 0;
  if(handle != NULL) {
    uint16_t rev_idx = 0;
    uint16_t N = handle -> cfg.smpl_size;
    uint8_t level = handle -> cfg.fft_level;
    int32_t buf = 0;

    for(uint16_t i = 0; i < N; i++) {
      rev_idx = handle -> cfg.bit_reverse(level, i);

      if(i < rev_idx) {
        buf = data[i];
        data[i] = data[rev_idx];
        data[rev_idx] = buf; 
      }
    }
  } else r = -1;
  return r;
}

int8_t fft_window(struct fft_handle_t *handle, int32_t *data, windows_t W) {
  int8_t r = 0;
  if(handle != NULL) {
    float m_pi = handle->cfg.m_pi;
    uint16_t N = handle->cfg.smpl_size;
    float gain = 1;
    switch(W) {
      case RECTANGULAR:
      __asm__("nop");
      break;

      case BARTLETT:
      bartlett_win(data, N);
      gain = 2.0;
      break;

      case HAMMING:
      hamming_win(data, N, m_pi);
      gain = 1.8557;
      break;

      case HANN:
      hann_win(data, N, m_pi);
      gain = 2.0;
      break;

      case IMPERIAL:
      imperial_win(data, N, m_pi);
      gain = 1.69638;
      break;

      case HIGH_IMPERIAL:
      high_imperial_win(data, N, m_pi);
      gain = 1.95414;
      break;

      case FLAT_TOP:
      flat_top_win(data, N, m_pi);
      gain = 4.63865;
      break;

      case BLACKMAN:
      blackman_win(data, N, m_pi);
      gain = 2.36233;
      break;

      case BLACKMAN_HARRIS:
      blackman_harris_win(data, N, m_pi);
      gain = 2.8094;
      break;
      

    }
  handle -> cfg.win = W;
  handle -> cfg.window_gain_correction = gain;
  } else r = -1;
  return r;
}


void bartlett_win(int32_t *data, uint16_t N) {
  uint16_t N1 = N-1;
  uint16_t Nc = (N/2)-1;
  float w = 0;
  float buf_data = 0;
  float buf_data2 = 0;
  float scalar = 2.0/(N1);
  for(uint16_t n = 0; n < Nc; n++) {
    buf_data = (float)(data[n])/32767.0;
    buf_data2 = (float)(data[N1-n])/32767.0;
    w = scalar*n;
    data[n] = int16_t(0x7FFF*w*buf_data);
    data[N1-n] = int16_t(0x7FFF*w*buf_data2);
  }
}

void hamming_win(int32_t *data, uint16_t N, float m_pi) {
  uint16_t N1 = N-1;
  uint16_t Nc = (N/2)-1;
  float w = 0;
  float buf_data = 0;
  float buf_data2 = 0;
  float scalar = (m_pi)*2.0/N1;
  for(uint16_t n = 0; n < Nc; n++) {
    buf_data = (float)(data[n])/32767.0;
    buf_data2 = (float)(data[N1-n])/32767.0;
    w = 0.54-0.46*cosf(n*scalar);
    data[n] = int16_t(0x7FFF*w*buf_data);
    data[N1-n] = int16_t(0x7FFF*w*buf_data2);
  }
}

void hann_win(int32_t *data, uint16_t N, float m_pi) {
  uint16_t N1 = N-1;
  uint16_t Nc = (N/2)-1;
  float w = 0;
  float buf_data = 0;
  float buf_data2 = 0;
  float scalar = (m_pi)*2.0/N1;
  for(uint16_t n = 0; n < Nc; n++) {
    buf_data = (float)(data[n])/32767.0;
    buf_data2 = (float)(data[N1-n])/32767.0;
    w = 0.5-0.5*cosf(n*scalar);
    data[n] = int16_t(0x7FFF*w*buf_data);
    data[N1-n] = int16_t(0x7FFF*w*buf_data2);
  }
}

void imperial_win(int32_t *data, uint16_t N, float m_pi) {
  uint16_t N1 = N-1;
  uint16_t Nc = (N/2)-1;
  float w = 0;
  float buf_data = 0;
  float buf_data2 = 0;
  float scalar = 2*m_pi/((float)N);
  float scalar2 = 0;
  for(uint16_t n = 0; n < Nc; n++) {
    buf_data = (float)(data[n])/32767.0;
    buf_data2 = (float)(data[N1-n])/32767.0;
    scalar2 = scalar*n-m_pi;
    w = sinf(scalar2)/(scalar2);
    data[n] = int16_t(0x7FFF*w*buf_data);
    data[N1-n] = int16_t(0x7FFF*w*buf_data2);
  }
}

void high_imperial_win(int32_t *data, uint16_t N, float m_pi) {
  uint16_t N1 = N-1;
  uint16_t Nc = (N/2)-1;
  float w = 0;
  float buf_data = 0;
  float buf_data2 = 0;
  float k1 = -0.1978312*N;
  float k2 = -1*m_pi*8/(3*N);
  float k3 = m_pi/3.0;
  float k4 = 0.1713268*N;
  float k5 = 0.3426536;
  for(uint16_t n = 0; n<Nc; n++) {
    buf_data = (float)(data[n]/32767.0);
    buf_data2 = (float)(data[N1-n])/32767.0;
    w = (k1*sinf(k2*n+k3)+k4-k5*n)/(-2*n+N);
    data[n] = int16_t(0x7FFF*w*buf_data);
    data[N1-n] = int16_t(0x7FFF*w*buf_data2);
  }
}

void flat_top_win(int32_t *data, uint16_t N, float m_pi) {
  uint16_t N1 = N-1;
  uint16_t Nc = (N/2)-1;
  float w = 0;
  float buf_data = 0;
  float buf_data2 = 0;
  float N1f = (float)N1;
  float scalar[4] = {(float)(2.0*m_pi/N1f), (float)(4.0*m_pi/N1f), (float)(6.0*m_pi/N1f), (float)(8.0*m_pi/N1f)};
  float coeff[5] = {0.21558, 0.41663, 0.27726, 0.083579, 0.0069474};
  for(uint16_t n = 0; n < Nc; n++) {
    buf_data = (float)(data[n])/32767.0;
    buf_data2 = (float)(data[N1-n])/32767.0;
    w = coeff[0]-(coeff[1]*cosf(scalar[0]*n))+(coeff[2]*cosf(scalar[1]*n))-(coeff[3]*cosf(scalar[2]*n))+(coeff[4]*cosf(scalar[3]*n));
    data[n] = int16_t(0x7FFF*w*buf_data);
    data[N1-n] = int16_t(0x7FFF*w*buf_data2);
  }
}

void blackman_win(int32_t *data, uint16_t N, float m_pi) {
  uint16_t N1 = N-1;
  uint16_t Nc = (N/2)-1;
  float w = 0;
  float buf_data = 0;
  float buf_data2 = 0;
  float N1f = (float)N1;
  float scalar[2] = {(float)(2.0*m_pi/N1f), (float)(4.0*m_pi/N1f)};
  float coeff[3] = {0.42659, 0.49656, 0.076849};
  for(uint16_t n = 0; n < Nc; n++) {
    buf_data = (float)(data[n])/32767.0;
    buf_data2 = (float)(data[N1-n])/32767.0;
    w = coeff[0]-(coeff[1]*cosf(scalar[0]*n))+(coeff[2]*cosf(scalar[1]*n));
    data[n] = int16_t(0x7FFF*w*buf_data);
    data[N1-n] = int16_t(0x7FFF*w*buf_data2);
  }
}

void blackman_harris_win(int32_t *data, uint16_t N, float m_pi) {
  uint16_t N1 = N-1;
  uint16_t Nc = (N/2)-1;
  float w = 0;
  float buf_data = 0;
  float buf_data2 = 0;
  float N1f = (float)N1;
  float scalar[3] = {(float)(2.0*m_pi/N1f), (float)(4.0*m_pi/N1f), (float)(6.0*m_pi/N1f)};
  float coeff[4] = {0.35875, 0.48829, 0.14128, 0.01168};
  for(uint16_t n = 0; n < Nc; n++) {
    buf_data = (float)(data[n])/32767.0;
    buf_data2 = (float)(data[N1-n])/32767.0;
    w = coeff[0]-(coeff[1]*cosf(scalar[0]*n))+(coeff[2]*cosf(scalar[1]*n))-(coeff[3]*cosf(scalar[2]*n));
    data[n] = int16_t(0x7FFF*w*buf_data);
    data[N1-n] = int16_t(0x7FFF*w*buf_data2);
  }
}


int8_t run_fft_cb(struct fft_handle_t *handle, int32_t *data, int16_t *LUT) {
  int8_t r = 0;
  
  if(handle != NULL) {
    int16_t ja = 0;
    int16_t jb = 0;
    uint16_t TwAddr = 0;
    uint16_t mask = 0;

    int32_t buf_r = 0;
    int32_t buf_i = 0;
    int32_t a_val = 0;
    int32_t b_val = 0;
    int16_t ar = 0;
    int16_t ai = 0;

    int32_t x = 0;
    int32_t tw_apx = 0;

    int16_t out_r_ja = 0;
    int16_t out_i_ja = 0;

    int16_t out_r_jb = 0;
    int16_t out_i_jb = 0;

    uint8_t level = handle -> cfg.fft_level;
    uint16_t N = handle -> cfg.smpl_size;

    uint16_t mask_a = handle -> cfg.mask_a;
    uint16_t mask_b = handle -> cfg.mask_b;

    for(uint8_t i = 0; i<level; i++) {
      for(uint16_t j = 0; j < (N/2); j++) {
        ja = j<<1;
        jb = ja+1;

        ja = ((ja<<i)|(ja>>(level-i))) & mask_a;
        jb = ((jb<<i)|(jb>>(level-i))) & mask_a;
        TwAddr = (((~mask_b)>>i) & mask_b) & j;

        handle -> cfg.get_twiddle(handle, TwAddr, LUT, &tw_apx);
        // tw_apx = LUT[TwAddr];

        x = data[jb];

        buf_r = (__SMUSD(x, tw_apx));
        buf_i = (__SMUADX(x, tw_apx));

        buf_r >>=15;
        buf_i >>=15;

        buf_r = __SSAT(buf_r, 16);
        buf_i = __SSAT(buf_i, 16);

        a_val = data[ja];
        b_val = __PKHBT(buf_r, buf_i, 16);
        
        data[jb] = __SHSUB16(data[ja], b_val);
        data[ja] = __SHADD16(data[ja], b_val);


        // ar = (int16_t)(a_val&0x0000FFFF);
        // ai = (int16_t)(a_val>>16);

        // out_r_jb = __SSAT(((int32_t)ar - buf_r)>>1, 16);
        // out_i_jb = __SSAT(((int32_t)ai - buf_i)>>1, 16);

        // out_r_ja = __SSAT(((int32_t)ar + buf_r)>>1, 16);
        // out_i_ja = __SSAT(((int32_t)ai + buf_i)>>1, 16);

        // data[jb] = __PKHBT(out_r_jb, out_i_jb, 16);
        // data[ja] = __PKHBT(out_r_ja, out_i_ja, 16);


      }
    }
 
  } else r = -1;
  return r;
}


int8_t run_fft_linear_cb(struct fft_handle_t *handle, int32_t *data, int16_t *LUT) {
  int8_t r = 0;
  
  if(handle != NULL) {
    int16_t ja = 0;
    int16_t jb = 0;
    uint16_t TwAddr = 0;
    uint16_t mask = 0;

    int32_t buf_r = 0;
    int32_t buf_i = 0;
    int32_t a_val = 0;
    int32_t b_val = 0;
    int16_t ar = 0;
    int16_t ai = 0;

    int32_t x = 0;
    int32_t tw_apx = 0;

    int16_t out_r_ja = 0;
    int16_t out_i_ja = 0;

    int16_t out_r_jb = 0;
    int16_t out_i_jb = 0;

    uint8_t level = handle -> cfg.fft_level;
    uint16_t N = handle -> cfg.smpl_size;

    uint16_t mask_a = handle -> cfg.mask_a;
    uint16_t mask_b = handle -> cfg.mask_b;

    for(uint8_t i = 0; i<level; i++) {
      for(uint16_t j = 0; j < (N/2); j++) {
        ja = j<<1;
        jb = ja+1;

        ja = ((ja<<i)|(ja>>(level-i))) & mask_a;
        jb = ((jb<<i)|(jb>>(level-i))) & mask_a;
        TwAddr = (((~mask_b)>>i) & mask_b) & j;

        handle -> cfg.get_twiddle_linear(handle, TwAddr, LUT, &tw_apx);
        // tw_apx = tw_LUT[TwAddr];

        x = data[jb];

        buf_r = (__SMUSD(x, tw_apx));
        buf_i = (__SMUADX(x, tw_apx));

        buf_r >>=15;
        buf_i >>=15;

        a_val = data[ja];
        b_val = __PKHBT(buf_r, buf_i, 16);
        
        data[jb] = __SHSUB16(a_val, b_val);
        data[ja] = __SHADD16(a_val, b_val);
        // ar = (int16_t)(a_val&0x0000FFFF);
        // ai = (int16_t)(a_val>>16);

        // out_r_jb = __SSAT(((int32_t)ar - buf_r)>>1, 16);
        // out_i_jb = __SSAT(((int32_t)ai - buf_i)>>1, 16);

        // out_r_ja = __SSAT(((int32_t)ar + buf_r)>>1, 16);
        // out_i_ja = __SSAT(((int32_t)ai + buf_i)>>1, 16);

        // data[jb] = __PKHBT(out_r_jb, out_i_jb, 16);
        // data[ja] = __PKHBT(out_r_ja, out_i_ja, 16);

      }
    }
 
  } else r = -1;
  return r;
}



int8_t full_fft_w_mag_cb(struct fft_handle_t *handle, int32_t *data, int16_t *LUT, float *data_o, windows_t W, bool Linear) {
  int8_t r = 0;
  if(handle != NULL) {
    handle -> fft_window(handle, data, W);
    handle -> fft_reorder(handle, data);
    if(Linear == 0) {
      handle -> run_fft(handle, data, tw_lut);
    }
    else handle -> run_fft_linear(handle, data, tw_lut);
    handle -> fft_mag_db(handle, data, data_o);
  } else r = -1;
  return r;
}


int8_t fft_mag_cb(struct fft_handle_t *handle, int32_t *data) {
  int8_t r = 0;
  if(handle != NULL) {
    float re = 0;
    float im = 0;
    float mag = 0;
    float window_correction = (handle -> cfg.window_gain_correction)*2;
    uint16_t N = handle -> cfg.smpl_size;
    for(uint16_t i = 0; i < N; i++) {
      re = (float)((int16_t)(data[i]&0x0000FFFF))/32767.0;
      im = (float)((int16_t)(data[i]>>16))/32767.0;
      mag = sqrtf((re*re)+(im*im));
      mag *= window_correction;
      data[i] = __SSAT((int16_t)(mag*0x7FFF),16);
    }
  } else r = -1;
  return r;
}

int8_t fft_mag_db_cb(struct fft_handle_t *handle, int32_t *data_i, float *data_o) {
  int8_t r = 0;
  if(handle != NULL) {
    float re = 0;
    float im = 0;
    float mag = 0;
    uint16_t N = handle -> cfg.smpl_size;
    float window_correction = (handle -> cfg.window_gain_correction)*2;
    for(uint16_t i = 0; i < N; i++) {
      re = (float)((int16_t)(data_i[i]&0x0000FFFF))/32767.0;
      im = (float)((int16_t)(data_i[i]>>16))/32767.0;
      mag = sqrtf((re*re)+(im*im));
      mag *=window_correction;
      if(mag < 0.0001) mag = 0.0001;
      data_o[i] = 20*log10f(mag*1.0); //*4 to scale the 14 bit adc to 16 bit.
    }
  } else r = -1;
  return r;
}

