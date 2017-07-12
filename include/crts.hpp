#ifndef _CRTS_HPP_
#define _CRTS_HPP_

/////////////////////////////////////////////////////////////////
// Command-line style arguments
/////////////////////////////////////////////////////////////////

void str2argcargv(char *string, char *progName, int &argc, char (**&argv));

void freeargcargv(int &argc, char **&argv);

/////////////////////////////////////////////////////////////////
// Config files
/////////////////////////////////////////////////////////////////

void read_master_parameters(char *nameMasterScenFile, 
                            int *num_scenarios, 
                            bool *octave_log_summary); 

int read_master_scenario(char * nameMasterScenFile, int scenario_num,
                              char * scenario_name);

struct scenario_parameters read_scenario_parameters(char *scenario_file);

struct node_parameters read_node_parameters(int node, char *scenario_file);

//////////////////////////////////////////////////////////////////
// Scenario parameters
//////////////////////////////////////////////////////////////////

struct scenario_parameters {

  // Number of nodes in the scenario
  int num_nodes;

  // The start time of the scenario
  //time_t start_time_s;
  int64_t start_time_s;

  // The length of time to run the scenario
  //time_t runTime;
  int64_t run_time;

  // Total number of times this scenario
  // will be run
  unsigned int total_num_reps;

  // The repetition number of this scenario instance
  // i.e. 1 <= rep_num <= total_num_reps
  unsigned int rep_num;

  // Scenario controller
  char SC[100];
  float sc_timeout_ms;
  char sc_args[2048];
};

//////////////////////////////////////////////////////////////////
// Node parameters
//////////////////////////////////////////////////////////////////

enum NodeType {
  COGNITIVE_RADIO = 0,    // cognitive radio node type
  INTERFERER // interferer node type
};

enum COGNITIVE_RADIO_TYPE {
  PYTHON = 0, // third party python radios
  EXTENSIBLE_COGNITIVE_RADIO // Radios created using ECR
};

enum net_traffic_type {
  NET_TRAFFIC_STREAM = 0,
  NET_TRAFFIC_BURST,
  NET_TRAFFIC_POISSON,
  NET_TRAFFIC_UNKNOWN
};

enum interference_type {
  INTERFERENCE_TYPE_CW = 0, // continuous-wave interference
  INTERFERENCE_TYPE_NOISE,  // random noise interference
  INTERFERENCE_TYPE_GMSK,   // gaussian minimum-shift keying inteference
  INTERFERENCE_TYPE_RRC,    // root-raised cosine interference (as in WCDMA)
  INTERFERENCE_TYPE_OFDM,    // orthogonal frequency division multiplexing interference
  INTERFERENCE_TYPE_AWGN,
  INTERFERENCE_TYPE_UNKNOWN
};

enum tx_freq_behavior { 
  TX_FREQ_BEHAVIOR_FIXED = 0, 
  TX_FREQ_BEHAVIOR_SWEEP, 
  TX_FREQ_BEHAVIOR_RANDOM,
  TX_FREQ_BEHAVIOR_UNKNOWN
};

enum subcarrier_alloc {
  LIQUID_DEFAULT_SUBCARRIER_ALLOC = 0,
  STANDARD_SUBCARRIER_ALLOC,
  CUSTOM_SUBCARRIER_ALLOC
};

struct node_parameters {
  // general
  int node_type;
  int cognitive_radio_type;
  char python_file[100];
  char python_args[2048];
  char team_name[200];
  
  // network settings
  char server_ip[20];
  char crts_ip[20];
  char target_ip[20];
  int net_traffic_type;
  int net_burst_length;
  double net_mean_throughput;
  
  // cognitive engine settings
  char cognitive_engine[100];
  double ce_timeout_ms;
  char ce_args[2048]; 
  
  // log/print settings
  bool print_rx_frame_metrics;
  bool log_phy_rx;
  bool log_phy_tx;
  bool log_net_rx;
  bool log_net_tx;
  char phy_rx_log_file[260];
  char phy_tx_log_file[260];
  char net_rx_log_file[260];
  char net_tx_log_file[260];
  int generate_octave_logs; 

  // USRP settings
  double rx_freq;
  double rx_rate;
  double rx_gain;
  double tx_freq;
  double tx_rate;
  double tx_gain;

  // liquid OFDM settings
  int rx_subcarriers;
  int rx_cp_len;
  int rx_taper_len;
  int rx_subcarrier_alloc_method;
  int rx_guard_subcarriers;
  int rx_central_nulls;
  int rx_pilot_freq;
  char rx_subcarrier_alloc[2048];

  double tx_gain_soft;
  int tx_subcarriers;
  int tx_cp_len;
  int tx_taper_len;
  int tx_modulation;
  int tx_crc;
  int tx_fec0;
  int tx_fec1;
  int tx_subcarrier_alloc_method;
  int tx_guard_subcarriers;
  int tx_central_nulls;
  int tx_pilot_freq;
  char tx_subcarrier_alloc[2048];

  // interferer only
  int interference_type; // see ENUM list above
  double period;          // seconds for a single period
  double duty_cycle;      // percent of period that interference
                         // is ON.  expressed as a float
                         // between 0.0 and 1.0

  // interferer freq hop parameters
  int tx_freq_behavior;     // NONE | ALTERNATING | SWEEP | RANDOM
  double tx_freq_min;        // center frequency minimum
  double tx_freq_max;        // center frequency maximum
  double tx_freq_dwell_time; // seconds at a given freq
  double tx_freq_resolution; // granularity for SWEEP and RANDOM frequency behaviors

};

void print_node_parameters(struct node_parameters *np);

//////////////////////////////////////////////////////////////////
// Control and feedback
//////////////////////////////////////////////////////////////////

#define CRTS_MAX_NODES 48
#define CRTS_TCP_CONTROL_PORT 4444
#define CRTS_CR_PORT 4444
#define CRTS_CR_PACKET_LEN 256        // length of network packet data
#define CRTS_CR_PACKET_NUM_LEN 4      // number of bytes used for packet numbering
#define CRTS_CR_PACKET_SR_LEN 12      // shift register length for pseudo-random packet generation
// the controller will forcefully terminate all node processes after this many seconds have passed once the scenario has ended
#define CRTS_FORCEFUL_TERMINATION_DELAY_S 5 

enum crts_msg_type {
  CRTS_MSG_SCENARIO_PARAMETERS = 0,
  CRTS_MSG_START,
  CRTS_MSG_TERMINATE,
  CRTS_MSG_CONTROL,
  CRTS_MSG_FEEDBACK,
  CRTS_MSG_SUMMARY
};

// enumeration of all types of control and feedback passed between 
// the controller and all other nodes during an experiment
#define CRTS_NUM_PARAM_TYPES 27
enum crts_params {
  CRTS_TX_STATE = 0,
  CRTS_TX_FREQ,
  CRTS_TX_RATE,
  CRTS_TX_GAIN,
  CRTS_TX_MOD,
  CRTS_TX_CRC,
  CRTS_TX_FEC0,
  CRTS_TX_FEC1,

  CRTS_RX_STATE,
  CRTS_RX_RESET,
  CRTS_RX_FREQ,
  CRTS_RX_RATE,
  CRTS_RX_GAIN,
  CRTS_RX_STATS,
  CRTS_RX_STATS_FB,
  CRTS_RX_STATS_RESET,
  
  CRTS_NET_THROUGHPUT,
  CRTS_NET_TRAFFIC_TYPE,

  CRTS_FB_EN,
  
  // interferer specific parameters
  CRTS_TX_DUTY_CYCLE,
  CRTS_TX_PERIOD,
  CRTS_TX_FREQ_BEHAVIOR,
  CRTS_TX_FREQ_MIN,
  CRTS_TX_FREQ_MAX,
  CRTS_TX_FREQ_DWELL_TIME,
  CRTS_TX_FREQ_RES,
  
  CRTS_UNKNOWN_PARAM
};

// defines bit masks used for feedback enables
#define CRTS_TX_STATE_FB_EN       (1<<CRTS_TX_STATE)
#define CRTS_TX_FREQ_FB_EN        (1<<CRTS_TX_FREQ)
#define CRTS_TX_RATE_FB_EN        (1<<CRTS_TX_RATE)
#define CRTS_TX_GAIN_FB_EN        (1<<CRTS_TX_GAIN)
#define CRTS_TX_MOD_FB_EN         (1<<CRTS_TX_MOD)
#define CRTS_TX_CRC_FB_EN         (1<<CRTS_TX_CRC)  
#define CRTS_TX_FEC0_FB_EN        (1<<CRTS_TX_FEC0)
#define CRTS_TX_FEC1_FB_EN        (1<<CRTS_TX_FEC1)

#define CRTS_RX_STATE_FB_EN       (1<<CRTS_RX_STATE)
#define CRTS_RX_FREQ_FB_EN        (1<<CRTS_RX_FREQ)
#define CRTS_RX_RATE_FB_EN        (1<<CRTS_RX_RATE)
#define CRTS_RX_GAIN_FB_EN        (1<<CRTS_RX_GAIN)
#define CRTS_RX_STATS_FB_EN       (1<<CRTS_RX_STATS)

// all types of parameters used by CRTS communication
enum crts_param_types{
  CRTS_PARAM_INT = 0,
  CRTS_PARAM_DOUBLE,
  CRTS_PARAM_RX_STATISTICS,
  CRTS_PARAM_NONE
};

extern const char * crts_param_str[CRTS_NUM_PARAM_TYPES];

void set_node_parameter(int node, char cont_type, void* _arg);

int get_control_arg_len(int control_type);
int get_feedback_arg_len(int fb_type);
int crts_get_param_type(int param);
int crts_get_str2param(const char *);
int crts_get_str2net_traffic_type(const char *);
int crts_get_str2tx_freq_behavior(const char *);

#endif
