#ifndef _INTERFERER_HPP_
#define _INTERFERER_HPP_

#include <uhd/usrp/multi_usrp.hpp>
#include <liquid/liquid.h>
#include "timer.h"
#include <random>

#define DEFAULT_RUN_TIME 20
#define DEFAULT_CONTROLLER_IP_ADDRESS "192.168.1.56"

#define USRP_BUFFER_LENGTH 256
#define TX_BUFFER_LENGTH 5120

#define GMSK_PAYLOAD_LENGTH 50
#define GMSK_HEADER_LENGTH 8

#define RRC_SYMS_PER_FRAME 100
#define RRC_SAMPS_PER_SYM 2
#define RRC_FILTER_SEMILENGTH 32
#define RRC_BETA 0.35

#define OFDM_CP_LENGTH 16
#define OFDM_TAPER_LENGTH 6
#define OFDM_HEADER_LENGTH 64
#define OFDM_PAYLOAD_LENGTH 128

#define DAC_RATE 64e6

enum int_tx_states {
  INT_TX_STOPPED = 0,
  INT_TX_DUTY_CYCLE_ON,
  INT_TX_DUTY_CYCLE_OFF
};

void *Interferer_tx_worker(void *_arg);

class Interferer {
public:
  Interferer(/*string with name of CE_execute function*/);
  ~Interferer();

  // Interference parameters
  int interference_type;
  double tx_gain_soft; // soft transmit gain (linear)
  double tx_gain;
  double tx_freq;
  double tx_rate;

  double period;
  double duty_cycle;

  // frequency hopping parameters
  int tx_freq_behavior;
  double tx_freq_min;
  double tx_freq_max;
  double tx_freq_bandwidth;
  double tx_freq_dwell_time;
  double tx_freq_resolution;

  // timers
  timer duty_cycle_timer;
  timer freq_dwell_timer;
  
  // log parameters
  bool log_tx_flag;
  std::ofstream tx_log_file;
  char tx_log_file_name[100];
  
  //log file for samples
  std::ofstream sample_file;
  
  std::default_random_engine generator;
  std::normal_distribution<double> dist;

  // liquid-dsp objects
  resamp2_crcf interp;
  gmskframegen gmsk_fg;
  firfilt_crcf rrc_filt;
  ofdmflexframegenprops_s fgprops;
  ofdmflexframegen ofdm_fg;

  // RF objects and properties
  uhd::usrp::multi_usrp::sptr usrp_tx;
  uhd::tx_metadata_t metadata_tx;

  unsigned int buffered_samps;
  std::vector<std::complex<float> > tx_buffer;

  // transmit thread objects
  pthread_t tx_process;
  pthread_mutex_t tx_mutex;
  pthread_cond_t tx_cond;
  bool tx_running;
  bool tx_thread_running;
  friend void *Interferer_tx_worker(void *);

  int tx_state;

  void start_tx();
  void stop_tx();

  void set_log_file(char*);
  void log_tx_parameters();

  void UpdateFrequency();
  void TransmitInterference();

  void BuildCWTransmission();
  void BuildNOISETransmission();
  void BuildGMSKTransmission();
  void BuildRRCTransmission();
  void BuildOFDMTransmission();
  void BuildAWGNTransmission();
  
private:
};

#endif
