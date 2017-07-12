#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <uhd/usrp/multi_usrp.hpp>
#include <complex>
#include <liquid/liquid.h>
#include <fstream>
#include "interferer.hpp"
#include "crts.hpp"
#include "timer.h"
#include <random>

#define DEBUG 0
#if DEBUG == 1
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) /*__VA_ARGS__*/
#endif

Interferer::Interferer()
    :generator(), dist(5.0, 5.0)     // Gaussian parameter set up
{
  // set default parameters
  interference_type = INTERFERENCE_TYPE_RRC;
  period = 1.0;
  duty_cycle = 1.0;
  tx_freq = 765e6;
  tx_rate = 1e6;
  tx_gain_soft = -3.0;
  tx_freq_behavior = TX_FREQ_BEHAVIOR_FIXED;
  tx_freq_min = 765e6;
  tx_freq_max = 765e6;
  tx_freq_dwell_time = 1.0;
  tx_freq_resolution = 1e6;
  tx_freq_bandwidth = 0.0;

  //
  uhd::device_addr_t dev_addr;
  usrp_tx = uhd::usrp::multi_usrp::make(dev_addr);
  usrp_tx->set_tx_antenna("TX/RX", 0);

  // create and start tx thread
  tx_state = INT_TX_STOPPED;
  tx_thread_running = true;
  pthread_mutex_init(&tx_mutex, NULL);
  pthread_cond_init(&tx_cond, NULL);
  pthread_create(&tx_process, NULL, Interferer_tx_worker, (void *)this);

  // set USRP settings
  usrp_tx->set_tx_freq(tx_freq);
  usrp_tx->set_tx_rate(tx_rate);
  usrp_tx->set_tx_gain(tx_gain);

  // setup objects needed for signal generation
  tx_buffer.resize(2 * TX_BUFFER_LENGTH);
  gmsk_fg = gmskframegen_create();
  interp = resamp2_crcf_create(7, 0.0f, 40.0f);
  unsigned int h_len = 2 * RRC_SAMPS_PER_SYM * RRC_FILTER_SEMILENGTH + 1;
  float h[h_len];
  liquid_firdes_rrcos(RRC_SAMPS_PER_SYM, RRC_FILTER_SEMILENGTH, RRC_BETA, 0.0,
                      h);
  rrc_filt = firfilt_crcf_create(h, h_len);
  unsigned int num_subcarriers = 64;
  unsigned char *subcarrierAlloc = NULL;
  ofdmflexframegenprops_init_default(&fgprops);
  ofdm_fg =
      ofdmflexframegen_create(num_subcarriers, OFDM_CP_LENGTH,
                              OFDM_TAPER_LENGTH, subcarrierAlloc, &fgprops);

  sample_file.open("AWGN_samples.dat");

  // create timers
  duty_cycle_timer = timer_create();
  freq_dwell_timer = timer_create();
}

Interferer::~Interferer() {

  sample_file.close();
  tx_log_file.close();

  // join tx thread
  tx_thread_running = false;
  pthread_cond_signal(&tx_cond);
  void *tx_exit_status;
  pthread_join(tx_process, &tx_exit_status);
}

void Interferer::start_tx() {
  tx_state = INT_TX_DUTY_CYCLE_ON;
  pthread_cond_signal(&tx_cond);
}

void Interferer::stop_tx() { tx_state = INT_TX_STOPPED; }

void Interferer::set_log_file(char *log_file_name) {
  log_tx_flag = true;
  strcpy(tx_log_file_name, log_file_name);

  // If log file names use subdirectories, create them if they don't exist

  char *subdirptr = strrchr(tx_log_file_name, '/');
  if (subdirptr) {
    char subdirs[60];
    // Get the names of the subdirectories
    strncpy(subdirs, tx_log_file_name, subdirptr - tx_log_file_name);
    subdirs[subdirptr - tx_log_file_name] = '\0';
    char mkdir_cmd[100];
    strcpy(mkdir_cmd, "mkdir -p ./logs/bin/");
    strcat(mkdir_cmd, subdirs);
    // Create them
    system(mkdir_cmd);
  }

  // open tx log file to delete any current contents
  if (log_tx_flag) {
    tx_log_file.open(tx_log_file_name,
                     std::ofstream::out | std::ofstream::trunc);
    if (!tx_log_file.is_open()) {
      std::cout << "Error opening log file: " << tx_log_file_name << std::endl;
    }
  }
}

void Interferer::BuildCWTransmission() {
  for (unsigned int i = 0; i < tx_buffer.size(); i++) {
    tx_buffer[i].real(0.5);
    tx_buffer[i].imag(0.5);
  }
  buffered_samps = tx_buffer.size();
}

void Interferer::BuildNOISETransmission() {
  for (unsigned int i = 0; i < tx_buffer.size(); i++) {
    tx_buffer[i].real(0.5 * (float)rand() / (float)RAND_MAX - 0.25);
    tx_buffer[i].imag(0.5 * (float)rand() / (float)RAND_MAX - 0.25);
  }
  buffered_samps = tx_buffer.size();
  }
  
  void Interferer::BuildAWGNTransmission() {
  for (unsigned int i = 0; i < tx_buffer.size(); i++) {
       tx_buffer[i].real (dist(generator));
    tx_buffer[i].imag (dist(generator));
    if(sample_file.is_open())
    {
        sample_file << tx_buffer[i].real() << " + " << tx_buffer[i].imag() << "i" << std::endl;
    }
  }
  buffered_samps = tx_buffer.size();
}

// ========================================================================f
//  FUNCTION:  Build GMSK Transmission
//
//  Based upon liquid-usrp gmskframe_tx.cc
// ========================================================================
void Interferer::BuildGMSKTransmission() {
  crc_scheme gmskCrcScheme = LIQUID_CRC_16;
  fec_scheme gmskFecSchemeInner = LIQUID_FEC_NONE;
  fec_scheme gmskFecSchemeOuter = LIQUID_FEC_HAMMING74;

  // header and payload for frame generators
  unsigned char header[GMSK_HEADER_LENGTH];
  unsigned char payload[GMSK_PAYLOAD_LENGTH];

  // generate a random header and payload
  for (unsigned int j = 0; j < GMSK_HEADER_LENGTH; j++) {
    header[j] = rand() & 0xff;
  }
  for (unsigned int j = 0; j < GMSK_PAYLOAD_LENGTH; j++) {
    payload[j] = rand() & 0xff;
  }

  // generate frame
  gmskframegen_assemble(gmsk_fg, header, payload, GMSK_PAYLOAD_LENGTH,
                        gmskCrcScheme, gmskFecSchemeInner, gmskFecSchemeOuter);

  // set up framing buffers
  unsigned int k = 2;
  std::complex<float> buffer[k];
  std::complex<float> buffer_interp[2 * k];
  buffered_samps = 0;

  // calculate soft gain
  float g = powf(10.0f, tx_gain_soft / 20.0f);

  int frame_complete = 0;

  // generate frame
  while (!frame_complete) {

    // generate k samples
    frame_complete = gmskframegen_write_samples(gmsk_fg, buffer);

    // interpolate by 2
    for (unsigned int j = 0; j < k; j++) {
      resamp2_crcf_interp_execute(interp, buffer[j], &buffer_interp[2 * j]);
    }

    // apply gain
    for (unsigned int j = 0; j < 2 * k; j++) {
      tx_buffer[buffered_samps++] = g * buffer_interp[j];
    }
  }

  // Add zero-padding for the frame. This is important to allow the
  // resampler filters to go back to a relaxed state
  unsigned int padding = 6;
  for (unsigned int i = 0; i < padding; i++) {
    resamp2_crcf_interp_execute(interp, 0.0f, &buffer_interp[2 * i]);
    for (unsigned int j = 0; j < 2; j++) {
      tx_buffer[buffered_samps++] = buffer_interp[j];
    }
  }
}

// ========================================================================
//  FUNCTION:  Build RRC Transmission
// ========================================================================

void Interferer::BuildRRCTransmission() {
  std::complex<float> complex_symbol;
  unsigned int h_len = 2 * RRC_SAMPS_PER_SYM * RRC_FILTER_SEMILENGTH + 1;
  const int samps_per_frame = RRC_SYMS_PER_FRAME * RRC_SAMPS_PER_SYM;
  buffered_samps = 0;

  firfilt_crcf_reset(rrc_filt);

  for (unsigned int j = 0; j < samps_per_frame; j++) {
    // generate a random QPSK symbol until within a filter length of the end
    if ((j % RRC_SAMPS_PER_SYM == 0) &&
        (buffered_samps < samps_per_frame - 2 * h_len)) {
      complex_symbol.real(0.5 * (float)roundf((float)rand() / (float)RAND_MAX) -
                          0.25);
      complex_symbol.imag(0.5 * (float)roundf((float)rand() / (float)RAND_MAX) -
                          0.25);
    }
    // zero insert to interpolate
    else {
      complex_symbol.real(0.0);
      complex_symbol.imag(0.0);
    }

    firfilt_crcf_push(rrc_filt, complex_symbol);
    firfilt_crcf_execute(rrc_filt, &tx_buffer[j]);

    buffered_samps++;
  }
}

// ========================================================================
//  FUNCTION:  Build OFDM Transmission
// ========================================================================

void Interferer::BuildOFDMTransmission() {
  unsigned int num_subcarriers = 2 * (unsigned int)(tx_rate / 30e3);
  unsigned int cp_len = OFDM_CP_LENGTH;

  // header and payload for frame generators
  unsigned char header[OFDM_HEADER_LENGTH];
  unsigned char payload[OFDM_PAYLOAD_LENGTH];

  ofdmflexframegen_assemble(ofdm_fg, header, payload, OFDM_PAYLOAD_LENGTH);

  buffered_samps = 0;
  int frame_complete = 0;
  int j = 0;
  while (!frame_complete &&
         (unsigned int)buffered_samps <
             TX_BUFFER_LENGTH - num_subcarriers - OFDM_CP_LENGTH) {
    frame_complete = ofdmflexframegen_writesymbol(
        ofdm_fg, &tx_buffer[j * (num_subcarriers + cp_len)]);
    buffered_samps += num_subcarriers + cp_len;
    j++;
  }

  // calculate soft gain
  float g = powf(10.0f, tx_gain_soft / 20.0f);

  // reduce amplitude of signal to avoid clipping
  for (unsigned int j = 0; j < buffered_samps; j++) {
    tx_buffer[j] *= g;
  }
}

// ========================================================================
//  FUNCTION:  Log transmission parameters
// ========================================================================
void Interferer::log_tx_parameters() {

  // update current time
  struct timeval tv;
  gettimeofday(&tv, NULL);

  if (tx_log_file.is_open()) {
    tx_log_file.write((char *)&tv, sizeof(tv));
    tx_log_file.write((char *)&tx_freq, sizeof(tx_freq));
  } else {
    std::cerr << "Error opening log file:" << tx_log_file_name << std::endl;
  }
}

// ========================================================================
//  FUNCTION:  Transmit Interference
// ========================================================================

void Interferer::TransmitInterference() {
  unsigned int tx_samp_count = 0;
  unsigned int usrp_samps = USRP_BUFFER_LENGTH;

  if (log_tx_flag)
    log_tx_parameters();

  while (tx_samp_count < buffered_samps) {
    if (buffered_samps - tx_samp_count <= USRP_BUFFER_LENGTH)
      usrp_samps = buffered_samps - tx_samp_count;

    usrp_tx->get_device()->send(&tx_buffer[tx_samp_count], usrp_samps,
                                metadata_tx, uhd::io_type_t::COMPLEX_FLOAT32,
                                uhd::device::SEND_MODE_FULL_BUFF);

    // update number of tx samples remaining
    tx_samp_count += USRP_BUFFER_LENGTH;
  }
}

// ========================================================================
//  FUNCTION:  Set Tx Freq for Frequency Hopping
// ========================================================================
void Interferer::UpdateFrequency() {

  static float tx_freq_coeff = 1.0;

  switch (tx_freq_behavior) {
  case (TX_FREQ_BEHAVIOR_SWEEP):
    tx_freq += (tx_freq_resolution * tx_freq_coeff);
    if ((tx_freq > tx_freq_max) || (tx_freq < tx_freq_min)) {
      tx_freq_coeff = tx_freq_coeff * -1.0;
      tx_freq = tx_freq + (2.0 * tx_freq_resolution * tx_freq_coeff);
    }
    break;
  case (TX_FREQ_BEHAVIOR_RANDOM):
    tx_freq =
        tx_freq_resolution * roundf((double)(rand() % (int)tx_freq_bandwidth) /
                                    tx_freq_resolution) +
        tx_freq_min;
    dprintf("Set transmit frequency to %.0f\n", tx_freq);
    break;
  }
  usrp_tx->set_tx_freq(tx_freq);
}

// ========================================================================
//  FUNCTION:
// ========================================================================
void *Interferer_tx_worker(void *_arg) {

  // typecast input
  Interferer *Int = (Interferer *)_arg;

  while (Int->tx_thread_running) {

    // wait for condition to start transmitting
    pthread_mutex_lock(&(Int->tx_mutex));
    pthread_cond_wait(&(Int->tx_cond), &(Int->tx_mutex));
    pthread_mutex_unlock(&(Int->tx_mutex));

    // initialize timers
    timer_tic(Int->duty_cycle_timer);
    timer_tic(Int->freq_dwell_timer);

    // send start of burst packet
    Int->metadata_tx.start_of_burst = true;
    Int->metadata_tx.end_of_burst = false;
    Int->metadata_tx.has_time_spec = false;
    Int->usrp_tx->get_device()->send(&Int->tx_buffer[0], 0, Int->metadata_tx,
                                     uhd::io_type_t::COMPLEX_FLOAT32,
                                     uhd::device::SEND_MODE_FULL_BUFF);
    Int->metadata_tx.start_of_burst = false;

    // run transmitter
    while (Int->tx_state != INT_TX_STOPPED) {
      // determine if we need to freq hop
      if ((Int->tx_freq_behavior != (TX_FREQ_BEHAVIOR_FIXED)) &&
          (timer_toc(Int->freq_dwell_timer) >= Int->tx_freq_dwell_time)) {
        timer_tic(Int->freq_dwell_timer);
        Int->UpdateFrequency();
      }
      // determine if we need to change state
      if (Int->tx_state == INT_TX_DUTY_CYCLE_ON &&
          timer_toc(Int->duty_cycle_timer) >= Int->duty_cycle * Int->period) {
        timer_tic(Int->duty_cycle_timer);
        dprintf("Turning off\n");
        Int->tx_state = INT_TX_DUTY_CYCLE_OFF;

        // send end of burst packet
        Int->metadata_tx.end_of_burst = true;
        Int->usrp_tx->get_device()->send("", 0, Int->metadata_tx,
                                         uhd::io_type_t::COMPLEX_FLOAT32,
                                         uhd::device::SEND_MODE_FULL_BUFF);
      }
      if (Int->tx_state == INT_TX_DUTY_CYCLE_OFF &&
          timer_toc(Int->duty_cycle_timer) >=
              (1.0 - Int->duty_cycle) * Int->period) {
        timer_tic(Int->duty_cycle_timer);
        dprintf("Turning on\n");
        Int->tx_state = INT_TX_DUTY_CYCLE_ON;

        // send start of burst packet
        Int->metadata_tx.start_of_burst = true;
        Int->metadata_tx.end_of_burst = false;
        Int->usrp_tx->get_device()->send(
            &Int->tx_buffer[0], 0, Int->metadata_tx,
            uhd::io_type_t::COMPLEX_FLOAT32, uhd::device::SEND_MODE_FULL_BUFF);
        Int->metadata_tx.start_of_burst = false;
      }

      // generate frame and transmit if in the on state
      if (Int->tx_state == INT_TX_DUTY_CYCLE_ON) {
        switch (Int->interference_type) {
        case (INTERFERENCE_TYPE_CW):
          Int->BuildCWTransmission();
          break;
        case (INTERFERENCE_TYPE_NOISE):
          Int->BuildNOISETransmission();
          break;
        case (INTERFERENCE_TYPE_GMSK):
          Int->BuildGMSKTransmission();
          break;
        case (INTERFERENCE_TYPE_RRC):
          Int->BuildRRCTransmission();
          break;
        case (INTERFERENCE_TYPE_OFDM):
          Int->BuildOFDMTransmission();
          break;
        case (INTERFERENCE_TYPE_AWGN):
          Int->BuildAWGNTransmission();
          break;
        }

        Int->TransmitInterference();
      }
    } // while tx_running
    dprintf("tx_worker finished running");
  } // while tx_thread_running
  dprintf("tx_worker exiting thread\n");
  pthread_exit(NULL);
}
