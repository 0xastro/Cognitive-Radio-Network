#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <string>
#include <time.h>
#include <uhd/utils/msg.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <errno.h>
#include <signal.h>
#include <complex>
#include <liquid/liquid.h>
#include <fstream>
#include "crts.hpp"
#include "interferer.hpp"
#include "timer.h"

#define DEBUG 0
#if DEBUG == 1
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) /*__VA_ARGS__*/
#endif

// global variables
bool start_msg_received = false;
time_t start_time_s;
time_t stop_time_s;
struct timeval tv;
time_t time_s;
int sig_terminate;
int time_terminate;
int TCP_controller;

void apply_control_msg(char cont_type, 
                       void* _arg, 
                       struct node_parameters *np,
                       Interferer * Int,
                       int *fb_enables); 

// ========================================================================
//  FUNCTION:  receive_command_from_controller
// ========================================================================
static inline void
receive_command_from_controller(Interferer *Int, struct node_parameters *np,
                                struct scenario_parameters *sp, int *fb_enables) {
  fd_set fds;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(TCP_controller, &fds);

  // Listen to socket for message from controller
  if (select(TCP_controller + 1, &fds, NULL, NULL, &timeout) || (!start_msg_received)) {
    char command_buffer[1 + sizeof(struct scenario_parameters) +
                        sizeof(struct node_parameters)];
    int rflag = recv(TCP_controller, command_buffer, 1, 0);
    int err = errno;
    if (rflag <= 0) {
      if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
        return;
      } else {
        close(TCP_controller);
        printf("Socket failure\n");
        exit(EXIT_FAILURE);
      }
    }

    // Parse command
    dprintf("Received command from controller\n");
    switch (command_buffer[0]) {
    case CRTS_MSG_SCENARIO_PARAMETERS: // settings for upcoming scenario
    {
      dprintf("Received settings for scenario\n");

      // copy scenario parameters
      rflag = recv(TCP_controller, &command_buffer[1], sizeof(struct scenario_parameters), 0);
      memcpy(sp, &command_buffer[1], sizeof(struct scenario_parameters));

      // copy node_parameters
      rflag = recv(TCP_controller, &command_buffer[1+sizeof(struct scenario_parameters)], 
                   sizeof(struct node_parameters), 0);
      memcpy(np, &command_buffer[1 + sizeof(struct scenario_parameters)],
             sizeof(struct node_parameters));
      print_node_parameters(np);

      // set interferer parameters
      Int->tx_freq = np->tx_freq;
      Int->interference_type = np->interference_type;
      Int->period = np->period;
      Int->duty_cycle = np->duty_cycle;
      Int->tx_rate = np->tx_rate;
      Int->tx_gain = np->tx_gain;
      Int->tx_gain_soft = np->tx_gain_soft;

      // set USRP settings
      Int->usrp_tx->set_tx_freq(Int->tx_freq);
      Int->usrp_tx->set_tx_rate(Int->tx_rate);
      Int->usrp_tx->set_tx_gain(Int->tx_gain);

      // set freq parameters
      Int->tx_freq_behavior = np->tx_freq_behavior;
      Int->tx_freq_min = np->tx_freq_min;
      Int->tx_freq_max = np->tx_freq_max;
      Int->tx_freq_dwell_time = np->tx_freq_dwell_time;
      Int->tx_freq_resolution = np->tx_freq_resolution;
      Int->tx_freq_bandwidth = floor(np->tx_freq_max - np->tx_freq_min);

      // FIXME
      // If log file names use subdirectories, create them if they don't exist
      // char * subdirptr_tx = strrchr(phy_tx_log_file, '/');
      // if (subdirptr_tx) {
      //  char subdirs_tx[60];
      //  // Get the names of the subdirectories
      //  strncpy(subdirs_tx, phy_tx_log_file, subdirptr_tx - phy_tx_log_file);
      //  subdirs_tx[subdirptr_tx - phy_tx_log_file] = '\0';
      //  char mkdir_cmd[100];
      //  strcpy(mkdir_cmd, "mkdir -p ./logs/bin/");
      //  strcat(mkdir_cmd, subdirs_tx);
      //  // Create them
      //  system(mkdir_cmd);
      //}

      // create string of actual log file location
      char tx_log_full_path[100];
      strcpy(tx_log_full_path, "./logs/bin/");
      strcat(tx_log_full_path, np->phy_tx_log_file);
      strcat(tx_log_full_path, ".log");
      Int->set_log_file(tx_log_full_path);
      Int->log_tx_flag = np->log_phy_tx;

      break;
    }

    case CRTS_MSG_START: // updated start time (used for manual mode)
      dprintf("Received scenario start time\n");
      rflag = recv(TCP_controller, &command_buffer[1], sizeof(time_t), 0);
      start_msg_received = true;
      memcpy(&start_time_s, &command_buffer[1], sizeof(time_t));
      stop_time_s = start_time_s + sp->run_time;
      break;
    case CRTS_MSG_TERMINATE: // terminate program
      dprintf("Received termination command from controller\n");
      sig_terminate = 1;
      break;
    case CRTS_MSG_CONTROL:
      rflag = recv(TCP_controller, &command_buffer[1], 1, 0);
      int arg_len = get_control_arg_len(command_buffer[1]);
      rflag = recv(TCP_controller, &command_buffer[2], arg_len, 0);
      dprintf("Received a %i byte control message\n", rflag);
      apply_control_msg(command_buffer[1], (void*) &command_buffer[2], np, Int, fb_enables); 
      break;
    }
  }
}

void apply_control_msg(char cont_type, 
                       void* _arg, 
                       struct node_parameters *np,
                       Interferer * Int,
                       int *fb_enables){
  switch (cont_type){
    case CRTS_TX_STATE:
      if (*(int*)_arg == INT_TX_STOPPED)
        Int->stop_tx();
      else
        Int->start_tx();
      break;
    case CRTS_TX_FREQ:
      Int->tx_freq = *(double*)_arg;
      break;
    case CRTS_TX_RATE:
      Int->tx_rate = (*(double*)_arg);
    case CRTS_TX_GAIN:
      Int->tx_gain = (*(double*)_arg);
      break;
    case CRTS_FB_EN:
      *fb_enables = *(int*)_arg;
      break;
    case CRTS_TX_DUTY_CYCLE:
      Int->duty_cycle = *(double*)_arg; 
      break;
    case CRTS_TX_PERIOD:
      Int->period = *(double*)_arg;
      break;
    case CRTS_TX_FREQ_BEHAVIOR:
      Int->tx_freq_behavior = np->tx_freq_behavior;
      break;
    case CRTS_TX_FREQ_MIN:
      Int->tx_freq_min = np->tx_freq_min;
      Int->tx_freq_bandwidth = floor(np->tx_freq_max - np->tx_freq_min);
      break;
    case CRTS_TX_FREQ_MAX:
      Int->tx_freq_max = np->tx_freq_max;
      Int->tx_freq_bandwidth = floor(np->tx_freq_max - np->tx_freq_min);
      break;
    case CRTS_TX_FREQ_DWELL_TIME:
      Int->tx_freq_dwell_time = np->tx_freq_dwell_time;
      break;
    case CRTS_TX_FREQ_RES:
      Int->tx_freq_resolution = np->tx_freq_resolution;
      break;
      default:
      printf("Control type is unknown or not applicable to interferers\n");
  }
}

void send_feedback_to_controller(int *TCP_controller,
                                 unsigned int fb_enables,
                                 Interferer *Int) {
  // variables used to keep track of the state and send feedback to the controller when needed
  static int last_tx_state = INT_TX_STOPPED;
  static double last_tx_freq = Int->tx_freq;
  static double last_tx_rate = Int->tx_rate;
  static double last_tx_gain = Int->tx_gain;
  
  // variables used to define the feedback message to the controller
  char fb_msg[1024];
  fb_msg[0] = CRTS_MSG_FEEDBACK;
  int fb_args = 0;
  int fb_msg_ind = 2;

  // check for all possible update messages
  if (fb_enables & CRTS_TX_STATE_FB_EN){
    int tx_state = Int->tx_state;
    if(tx_state != last_tx_state){
      fb_msg[fb_msg_ind] = CRTS_TX_STATE;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_state, sizeof(tx_state));
      fb_msg_ind += 1+sizeof(tx_state);
      last_tx_state = tx_state;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_TX_FREQ_FB_EN){
    double tx_freq = Int->tx_freq;
    if(tx_freq != last_tx_freq){
      fb_msg[fb_msg_ind] = CRTS_TX_FREQ;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_freq, sizeof(tx_freq));
      fb_msg_ind += 1+sizeof(tx_freq);
      last_tx_freq = tx_freq;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_TX_RATE_FB_EN){
    double tx_rate = Int->tx_rate;
    if(tx_rate != last_tx_rate){
      fb_msg[fb_msg_ind] = CRTS_TX_RATE;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_rate, sizeof(tx_rate));
      fb_msg_ind += 1+sizeof(tx_rate);
      last_tx_rate = tx_rate;
      fb_args++;
    }
  }
  if (fb_enables & CRTS_TX_GAIN_FB_EN){
    double tx_gain = Int->tx_gain;
    if(tx_gain != last_tx_gain){
      fb_msg[fb_msg_ind] = CRTS_TX_GAIN;
      memcpy(&fb_msg[fb_msg_ind+1], (void*)&tx_gain, sizeof(tx_gain));
      fb_msg_ind += 1+sizeof(tx_gain);
      last_tx_gain = tx_gain;
      fb_args++;
    }
  }
  
  fb_msg[1] = fb_args;
  
  // send feedback to controller
  if(fb_args > 0){
    send(*TCP_controller, fb_msg, fb_msg_ind, 0);
  }      
}

// ========================================================================
//  FUNCTION:  uhd_quiet
// ========================================================================
void uhd_quiet(uhd::msg::type_t type, const std::string &msg) {}

// ========================================================================
//  FUNCTION:  help_CRTS_interferer
// ========================================================================
void help_CRTS_interferer() {
  printf("CRTS_interferer -- Start a cognitive radio interferer node. Only "
         "needs to be run explicitly when using CRTS_controller with -m "
         "option.\n");
  printf("                -- This program must be run from the main CRTS "
         "directory.\n");
  printf(" -h : Help.\n");
  printf(" -t : Run Time - Length of time this node will run. In seconds.\n");
  printf("      Default: 20.0 s\n");
  printf(" -a : IP Address of node running CRTS_controller.\n");
  printf("      Default: 192.168.1.56.\n");
}

// ========================================================================
//  FUNCTION:  terminate
// ========================================================================
void terminate(int signum) { sig_terminate = 1; }

// ==========================================================================
// ==========================================================================
// ==========================================================================
//  MAIN PROGRAM
// ==========================================================================
// ==========================================================================
// ==========================================================================
int main(int argc, char **argv) {

  // register signal handlers
  signal(SIGINT, terminate);
  signal(SIGQUIT, terminate);
  signal(SIGTERM, terminate);

  // set default values
  time_t run_time = DEFAULT_RUN_TIME;
  char *controller_ipaddr = (char *)DEFAULT_CONTROLLER_IP_ADDRESS;
  TCP_controller = socket(AF_INET, SOCK_STREAM, 0);

  // validate TCP Controller
  if (TCP_controller < 0) {
    printf("ERROR: Receiver Failed to Create Client Socket\n");
    exit(EXIT_FAILURE);
  }

  // get command line parameters
  int d;
  while ((d = getopt(argc, argv, "ht:a:")) != EOF) {
    switch (d) {
    case 'h':
      help_CRTS_interferer();
      return 0;
    case 't':
      run_time = atof(optarg);
      break;
    case 'a':
      controller_ipaddr = optarg;
      break;
    }
  }

  // Parameters for connecting to server
  struct sockaddr_in controller_addr;
  memset(&controller_addr, 0, sizeof(controller_addr));
  controller_addr.sin_family = AF_INET;
  controller_addr.sin_addr.s_addr = inet_addr(controller_ipaddr);
  controller_addr.sin_port = htons(CRTS_TCP_CONTROL_PORT);

  // Attempt to connect client socket to server
  int connect_status =
      connect(TCP_controller, (struct sockaddr *)&controller_addr,
              sizeof(controller_addr));
  if (connect_status) {
    printf("Failed to Connect to server.\n");
    exit(EXIT_FAILURE);
  }

  uhd::msg::register_handler(&uhd_quiet);

  // Create node parameters struct and interferer object
  struct node_parameters np;
  struct scenario_parameters sp;
  Interferer *Int = new Interferer;

  int fb_enables = 0;

  // Read initial scenario info from controller
  dprintf("Receiving command from controller\n");
  receive_command_from_controller(Int, &np, &sp, &fb_enables);

  sig_terminate = 0;
  
  // wait for start time and calculate stop time
  stop_time_s = sp.start_time_s + run_time;
  while (1) {
    receive_command_from_controller(Int, &np, &sp, &fb_enables);
    gettimeofday(&tv, NULL);
    time_s = tv.tv_sec;
    if ((time_s >= start_time_s) && start_msg_received)
      break;
    if (sig_terminate)
      break;
  }

  dprintf("Starting interferer\n");
  Int->start_tx();

  // loop until end of scenario
  while (!sig_terminate && time_s < stop_time_s) {
    receive_command_from_controller(Int, &np, &sp, &fb_enables);

    if (fb_enables > 0)
      send_feedback_to_controller(&TCP_controller, fb_enables, Int);
    
    // update current time
    gettimeofday(&tv, NULL);
    time_s = tv.tv_sec;
  }

  Int->stop_tx();
  sleep(1);
  delete Int;

  if (np.generate_octave_logs) {
    char command[100];
    sprintf(command, "./logs/convert_logs_bin_to_octave -i -l %s", np.phy_tx_log_file);
    printf("command: %s\n", command);
    system(command);
  }

  dprintf("Sending termination message to controller\n");
  char term_message = CRTS_MSG_TERMINATE;
  write(TCP_controller, &term_message, 1);
}
