#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "crts.hpp"
#include "extensible_cognitive_radio.hpp"

// EDIT INCLUDE START FLAG
#include "../scenario_controllers/SC_Performance_Sweep_Utility/SC_Performance_Sweep_Utility.hpp"
#include "../scenario_controllers/SC_BER_Sweep/SC_BER_Sweep.hpp"
#include "../scenario_controllers/SC_Template/SC_Template.hpp"
#include "../scenario_controllers/SC_Network_Loading/SC_Network_Loading.hpp"
#include "../scenario_controllers/SC_CORNET_Display/SC_CORNET_Display.hpp"
#include "../scenario_controllers/SC_Control_and_Feedback_Test/SC_Control_and_Feedback_Test.hpp"
#include "../scenario_controllers/SC_CORNET_Tutorial/SC_CORNET_Tutorial.hpp"
// EDIT INCLUDE END FLAG

#define MAXPENDING 5

// global variables
int sig_terminate;
int num_nodes_terminated;
long int bytes_sent[CRTS_MAX_NODES];
long int bytes_received[CRTS_MAX_NODES];

int receive_msg_from_nodes(int *TCP_nodes, int num_nodes, ScenarioController *SC) {
  // Listen to sockets for messages from any node
  char msg[256];
  for (int i = 0; i < num_nodes; i++) {
    int rflag = recv(TCP_nodes[i], msg, 1, 0);
    int err = errno;

    // Handle errors
    if (rflag <= 0) {
      if (!((err == EAGAIN) || (err == EWOULDBLOCK))) {
        close(TCP_nodes[i]);
        printf(
            "Node %i has disconnected. Terminating the current scenario..\n\n",
            i + 1);
        // tell all nodes to terminate program
        for (int j = 0; j < num_nodes; j++) {
          write(TCP_nodes[j], &msg, 1);
        }
        return 1;
      }
    }

    // Parse command if received a message
    else {
      switch (msg[0]) {
        case CRTS_MSG_TERMINATE: // terminate program
          printf("Node %i has sent a termination message...\n", i+1);
          num_nodes_terminated++;
          // check if all nodes have terminated
          if (num_nodes_terminated == num_nodes)
            return 1;
          break;
        case CRTS_MSG_FEEDBACK:{
          // receive the number of feedback arguments sent
          rflag = recv(TCP_nodes[i], &msg[1], 1, 0);
          int fb_msg_ind = 2;
          // receive all feedback arguments
          for(int j=0; j<msg[1]; j++){
            rflag = recv(TCP_nodes[i], &msg[fb_msg_ind], 1, 0);
            int fb_arg_len = get_feedback_arg_len(msg[fb_msg_ind]);
            fb_msg_ind++;
            
            rflag = recv(TCP_nodes[i], &msg[fb_msg_ind], fb_arg_len, 0);
            SC->receive_feedback(i, msg[fb_msg_ind-1], (void *) &msg[fb_msg_ind]);
            fb_msg_ind += fb_arg_len;
          }
          break;
        }
        case CRTS_MSG_SUMMARY:
          // receive the number of bytes sent and received
          rflag = recv(TCP_nodes[i], &bytes_sent[i], sizeof(long int), 0);
          rflag = recv(TCP_nodes[i], &bytes_received[i], sizeof(long int), 0);
          break;
        default:
          printf("Invalid message type received from node %i\n", i+1);
      }
    }
  }

  return 0;
}

ScenarioController* create_sc(struct scenario_parameters *sp){
  int argc = 0;
  char ** argv = NULL;
  str2argcargv(sp->sc_args, sp->SC, argc, argv);

  ScenarioController *SC;

  // EDIT SET SC START FLAG
  if(!strcmp(sp->SC, "SC_Performance_Sweep_Utility"))
    SC = new SC_Performance_Sweep_Utility(argc, argv);
  if(!strcmp(sp->SC, "SC_BER_Sweep"))
    SC = new SC_BER_Sweep(argc, argv);
  if(!strcmp(sp->SC, "SC_Template"))
    SC = new SC_Template(argc, argv);
  if(!strcmp(sp->SC, "SC_Network_Loading"))
    SC = new SC_Network_Loading(argc, argv);
  if(!strcmp(sp->SC, "SC_CORNET_Display"))
    SC = new SC_CORNET_Display(argc, argv);
  if(!strcmp(sp->SC, "SC_Control_and_Feedback_Test"))
    SC = new SC_Control_and_Feedback_Test(argc, argv);
  if(!strcmp(sp->SC, "SC_CORNET_Tutorial"))
    SC = new SC_CORNET_Tutorial(argc, argv);
  // EDIT SET SC END FLAG
    
  freeargcargv(argc, argv);
 
  return SC;
}

void log_scenario_summary(int scenario_num, int rep_num, char *scenario_master_name,
                          char *scenario_name, struct scenario_parameters *sp){
 
  char log_summary_filename[100];
  sprintf(log_summary_filename, "logs/octave/%s_summary.m", scenario_master_name);

  // Append to file if not first scenario and first rep
  FILE *log_summary;  
  if ((scenario_num>1) || (rep_num>1))
    log_summary = fopen(log_summary_filename, "a");
  else{
    log_summary = fopen(log_summary_filename, "w");
  }

  if (rep_num == 1) {
    fprintf(log_summary, "scenario_name{%i} = '%s';\n", scenario_num, scenario_name);
    fprintf(log_summary, "run_time(%i) = %li;\n", scenario_num, sp->run_time);
    fprintf(log_summary, "num_nodes(%i) = %i;\n\n", scenario_num, sp->num_nodes);
  }
  for (int i=0; i< sp->num_nodes; i++){
    fprintf(log_summary, "bytes_sent(%i,%i,%i) = %li;\n", 
            scenario_num, i+1, rep_num, bytes_sent[i]);
    fprintf(log_summary, "bytes_received(%i,%i,%i) = %li;\n", 
            scenario_num, i+1, rep_num, bytes_received[i]);
  }
  fprintf(log_summary, "\n");
  fclose(log_summary);
}

void help_crts_controller() {
  printf("crts_controller -- Initiates cognitive radio testing and provides means\n"
         "  to obtain real-time feedback and exert control over the test scenarios.\n");
  printf(" -h : Help.\n");
  printf(" -m : Manual Mode - Start each node manually rather than have\n"
         "      crts_controller do it automatically.\n");
  printf(" -f : Master scenario configuration (default: scenario_master_template).\n");
  printf(" -a : IP Address - IP address of this computer as seen by remote nodes.\n"
         "      Autodetected by default.\n");
  printf(" -s : Scenario - Specifies a single scenario to run rather than using a\n"
         "      scenario master file. The path is assumed to start in /scenarios/ e.g.\n"
         "      -s test_scenarios/tx_gain_sweep\n");
  printf(" -r : Repetitions - Specifies the number of times the provided scenario\n"
         "      will be repeated. This only applies when a scenario is specified with\n"
         "      the -s option\n");
}

void terminate(int signum) {
  printf("Terminating scenario on all nodes\n");
  sig_terminate = 1;
}

int main(int argc, char **argv) {

  // register signal handlers
  signal(SIGINT, terminate);
  signal(SIGQUIT, terminate);
  signal(SIGTERM, terminate);

  sig_terminate = 0;

  int manual_execution = 0;

  // Use current username as default username for ssh
  const char* ssh_uname = std::getenv("LOGNAME");

  // Use currnet location of CRTS Directory as defualt for ssh
  char crts_dir[1000];
  getcwd(crts_dir, 1000);

  // Default name of master scenario file
  char scenario_master_name[100];
  strcpy(scenario_master_name, "scenario_master_template");

  // Default IP address of server as seen by other nodes
  char *serv_ip_addr;
  // Autodetect IP address as seen by other nodes
  struct ifreq interfaces;
  // Create a socket
  int fd_ip = socket(AF_INET, SOCK_DGRAM, 0);
  // For IPv4 Address
  interfaces.ifr_addr.sa_family = AF_INET;
  // Get Address associated with eth0
  strncpy(interfaces.ifr_name, "eth0", IFNAMSIZ - 1);
  ioctl(fd_ip, SIOCGIFADDR, &interfaces);
  close(fd_ip);
  // Get IP address out of struct
  char serv_ip_addr_auto[30];
  strcpy(serv_ip_addr_auto,
         inet_ntoa(((struct sockaddr_in *)&interfaces.ifr_addr)->sin_addr));
  serv_ip_addr = serv_ip_addr_auto;

  char scenario_file[255];
  unsigned int scenario_reps = 1;
  bool scenario_opt_given = false;

  // interpret command line options
  int d;
  while ((d = getopt(argc, argv, "hf:ma:s:r:")) != EOF) {
    switch (d) {
    case 'h':
      help_crts_controller();
      return 0;
    case 'f':
      strcpy(scenario_master_name, optarg);
      break;
    case 'm':
      manual_execution = 1;
      break;
    case 'a':
      serv_ip_addr = optarg;
      break;
    case 's':
      scenario_opt_given = true;
      strcpy(scenario_file,optarg);
      break;
    case 'r':
      scenario_reps = atoi(optarg);
      break;
    }
  }

  optind = 0;

  // Message about IP Address Detection
  printf("IP address of CRTS_controller autodetected as %s\n",
         serv_ip_addr_auto);
  printf("If this is incorrect, use -a option to fix.\n\n");

  // Create socket for incoming connections
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Transmitter Failed to Create Server Socket.\n");
    exit(EXIT_FAILURE);
  }
  // Allow reuse of a port. See
  // http://stackoverflow.com/questions/14388706/socket-options-so-reuseaddr-and-so-reuseport-how-do-they-differ-do-they-mean-t
  // FIXME: May not be necessary in this version of CRTS
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    printf("setsockopt() failed\n");
    exit(EXIT_FAILURE);
  }
  // Construct local (server) address structure
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));          // Zero out structure
  serv_addr.sin_family = AF_INET;                    // Internet address family
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);     // Any incoming interface
  serv_addr.sin_port = htons(CRTS_TCP_CONTROL_PORT); // Local port
  // Bind to the local address to a port
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("ERROR: bind() error\n");
    exit(EXIT_FAILURE);
  }

  // objects needs for TCP links to cognitive radio nodes
  int TCP_nodes[48];
  struct sockaddr_in nodeAddr[48];
  socklen_t node_addr_size[48];
  for (int i = 0; i < 48; i++)
    node_addr_size[i] = sizeof(nodeAddr[i]);
  struct node_parameters np[48];

  // read master scenario config file
  int num_scenarios;
  char scenario_name[251];
  char *scenario_name_ptr;
  bool octave_log_summary;
  
  if (!scenario_opt_given) {
    read_master_parameters(scenario_master_name, &num_scenarios, &octave_log_summary);
  } else {
    num_scenarios = 1;
    octave_log_summary = true;
  }

  if (octave_log_summary);
    printf("Will generate a summary octave script: /logs/octave/%s.m\n", scenario_master_name);
  printf("Number of scenarios: %i\n\n", num_scenarios);
  
  // variables for reading the system clock
  struct timeval tv;
  time_t time_s;
  int64_t start_time_s;

  // loop through scenarios
  for (int i = 0; i < num_scenarios; i++) {

    // read scenario file name and repetitions if they were not specified
    if (!scenario_opt_given)
      scenario_reps =
          read_master_scenario(scenario_master_name, i + 1, scenario_file);
    // store the full path and scenario name separately
    scenario_name_ptr = strrchr(scenario_file,'/');
    if(scenario_name_ptr != NULL) {
      scenario_name_ptr++;
      strcpy(scenario_name, scenario_name_ptr);
    }
    strcat(scenario_file, ".cfg");

    if (scenario_opt_given)
      strcpy(scenario_master_name, scenario_name);
      
    for (unsigned int rep_i = 1; rep_i <= scenario_reps; rep_i++) {
      printf("Scenario %i: %s\n", i + 1, scenario_name);
      printf("Rep: %i\n", rep_i);
      // read the scenario parameters from file
      struct scenario_parameters sp = read_scenario_parameters(scenario_file);
      // Set the number of scenario  repititions in struct.
      sp.total_num_reps = scenario_reps;
      sp.rep_num = rep_i;

      printf("Number of nodes: %i\n", sp.num_nodes);
      printf("Run time: %lld\n", (long long)sp.run_time);
      printf("Scenario controller: %s\n", sp.SC);

      // create the scenario controller
      ScenarioController *SC = create_sc(&sp); 

      // determine the start time for the scenario based
      // on the current time and the number of nodes
      gettimeofday(&tv, NULL);
      time_s = tv.tv_sec;
      int pad_s = manual_execution ? 120 : 10;
      sp.start_time_s = (int64_t) (time_s + 1 * sp.num_nodes + pad_s);

      // loop through nodes in scenario
      for (int j = 0; j < sp.num_nodes; j++) {
        char node_id[10];
        snprintf(node_id, 10, "%d", j + 1);

        // read in node settings
        memset(&np[j], 0, sizeof(struct node_parameters));
        printf("Reading node %i's parameters...\n", j + 1);
        np[j] = read_node_parameters(j + 1, scenario_file);
       
        // define log file names if they weren't defined by the scenario
        if (!strcmp(np[j].phy_rx_log_file, "")) {
          strcpy(np[j].phy_rx_log_file, scenario_name);
          sprintf(np[j].phy_rx_log_file, "%s_node_%i%s", np[j].phy_rx_log_file,
                  j + 1, "_cognitive_radio_phy_rx");
        }

        if (!strcmp(np[j].phy_tx_log_file, "")) {
          strcpy(np[j].phy_tx_log_file, scenario_name);
          sprintf(np[j].phy_tx_log_file, "%s_node_%i", np[j].phy_tx_log_file,
                  j + 1);
          switch (np[j].node_type) {
          case (COGNITIVE_RADIO):
            sprintf(np[j].phy_tx_log_file, "%s%s", np[j].phy_tx_log_file,
                    "_cognitive_radio_phy_tx");
            break;
          case (INTERFERER):
            sprintf(np[j].phy_tx_log_file, "%s%s", np[j].phy_tx_log_file,
                    "_interferer_phy_tx");
            break;
          }
        }

        if (!strcmp(np[j].net_rx_log_file, "")) {
          strcpy(np[j].net_rx_log_file, scenario_name);
          sprintf(np[j].net_rx_log_file, "%s_node_%i%s", np[j].net_rx_log_file,
                  j + 1, "_cognitive_radio_net_rx");
        }

        if (!strcmp(np[j].net_tx_log_file, "")) {
          strcpy(np[j].net_tx_log_file, scenario_name);
          sprintf(np[j].net_tx_log_file, "%s_node_%i%s", np[j].net_tx_log_file,
                  j + 1, "_cognitive_radio_net_tx");
        }

        // append the rep number if necessary
        if (scenario_reps - 1) {
          sprintf(np[j].phy_rx_log_file, "%s_rep_%i", np[j].phy_rx_log_file,
                  rep_i);
          sprintf(np[j].phy_tx_log_file, "%s_rep_%i", np[j].phy_tx_log_file,
                  rep_i);
          sprintf(np[j].net_tx_log_file, "%s_rep_%i", np[j].net_tx_log_file,
                  rep_i);
          sprintf(np[j].net_rx_log_file, "%s_rep_%i", np[j].net_rx_log_file,
                  rep_i);
        }

        print_node_parameters(&np[j]);

        // send command to launch executable if not doing so manually
        int ssh_return = 0;
        if (!manual_execution) {

          char executable[20];
          switch (np[j].node_type) {
            case COGNITIVE_RADIO: sprintf(executable, "crts_cognitive_radio"); break;
            case INTERFERER: sprintf(executable, "crts_interferer"); break;
          }

          char sysout_log[200];
          sprintf(sysout_log, "%s/logs/sysout/%s_node_%i_rep_%i.sysout", crts_dir, scenario_name, j+1, rep_i);

          char command[2000];
          sprintf(command, "ssh %s@%s 'sleep 1 && cd %s && ./%s -a %s 2>&1 &' > %s &",
                  ssh_uname, np[j].server_ip, crts_dir, executable, serv_ip_addr, sysout_log);
          
          ssh_return = system(command);
          if (ssh_return) {
            printf("SSH failed for node %i with address %s\n", j+1, np[j].server_ip); 
            exit(EXIT_FAILURE);
          }
        }

        if (ssh_return != 0) {
          printf("Failed to execute CRTS on node %i\n", j + 1);
          sig_terminate = 1;
          break;
        }

        // listen for node to connect to server
        if (listen(sockfd, MAXPENDING) < 0) {
          fprintf(stderr, "ERROR: Failed to Set Sleeping (listening) Mode\n");
          exit(EXIT_FAILURE);
        }

        // loop to accept
        // accept connection
        int accepted = 0;
        fd_set fds;
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        while (!sig_terminate && !accepted) {
          FD_ZERO(&fds);
          FD_SET(sockfd, &fds);
          if (select(sockfd + 1, &fds, NULL, NULL, &timeout) > 0) {
            TCP_nodes[j] = accept(sockfd, (struct sockaddr *)&nodeAddr[j],
                               &node_addr_size[j]);
            if (TCP_nodes[j] < 0) {
              fprintf(stderr, "ERROR: Sever Failed to Connect to Client\n");
              exit(EXIT_FAILURE);
            }

            accepted = 1;
          }
        }

        if (sig_terminate)
          break;

        // set socket to non-blocking
        fcntl(TCP_nodes[j], F_SETFL, O_NONBLOCK);

        // copy IP of connected node to node parameters if in manual mode
        if (manual_execution)
          strcpy(np[j].server_ip, inet_ntoa(nodeAddr[j].sin_addr));

        // send scenario and node parameters
        printf("\nNode %i has connected. Sending its parameters...\n", j + 1);
        char msg_type = CRTS_MSG_SCENARIO_PARAMETERS;
        send(TCP_nodes[j], (void *)&msg_type, sizeof(char), 0);
        send(TCP_nodes[j], (void *)&sp, sizeof(struct scenario_parameters), 0);
        send(TCP_nodes[j], (void *)&np[j], sizeof(struct node_parameters), 0);
      
        // copy node parameters to the scenario controller object
        SC->np[j] = np[j]; 
      }

      printf("\n");

      // initialize feedback settings on nodes
      SC->TCP_nodes = TCP_nodes;
      SC->sp = sp;
      SC->initialize_node_fb();

      // if in manual mode update the start time for all nodes
      if (!sig_terminate) {

        gettimeofday(&tv, NULL);
        start_time_s = (int64_t) (tv.tv_sec + 3);

        // send updated start time to all nodes
        char msg_type = CRTS_MSG_START;
        for (int j = 0; j < sp.num_nodes; j++) {
          send(TCP_nodes[j], (void *)&msg_type, sizeof(char), 0);
          send(TCP_nodes[j], (void *)&start_time_s, sizeof(int64_t), 0);
        }
      }

      // wait until start time
      while (1) {
        gettimeofday(&tv, NULL);
        time_s = tv.tv_sec;
        if (time_s >= start_time_s)
          break;
        if (sig_terminate)
          break;
        usleep(1e4);
      }

      SC->set_sc_timeout_ms(sp.sc_timeout_ms);
      SC->start_sc();
      
      // main loop: wait for any of three possible termination conditions
      int time_terminate = 0;
      int msg_terminate = 0;
      num_nodes_terminated = 0;
      while ((!sig_terminate) && (!msg_terminate) && (!time_terminate)) { 
        msg_terminate = receive_msg_from_nodes(&TCP_nodes[0], sp.num_nodes, SC);

        // Check if the scenario should be terminated based on the elapsed time.
        // Note that by default the nodes should terminate on their own. This is
        // just to handle the case where a node or nodes do not terminate properly.
        gettimeofday(&tv, NULL);
        time_s = tv.tv_sec;
        if (time_s > (time_t) start_time_s + (time_t) sp.run_time + 10)
          time_terminate = 1;
      }

      if (msg_terminate)
        printf("Ending scenario %i because all nodes have sent a "
               "termination message\n",
               i + 1);

      // terminate the current scenario on all nodes
      if (sig_terminate) {
        printf("Sending message to terminate nodes\n");
        // tell all nodes in the scenario to terminate the testing process
        char msg = CRTS_MSG_TERMINATE;
        for (int j = 0; j < sp.num_nodes; j++) {
          write(TCP_nodes[j], &msg, 1);
        }

        // get current time
        gettimeofday(&tv, NULL);
        time_t msg_sent_time_s = tv.tv_sec;

        // listen for confirmation that all nodes have terminated
        while ((!msg_terminate) && (!time_terminate)) {
          msg_terminate = receive_msg_from_nodes(&TCP_nodes[0], sp.num_nodes, SC);

          // check if the scenario should be terminated based on the elapsed
          // time
          gettimeofday(&tv, NULL);
          time_s = tv.tv_sec;
          if (time_s > msg_sent_time_s + CRTS_FORCEFUL_TERMINATION_DELAY_S) {
            printf("Nodes have not all responded with a successful "
                   "termination... forciblly terminating any CRTS processes "
                   "still running\n");
            time_terminate = 1;
          }
        }
      }

      // forcefully terminate all processes if one or more has failed to
      // terminate gracefully
      if (time_terminate) {
        for (int j = 0; j < sp.num_nodes; j++) {
          printf("Running CRTS_CR cleanup on node %i: %s\n", j+1, np[j].server_ip);
          char command[2000] = "ssh ";
          sprintf(command, "ssh %s@%s 'python %s/src/terminate_crts_cognitive_radio.py'",
                  ssh_uname, np[j].server_ip, crts_dir); 
          int ssh_return = system(command);
          if (ssh_return < 0)
            printf("Error terminating CRTS on node %i: %s", j+1, np[j].server_ip);
        }
      }
      
      // Close TCP Connections
      for (int j = 0; j < sp.num_nodes; j++) {
        close(TCP_nodes[j]);
      }

      SC->stop_sc();
      delete SC;

      if (octave_log_summary)
        log_scenario_summary(i+1, rep_i, scenario_master_name, scenario_name, &sp);
      
      // don't continue to next scenario if there was a user issued termination
      if (sig_terminate)
        break;
      
    } // scenario repition loop

    // don't continue to next scenario if there was a user issued termination
    if (sig_terminate)
      break;

  } // scenario loop

} // main
