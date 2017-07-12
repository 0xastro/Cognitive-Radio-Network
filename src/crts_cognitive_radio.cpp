#include <stdlib.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <string>
#include <uhd/utils/msg.hpp>
#include <uhd/types/time_spec.hpp>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <signal.h>
#include <random>
#include "timer.h"
#include "crts.hpp"
#include "extensible_cognitive_radio.hpp"
#include "tun.hpp"

#define DEBUG 0
#if DEBUG == 1
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) /*__VA_ARGS__*/
#endif

int sig_terminate;
bool start_msg_received = false;
time_t start_time_s = 0;
time_t stop_time_s = 0;
std::ofstream log_rx_fstream;
std::ofstream log_tx_fstream;
float rx_stats_fb_period = 1e4;
timer rx_stat_fb_timer;
long int bytes_sent;
long int bytes_received;

void apply_control_msg(char cont_type, 
                       void* _arg, 
                       struct node_parameters *np,
                       ExtensibleCognitiveRadio * ECR,
                       int *fb_enables,
                       float *t_step); 

void receive_command_from_controller(int *tcp_controller,
                                     struct scenario_parameters *sp,
                                     struct node_parameters *np,
                                     ExtensibleCognitiveRadio *ECR,
                                     int *fb_enables,
                                     float *t_step) {
  // Listen to socket for message from controller
  char command_buffer[1 + sizeof(struct scenario_parameters) +
                      sizeof(struct node_parameters)];
  memset(&command_buffer, 0, sizeof(command_buffer));

  // setup file descriptor to listen for data on TCP controller link.
  fd_set fds;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 100;
  FD_ZERO(&fds);
  FD_SET(*tcp_controller, &fds);

  // if data is available or the scenario has not been started, read in message from controller
  if (select(*tcp_controller + 1, &fds, NULL, NULL, &timeout) || (!start_msg_received)) {
    // read the first byte which designates the message type
    int rflag = recv(*tcp_controller, command_buffer, 1, 0);

    int err = errno;
    if (rflag <= 0) {
      if ((err == EAGAIN) || (err == EWOULDBLOCK))
        return;
      else {
        close(*tcp_controller);
        printf("Socket failure\n");
        sig_terminate = 1;
      }
    }

    dprintf("Received command type %i from controller\n", command_buffer[0]);
    // Parse command based on the message type
    switch (command_buffer[0]) {
      case CRTS_MSG_SCENARIO_PARAMETERS: // settings for upcoming scenario
        dprintf("Received settings for scenario\n");
        // receive and copy scenario parameters
        rflag = recv(*tcp_controller, &command_buffer[1],
                     sizeof(struct scenario_parameters), 0);
        memcpy(sp, &command_buffer[1], sizeof(struct scenario_parameters));

        // receive and copy node_parameters
        rflag = recv(*tcp_controller,
                     &command_buffer[1 + sizeof(struct scenario_parameters)],
                     sizeof(struct node_parameters), 0);
        memcpy(np, &command_buffer[1 + sizeof(struct scenario_parameters)],
               sizeof(struct node_parameters));
        print_node_parameters(np);
        break;
      case CRTS_MSG_START: // updated start time (used for manual mode)
        dprintf("Received manual start from controller");
        rflag = recv(*tcp_controller, &command_buffer[1], sizeof(int64_t), 0);
        start_msg_received = true;
        memcpy(&start_time_s, &command_buffer[1], sizeof(int64_t));
        stop_time_s = (time_t) (start_time_s + sp->run_time);
        break;
      case CRTS_MSG_TERMINATE: // terminate program
        dprintf("Received termination command from controller\n");
        sig_terminate = 1;
        break;
      case CRTS_MSG_CONTROL:
        rflag = recv(*tcp_controller, &command_buffer[1], 1, 0);
        int arg_len = get_control_arg_len(command_buffer[1]);
        if(arg_len > 0)
          rflag = recv(*tcp_controller, &command_buffer[2], arg_len, 0);
        dprintf("Received a %i byte control message\n", rflag);
        apply_control_msg(command_buffer[1], (void*) &command_buffer[2], np, ECR, fb_enables, t_step); 
        break;
    }
  }
}

void apply_control_msg(char cont_type, 
                       void* _arg, 
                       struct node_parameters *np,
                       ExtensibleCognitiveRadio * ECR,
                       int *fb_enables,
                       float *t_step){
  switch (cont_type){
    case CRTS_TX_STATE:
      if (*(int*)_arg == TX_STOPPED)
        ECR->stop_tx();
      if (*(int*)_arg == TX_CONTINUOUS)
        ECR->start_tx();
      break;
    case CRTS_TX_FREQ:
      ECR->set_tx_freq(*(double*)_arg);
      break;
    case CRTS_TX_RATE:
      ECR->set_tx_rate(*(double*)_arg);
      break;
    case CRTS_TX_GAIN:
      ECR->set_tx_gain_uhd(*(double*)_arg);
      break;
    case CRTS_TX_MOD:
      ECR->set_tx_modulation(*(int*)_arg);
      break;
    case CRTS_TX_CRC:
      ECR->set_tx_crc(*(int*)_arg);
      break;
    case CRTS_TX_FEC0:
      ECR->set_tx_fec0(*(int*)_arg);
      break;
    case CRTS_TX_FEC1:
      ECR->set_tx_fec1(*(int*)_arg);
      break;
    case CRTS_RX_STATE:
      if (*(int*)_arg == RX_STOPPED)
        ECR->stop_rx();
      if (*(int*)_arg == RX_CONTINUOUS)
        ECR->start_rx();
      break;
    case CRTS_RX_RESET:
      ECR->reset_rx();
      break;
    case CRTS_RX_FREQ:
      ECR->set_rx_freq(*(double*)_arg);
      break;
    case CRTS_RX_RATE:
      ECR->set_rx_rate(*(double*)_arg);
      break;
    case CRTS_RX_GAIN:
      ECR->set_rx_gain_uhd(*(double*)_arg);
      break;
    case CRTS_RX_STATS:
      if (*(double*)_arg > 0.0){
        ECR->set_rx_stat_tracking(true, *(double*)_arg);
      } else {
        ECR->set_rx_stat_tracking(false, 0.0);
      }
      break;
    case CRTS_RX_STATS_RESET:
      ECR->reset_rx_stats();
      timer_tic(rx_stat_fb_timer);
      break;
    case CRTS_RX_STATS_FB:
      rx_stats_fb_period = *(double*)_arg;
      break;
    case CRTS_NET_THROUGHPUT:
      np->net_mean_throughput = *(double*)_arg;
      *t_step = 8.0 * (double)CRTS_CR_PACKET_LEN / np->net_mean_throughput;
      break;
    case CRTS_NET_TRAFFIC_TYPE:
      np->net_traffic_type = *(int*)_arg;
      break;
    case CRTS_FB_EN:
      *fb_enables = *(int*)_arg;
      break;
    default:
      printf("Unknown control type\n");
  }
}

void send_feedback_to_controller(int *tcp_controller,
                                 unsigned int fb_enables,
                                 ExtensibleCognitiveRadio *ECR) {
  // variables used to keep track of the state and send feedback to the controller when needed
  static int last_tx_state = TX_STOPPED;
  static double last_tx_freq = ECR->get_tx_freq();
  static double last_tx_rate = ECR->get_tx_rate();
  static double last_tx_gain = ECR->get_tx_gain_uhd();
  static int last_tx_mod = ECR->get_tx_modulation();
  static int last_tx_crc = ECR->get_tx_crc();
  static int last_tx_fec0 = ECR->get_tx_fec0();
  static int last_tx_fec1 = ECR->get_tx_fec1();

  static int last_rx_state = RX_STOPPED;
  static double last_rx_freq = ECR->get_rx_freq();
  static double last_rx_rate = ECR->get_rx_rate();
  static double last_rx_gain = ECR->get_rx_gain_uhd();
  
  static bool timer_init = 0;

  if(!timer_init){
    timer_init = 1;
    timer_tic(rx_stat_fb_timer);
  }
  
  // variables used to define the feedback message to the controller
  char fb_msg[1024];
  fb_msg[0] = CRTS_MSG_FEEDBACK;
  int fb_args = 0;
  int fb_msg_ind = 2;

  // check for all possible update messages
  if (fb_enables & CRTS_TX_STATE_FB_EN){
    int tx_state = ECR->get_tx_state();
    if(tx_state != last_tx_state){
      fb_msg[fb_msg_ind] = CRTS_TX_STATE;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_state, sizeof(tx_state));
      fb_msg_ind += 1+sizeof(tx_state);
      last_tx_state = tx_state;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_TX_FREQ_FB_EN){
    double tx_freq = ECR->get_tx_freq();
    if(tx_freq != last_tx_freq){
      fb_msg[fb_msg_ind] = CRTS_TX_FREQ;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_freq, sizeof(tx_freq));
      fb_msg_ind += 1+sizeof(tx_freq);
      last_tx_freq = tx_freq;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_TX_RATE_FB_EN){
    double tx_rate = ECR->get_tx_rate();
    if(tx_rate != last_tx_rate){
      fb_msg[fb_msg_ind] = CRTS_TX_RATE;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_rate, sizeof(tx_rate));
      fb_msg_ind += 1+sizeof(tx_rate);
      last_tx_rate = tx_rate;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_TX_GAIN_FB_EN){
    double tx_gain = ECR->get_tx_gain_uhd();
    if(tx_gain != last_tx_gain){
      fb_msg[fb_msg_ind] = CRTS_TX_GAIN;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_gain, sizeof(tx_gain));
      fb_msg_ind += 1+sizeof(tx_gain);
      last_tx_gain = tx_gain;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_TX_MOD_FB_EN){
    int tx_mod = ECR->get_tx_modulation();
    if(tx_mod != last_tx_mod){
      fb_msg[fb_msg_ind] = CRTS_TX_MOD;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_mod, sizeof(tx_mod));
      fb_msg_ind += 1+sizeof(tx_mod);
      last_tx_mod = tx_mod;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_TX_CRC_FB_EN){
      int tx_crc = ECR->get_tx_crc();
      if(tx_crc != last_tx_crc){
          fb_msg[fb_msg_ind] = CRTS_TX_CRC;
          memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_crc, sizeof(tx_crc));
          fb_msg_ind += 1+sizeof(tx_crc);
          last_tx_crc = tx_crc;
          fb_args++;
      }
  }
  if (fb_enables & CRTS_TX_FEC0_FB_EN){
    int tx_fec0 = ECR->get_tx_fec0();
    if(tx_fec0 != last_tx_fec0){
      fb_msg[fb_msg_ind] = CRTS_TX_FEC0;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_fec0, sizeof(tx_fec0));
      fb_msg_ind += 1+sizeof(tx_fec0);
      last_tx_fec0 = tx_fec0;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_TX_FEC1_FB_EN){
    int tx_fec1 = ECR->get_tx_fec1();
    if(tx_fec1 != last_tx_fec1){
      fb_msg[fb_msg_ind] = CRTS_TX_FEC1;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_fec1, sizeof(tx_fec1));
      fb_msg_ind += 1+sizeof(tx_fec1);
      last_tx_fec1 = tx_fec1;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_RX_STATE_FB_EN){
    int rx_state = ECR->get_rx_state();
    if(rx_state != last_rx_state){
      fb_msg[fb_msg_ind] = CRTS_RX_STATE;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&rx_state, sizeof(rx_state));
      fb_msg_ind += 1+sizeof(rx_state);
      last_rx_state = rx_state;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_RX_FREQ_FB_EN){
    int rx_freq = ECR->get_rx_freq();
    if(rx_freq != last_rx_freq){
      fb_msg[fb_msg_ind] = CRTS_RX_FREQ;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&rx_freq, sizeof(rx_freq));
      fb_msg_ind += 1+sizeof(rx_freq);
      last_rx_freq = rx_freq;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_RX_RATE_FB_EN){
    double rx_rate = ECR->get_rx_rate();
    if(rx_rate != last_rx_rate){
      fb_msg[fb_msg_ind] = CRTS_RX_RATE;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&rx_rate, sizeof(rx_rate));
      fb_msg_ind += 1+sizeof(rx_rate);
      last_rx_rate = rx_rate;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_RX_GAIN_FB_EN){
    double rx_gain = ECR->get_rx_gain_uhd();
    if(rx_gain != last_rx_gain){
      fb_msg[fb_msg_ind] = CRTS_RX_GAIN;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&rx_gain, sizeof(rx_gain));
      fb_msg_ind += 1+sizeof(rx_gain);
      last_rx_gain = rx_gain;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_RX_STATS_FB_EN){
    if(timer_toc(rx_stat_fb_timer) > rx_stats_fb_period){
      timer_tic(rx_stat_fb_timer);
      struct ExtensibleCognitiveRadio::rx_statistics rx_stats = ECR->get_rx_stats();
      /*printf("\nSending feedback:\n");
      printf("PER: %f\n", rx_stats.avg_per);
      printf("BER: %f\n", rx_stats.avg_ber);
      printf("RSSI: %f\n", rx_stats.avg_rssi);
      printf("EVM: %f\n", rx_stats.avg_evm);
      */
      fb_msg[fb_msg_ind] = CRTS_RX_STATS;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&rx_stats, sizeof(rx_stats));
      fb_msg_ind += 1+sizeof(rx_stats);
      fb_args++;
    }
  }

  fb_msg[1] = fb_args;
  
  // send feedback to controller
  if(fb_args > 0){
    send(*tcp_controller, fb_msg, fb_msg_ind, 0);
  }      
}

void Initialize_CR(struct node_parameters *np, void *ECR_p,
                   int argc, char **argv) {

  // initialize ECR parameters if applicable
  if (np->cognitive_radio_type == EXTENSIBLE_COGNITIVE_RADIO) {
    ExtensibleCognitiveRadio *ECR = (ExtensibleCognitiveRadio *)ECR_p;

    // append relative locations for log files
    char phy_rx_log_file_name[255];
    strcpy(phy_rx_log_file_name, "./logs/bin/");
    strcat(phy_rx_log_file_name, np->phy_rx_log_file);
    strcat(phy_rx_log_file_name, ".log");

    char phy_tx_log_file_name[255];
    strcpy(phy_tx_log_file_name, "./logs/bin/");
    strcat(phy_tx_log_file_name, np->phy_tx_log_file);
    strcat(phy_tx_log_file_name, ".log");

    // set cognitive radio parameters
    ECR->set_ip(np->crts_ip);
    ECR->print_metrics_flag = np->print_rx_frame_metrics;
    ECR->log_phy_rx_flag = np->log_phy_rx;
    ECR->log_phy_tx_flag = np->log_phy_tx;
    ECR->set_ce_timeout_ms(np->ce_timeout_ms);
    strcpy(ECR->phy_rx_log_file, phy_rx_log_file_name);
    strcpy(ECR->phy_tx_log_file, phy_tx_log_file_name);
    ECR->set_rx_freq(np->rx_freq);
    ECR->set_rx_rate(np->rx_rate);
    ECR->set_rx_gain_uhd(np->rx_gain);
    ECR->set_rx_subcarriers(np->rx_subcarriers);
    ECR->set_rx_cp_len(np->rx_cp_len);
    ECR->set_rx_taper_len(np->rx_taper_len);
    ECR->set_tx_freq(np->tx_freq);
    ECR->set_tx_rate(np->tx_rate);
    ECR->set_tx_gain_soft(np->tx_gain_soft);
    ECR->set_tx_gain_uhd(np->tx_gain);
    ECR->set_tx_subcarriers(np->tx_subcarriers);
    ECR->set_tx_cp_len(np->tx_cp_len);
    ECR->set_tx_taper_len(np->tx_taper_len);
    ECR->set_tx_modulation(np->tx_modulation);
    ECR->set_tx_crc(np->tx_crc);
    ECR->set_tx_fec0(np->tx_fec0);
    ECR->set_tx_fec1(np->tx_fec1);
    ECR->set_rx_stat_tracking(false, 0.0);
    ECR->set_ce(np->cognitive_engine, argc, argv);
    ECR->reset_log_files();

    // copy subcarrier allocations if other than liquid-dsp default
    if (np->tx_subcarrier_alloc_method == CUSTOM_SUBCARRIER_ALLOC ||
        np->tx_subcarrier_alloc_method == STANDARD_SUBCARRIER_ALLOC) {
      ECR->set_tx_subcarrier_alloc(np->tx_subcarrier_alloc);
    } else {
      ECR->set_tx_subcarrier_alloc(NULL);
    }
    if (np->rx_subcarrier_alloc_method == CUSTOM_SUBCARRIER_ALLOC ||
        np->rx_subcarrier_alloc_method == STANDARD_SUBCARRIER_ALLOC) {
      ECR->set_rx_subcarrier_alloc(np->rx_subcarrier_alloc);
    } else {
      ECR->set_rx_subcarrier_alloc(NULL);
    }
  }
  // intialize python radio if applicable
  else if(np->cognitive_radio_type == PYTHON)
  {
      // set IP for TUN interface
      char command[100];
      sprintf(command, "ifconfig tunCRTS %s", np->crts_ip);
      system("ip link set dev tunCRTS up");
      sprintf(command, "ip addr add %s/24 dev tunCRTS", np->crts_ip);
      system(command);
      printf("Running command: %s\n", command);
      system("route add -net 10.0.0.0 netmask 255.255.255.0 dev tunCRTS");
      system(command);
      system("ifconfig");
  }
}



void log_rx_data(struct scenario_parameters *sp, struct node_parameters *np,
                 int bytes, int packet_num) {
  // update current time
  struct timeval tv;
  gettimeofday(&tv, NULL);

  // open file, append parameters, and close
  if (log_rx_fstream.is_open()) {
    log_rx_fstream.write((char *)&tv, sizeof(tv));
    log_rx_fstream.write((char *)&bytes, sizeof(bytes));
    log_rx_fstream.write((char *)&packet_num, sizeof(packet_num));
  } else
    printf("Error opening log file: %s\n", np->net_rx_log_file);
}

void log_tx_data(struct scenario_parameters *sp, struct node_parameters *np,
                 int bytes, int packet_num) {
  // update current time
  struct timeval tv;
  gettimeofday(&tv, NULL);

  // open file, append parameters, and close
  if (log_tx_fstream.is_open()) {
    log_tx_fstream.write((char *)&tv, sizeof(tv));
    log_tx_fstream.write((char *)&bytes, sizeof(bytes));
    log_tx_fstream.write((char *)&packet_num, sizeof(packet_num));
  } else
    printf("Error opening log file: %s\n", np->net_tx_log_file);
}

void help_CRTS_CR() {
  printf("CRTS_CR -- Start a cognitive radio node. Only needs to be run "
         "explicitly when using CRTS_controller with -m option.\n");
  printf("        -- This program must be run from the main CRTS directory.\n");
  printf(" -h : Help.\n");
  printf(" -a : IP Address of node running CRTS_controller.\n");
}

void terminate(int signum) {
  printf("\nSending termination message to controller\n");
  sig_terminate = 1;
}

int main(int argc, char **argv) {

  // register signal handlers
  signal(SIGINT, terminate);
  signal(SIGQUIT, terminate);
  signal(SIGTERM, terminate);

  // Default IP address of controller
  char *controller_ipaddr = (char *)"192.168.1.56";

  int d;
  while ((d = getopt(argc, argv, "ha:")) != EOF) {
    switch (d) {
    case 'h':
      help_CRTS_CR();
      return 0;
    case 'a':
      controller_ipaddr = optarg;
      break;
    }
  }
  
  // Must reset getopt in case it is used later by the CE constructor
  optind = 0;

  // Create TCP client to controller
  int tcp_controller = socket(AF_INET, SOCK_STREAM, 0);
  if (tcp_controller < 0) {
    printf("ERROR: Receiver Failed to Create Client Socket\n");
    exit(EXIT_FAILURE);
  }
  // Parameters for connecting to server
  struct sockaddr_in controller_addr;
  memset(&controller_addr, 0, sizeof(controller_addr));
  controller_addr.sin_family = AF_INET;
  controller_addr.sin_addr.s_addr = inet_addr(controller_ipaddr);
  controller_addr.sin_port = htons(CRTS_TCP_CONTROL_PORT);

  // Attempt to connect client socket to server
  int connect_status =
      connect(tcp_controller, (struct sockaddr *)&controller_addr,
              sizeof(controller_addr));
  if (connect_status) {
    printf("Failed to Connect to server.\n");
    exit(EXIT_FAILURE);
  }
  dprintf("Connected to server\n");

  // Port to be used by CRTS server and client
  int port = CRTS_CR_PORT;

  // pointer to ECR which may or may not be used
  ExtensibleCognitiveRadio *ECR = NULL;

  // Create node parameters struct and the scenario parameters struct
  // and read info from controller
  struct node_parameters np;
  memset(&np, 0, sizeof(np));
  struct scenario_parameters sp;
  dprintf("Receiving command from controller...\n");
  sleep(1);
  float t_step;
  int fb_enables = 0;;
  receive_command_from_controller(&tcp_controller, &sp, &np, ECR, &fb_enables, &t_step);
  
  // copy log file name for post processing later
  char net_rx_log_file_cpy[100];
  strcpy(net_rx_log_file_cpy, np.net_rx_log_file);
  char net_tx_log_file_cpy[100];
  strcpy(net_tx_log_file_cpy, np.net_tx_log_file);

  // If log file names use subdirectories, create them if they don't exist
  char *subdirptr_rx = strrchr(net_rx_log_file_cpy, '/');
  char *subdirptr_tx = strrchr(net_tx_log_file_cpy, '/');
  if (subdirptr_rx) {
    char subdirs_rx[60];
    // Get the names of the subdirectories
    strncpy(subdirs_rx, net_rx_log_file_cpy,
            subdirptr_rx - net_rx_log_file_cpy);
    subdirs_rx[subdirptr_rx - net_rx_log_file_cpy] = '\0';
    char mkdir_cmd[100];
    strcpy(mkdir_cmd, "mkdir -p ./logs/bin/");
    strcat(mkdir_cmd, subdirs_rx);
    // Create them
    system(mkdir_cmd);
  }
  if (subdirptr_tx) {
    char subdirs_tx[60];
    // Get the names of the subdirectories
    strncpy(subdirs_tx, net_tx_log_file_cpy,
            subdirptr_tx - net_tx_log_file_cpy);
    subdirs_tx[subdirptr_tx - net_tx_log_file_cpy] = '\0';
    char mkdir_cmd[100];
    strcpy(mkdir_cmd, "mkdir -p ./logs/bin/");
    strcat(mkdir_cmd, subdirs_tx);
    // Create them
    system(mkdir_cmd);
  }

  // modify log file name in node parameters for logging function
  char net_rx_log_file[100];
  strcpy(net_rx_log_file, "./logs/bin/");
  strcat(net_rx_log_file, np.net_rx_log_file);
  strcat(net_rx_log_file, ".log");
  strcpy(np.net_rx_log_file, net_rx_log_file);

  char net_tx_log_file[100];
  strcpy(net_tx_log_file, "./logs/bin/");
  strcat(net_tx_log_file, np.net_tx_log_file);
  strcat(net_tx_log_file, ".log");
  strcpy(np.net_tx_log_file, net_tx_log_file);

  // open CRTS log files to delete any current contents
  if (np.log_net_rx) {
    log_rx_fstream.open(net_rx_log_file,
                        std::ofstream::out | std::ofstream::trunc);
    if (!log_rx_fstream.is_open()) {
      std::cout << "Error opening log file:" << net_rx_log_file << std::endl;
    }
  }
  if (np.log_net_tx) {
    log_tx_fstream.open(net_tx_log_file,
                        std::ofstream::out | std::ofstream::trunc);
    if (!log_tx_fstream.is_open()) {
      std::cout << "Error opening log file:" << net_rx_log_file << std::endl;
    }
  }

    // this is used to create a child process for python radios which can be killed later
    pid_t python_pid;
    
    // Create and start the ECR or python CR so that they are in a ready
    // state when the experiment begins
    if(np.cognitive_radio_type == EXTENSIBLE_COGNITIVE_RADIO)
    {
        dprintf("Creating ECR object...\n");
        ECR = new ExtensibleCognitiveRadio;

        // set the USRP's timer to 0
        uhd::time_spec_t t0(0, 0, 1e6);
        ECR->usrp_rx->set_time_now(t0, 0);


        rx_stat_fb_timer = timer_create();

        int argc = 0;
        char ** argv = NULL;
        dprintf("Converting ce_args to argc argv format\n");
        str2argcargv(np.ce_args, np.cognitive_engine, argc, argv);
        dprintf("Initializing CR\n");
        Initialize_CR(&np, (void *)ECR, argc, argv);
        freeargcargv(argc, argv);
    } 
    else if(np.cognitive_radio_type == PYTHON)
    {
        //Create tun interface
        system("sudo ip tuntap add dev tunCRTS mode tun");
        system("sudo ip link set dev tunCRTS up");
        dprintf("CRTS: Forking child process\n");
        //Calling fork creates a duplicate of the calling process that proceeeds from this point
        //In the new process, python_pid will be 0, which is how we know it is the child
        //In the parent however, python_pid will be the process id of the new process created by fork.
        //Then when we replace the child process below with execvp, the new process inherits the old pid,
        //allowing us to kill it later. See the very end of main()
        python_pid = fork();
        
        // define child's process
        if(python_pid == 0){
            //The execvp fuction used below takes two arguments
            //1. The command to run, python in this case
            //2. An array of null-terminated arguemnts
            //      The first item should be the program to run (python, I'm not exactly sure why this needs to be
            //          specified when it's the first argument to execvp)
            //      The second item is the file to run, this case the cognitive radio file.
            //      The next items in the array are the actual arguments from the node_parameters
            //      The final item must be a NULL pointer
            char* c_arguments[4];
            //Allocate space for and place "python" in first slot
            c_arguments[0] = new char[strlen("python")];
            strcpy(c_arguments[0], "python");
            //Allocate space for and place entire path of python radio file to run
            //Python radios are stored in the cognitive_radios folder inside the main crts folder
            char path_to_radio[1000];
            if(getcwd(path_to_radio, sizeof(path_to_radio)) != NULL)
            {
                strcat(path_to_radio, "/cognitive_radios/");
                strcat(path_to_radio, np.python_file);
            }
            else
            {
                perror("cwd");
                exit(1);
            }
            c_arguments[1] = new char[strlen(path_to_radio)];
            strcpy(c_arguments[1], path_to_radio);

            //Allocate space for and place arguments from np.arguments into array
            c_arguments[2] = new char[strlen(np.python_args)];
            strcpy(c_arguments[2], np.python_args);
            //End array with a NULL pointer
            c_arguments[3] = (char*)0;
            //Call execvp to start python radio. execvp replaces the current process (in this case the child of CRTS_CR
            //forked above), so the original CRTS_CR keeps running.
            execvp("python", c_arguments);
        
        }
        else
        {
            sleep(5);
            printf("CRTS Child: Initializing python CR\n");
            Initialize_CR(&np, NULL, 0, NULL);
        }
        
    } 
  
  // Define address structure for CRTS socket server used to receive network
  // traffic
  struct sockaddr_in crts_server_addr;
  memset(&crts_server_addr, 0, sizeof(crts_server_addr));
  crts_server_addr.sin_family = AF_INET;
  // Only receive packets addressed to the crts_ip
  crts_server_addr.sin_addr.s_addr = inet_addr(np.crts_ip);
  crts_server_addr.sin_port = htons(port);
  socklen_t clientlen = sizeof(crts_server_addr);
  int crts_server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // Define address structure for CRTS socket client used to send network
  // traffic
  struct sockaddr_in crts_client_addr;
  memset(&crts_client_addr, 0, sizeof(crts_client_addr));
  crts_client_addr.sin_family = AF_INET;
  crts_client_addr.sin_addr.s_addr = inet_addr(np.target_ip);
  crts_client_addr.sin_port = htons(port);
  int crts_client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // Bind CRTS server socket
  bind(crts_server_sock, (sockaddr *)&crts_server_addr, clientlen);

  // Define a buffer for receiving and a temporary message for sending
  int recv_buffer_len = 8192 * 2;
  char recv_buffer[recv_buffer_len];

  // Define parameters and message for sending
  int packet_counter = 0;
  unsigned char packet_num_prs[CRTS_CR_PACKET_NUM_LEN]; // pseudo-random sequence used
                                                  // to modify packet number
  unsigned char message[CRTS_CR_PACKET_LEN];
  srand(12);
  msequence ms = msequence_create_default(CRTS_CR_PACKET_SR_LEN);
  
  // define bit mask applied to packet number
  for (int i = 0; i < CRTS_CR_PACKET_NUM_LEN; i++)
    packet_num_prs[i] = msequence_generate_symbol(ms,8); //rand() & 0xff;

  // create a defined payload generated pseudo-randomly
  for (int i = CRTS_CR_PACKET_NUM_LEN; i < CRTS_CR_PACKET_LEN; i++){
    message[i] = msequence_generate_symbol(ms,8);//(rand() & 0xff);
  }
  
  // initialize sig_terminate flag and check return from socket call
  sig_terminate = 0;
  if (crts_client_sock < 0) {
    printf("CRTS failed to create client socket\n");
    sig_terminate = 1;
  }
  if (crts_server_sock < 0) {
    printf("CRTS failed to create server socket\n");
    sig_terminate = 1;
  }

  t_step = 8.0 * (float)CRTS_CR_PACKET_LEN / np.net_mean_throughput;
  float send_time_delta = 0;
  struct timeval send_time;
  fd_set read_fds;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 1;

  // Poisson RV generator
  std::default_random_engine rand_generator;
  std::poisson_distribution<int> poisson_generator(1e6);
  int poisson_rv;

  bytes_sent = 0;
  bytes_received = 0;

  // Wait for the start-time before beginning the scenario
  struct timeval tv;
  time_t time_s;
  while (1) {
    receive_command_from_controller(&tcp_controller, &sp, &np, ECR, &fb_enables, &t_step);
    gettimeofday(&tv, NULL);
    time_s = tv.tv_sec;
    if ((time_s >= (time_t) start_time_s) && start_msg_received)
      break;
    if (sig_terminate)
      break;
    usleep(5e2);
  }

  if (np.cognitive_radio_type == EXTENSIBLE_COGNITIVE_RADIO) {
    // Start ECR
    dprintf("Starting ECR object...\n");
    ECR->start_rx();
    ECR->start_tx();
    ECR->start_ce();
  }

  bool send_flag = true;
  
  // main loop: receives control, sends feedback, and generates/receives network traffic
  while ((time_s < stop_time_s) && (!sig_terminate)) {
    // Listen for any updates from the controller
    receive_command_from_controller(&tcp_controller, &sp, &np, ECR, &fb_enables, &t_step);

    // send feedback to the controller if applicable
    if (fb_enables > 0)
      send_feedback_to_controller(&tcp_controller, fb_enables, ECR);

    // Update send time only after immediately sending (send_flag == true)
    if (send_flag == true) {
      send_flag = false;
      // Send packets according to traffic model
      switch (np.net_traffic_type) {
      case (NET_TRAFFIC_STREAM):
        send_time_delta += t_step * 1e6;
        break;
      case (NET_TRAFFIC_BURST):
        send_time_delta += t_step * np.net_burst_length * 1e6;
        break;
      case (NET_TRAFFIC_POISSON):
        poisson_rv = poisson_generator(rand_generator);
        send_time_delta += t_step * (float)poisson_rv;
        break;
      }
      send_time.tv_sec = start_time_s + (long int)floorf(send_time_delta / 1e6);
      send_time.tv_usec = (long int)fmod(send_time_delta, 1e6);
    }

    // determine if it's time to send another packet burst
    gettimeofday(&tv, NULL);
    if ((tv.tv_sec == send_time.tv_sec && tv.tv_usec > send_time.tv_usec) ||
        tv.tv_sec > send_time.tv_sec)
      send_flag = true;

    if (send_flag) {
      // send burst of packets
      for (int i = 0; i < np.net_burst_length; i++) {

        // update packet number
        packet_counter++;
        for (int i = 0; i < CRTS_CR_PACKET_NUM_LEN; i++)
          message[i] =
              ((packet_counter >> (8 * (CRTS_CR_PACKET_NUM_LEN - i - 1))) & 0xff) ^
              packet_num_prs[i];

        // send UDP packet via CR
        dprintf("CRTS sending packet %i\n", packet_counter);
        int send_return = 0;
        sendto(crts_client_sock, (char *)message, sizeof(message), 0,
                (struct sockaddr *)&crts_client_addr, sizeof(crts_client_addr));
        if (send_return < 0)
          printf("Failed to send message\n");
        else
          bytes_sent += send_return;

        ECR->inc_tx_queued_bytes(send_return+32);
        
        if (np.log_net_tx) {
          log_tx_data(&sp, &np, send_return, packet_counter);
        }
      }
    }

    // read all available data from the UDP socket
    int recv_len = 0;
    FD_ZERO(&read_fds);
    FD_SET(crts_server_sock, &read_fds);
    while (select(crts_server_sock + 1, &read_fds, NULL, NULL, &timeout) > 0) {
      recv_len = recvfrom(crts_server_sock, recv_buffer, recv_buffer_len, 0,
                          (struct sockaddr *)&crts_server_addr, &clientlen);

      // determine packet number
      int rx_packet_num = 0;
      for (int i = 0; i < CRTS_CR_PACKET_NUM_LEN; i++)
        rx_packet_num +=
            (((unsigned char)recv_buffer[i]) ^ packet_num_prs[i])
            << 8 * (CRTS_CR_PACKET_NUM_LEN - i - 1);

      // print out/log details of received messages
      if (recv_len > 0) {
        // TODO: Say what address message was received from.
        // (It's in CRTS_server_addr)
        dprintf("CRTS received packet %i containing %i bytes:\n", rx_packet_num,
                recv_len);
        bytes_received += recv_len;
        if (np.log_net_rx) {
          log_rx_data(&sp, &np, recv_len, rx_packet_num);
        }
      }

      FD_ZERO(&read_fds);
      FD_SET(crts_server_sock, &read_fds);
    }

    // Update the current time
    gettimeofday(&tv, NULL);
    time_s = tv.tv_sec;
  }

  // close the log files
  if (np.log_net_rx)
    log_rx_fstream.close();
  if (np.log_net_tx)
    log_tx_fstream.close();

  // close all network connections
  close(crts_client_sock);
  close(crts_server_sock);

  // auto-generate octave logs from binary logs
  char command[1000];
  if (np.generate_octave_logs) {
    if (np.log_net_rx) {
      sprintf(command, "./logs/convert_logs_bin_to_octave -c -l %s", net_rx_log_file_cpy);
      system(command);
    }

    if (np.log_net_tx) {
      sprintf(command, "./logs/convert_logs_bin_to_octave -C -l %s", net_tx_log_file_cpy);
      system(command);
    }

    if (np.log_phy_rx) {
      sprintf(command, "./logs/convert_logs_bin_to_octave -r -l %s", np.phy_rx_log_file);
      system(command);
    }

    if (np.log_phy_tx) {
      sprintf(command, "./logs/convert_logs_bin_to_octave -t -l %s", np.phy_tx_log_file);
      system(command);
    }
  }
  
  // clean up ECR/python process
  if (np.cognitive_radio_type == EXTENSIBLE_COGNITIVE_RADIO) {
    delete ECR;
  } else if (np.cognitive_radio_type == PYTHON) {
    kill(python_pid, SIGTERM);
  }

  printf(
      "CRTS: Reached termination. Sending termination message to controller\n");
  char msg[1+2*sizeof(long int)];
  msg[0] = CRTS_MSG_SUMMARY;
  memcpy((void*)&msg[1], (void*)&bytes_sent, sizeof(bytes_sent));
  memcpy((void*)&msg[1+sizeof(long int)], (void*)&bytes_received, sizeof(bytes_sent));
  write(tcp_controller, &msg, 1+2*sizeof(long int));
  msg[0] = CRTS_MSG_TERMINATE;
  write(tcp_controller, &msg, 1);
  close(tcp_controller);
}
