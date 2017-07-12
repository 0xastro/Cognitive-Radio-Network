#include <stdio.h>
#include <limits.h>
#include "extensible_cognitive_radio.hpp"

void help() {
  printf("logs2octave -- Create Octave .m file to visualize logs.\n");
  printf(" -h : Help.\n");
  printf(" -l : Name of log file to process (required).\n");
  printf(" -r : Log file contains PHY receive metrics.\n");
  printf(" -t : Log file contains PHY transmit parameters.\n");
  printf(" -i : Log file contains interferer transmit parameters.\n");
  printf(" -c : Log file contains CRTS rx data.\n");
  printf(" -C : Log file contains CRTS tx data.\n");
}

int main(int argc, char **argv) {

  char log_file[PATH_MAX];
  char output_file[PATH_MAX];
  strcpy(log_file, "logs/bin/");
  strcpy(output_file, "logs/octave/"); 
  
  enum log_t { PHY_RX_LOG = 0, PHY_TX_LOG, NET_RX_LOG, NET_TX_LOG, INT_TX_LOG };
  
  // default log type is PHY RX metrics
  log_t log_type = PHY_RX_LOG;

  //unsigned int totalNumReps = 1;
  //unsigned int repNumber = 1;

  // Option flags
  int l_opt = 0;
  //int n_opt = 0;

  int d;
  while ((d = getopt(argc, argv, "hl:rticCN:n:")) != EOF) {
    switch (d) {
    case 'h':
      help();
      return 0;
    case 'l': {

      // If log files use subdirectories, create them if they don't exist
      char *subdirptr = strrchr(optarg, '/');
      if (subdirptr) {
        char subdirs[1000];
        // Get the names of the subdirectories
        strncpy(subdirs, optarg, subdirptr - optarg);
        subdirs[subdirptr - optarg] = '\0';
        char mkdir_cmd[1000];
        strcpy(mkdir_cmd, "mkdir -p ./logs/octave/");
        strcat(mkdir_cmd, subdirs);
        // Create them
        system(mkdir_cmd);
      }

        strcat(log_file, optarg);
        strcat(log_file, ".log");
        strcat(output_file, optarg);
        strcat(output_file, ".m");
        l_opt = 1;
        break;
      }
      case 'r':
        break;
      case 't':
        log_type = PHY_TX_LOG;
        break;
      case 'i':
        log_type = INT_TX_LOG;
        break;
      case 'c':
        log_type = NET_RX_LOG;
        break;
      case 'C':
        log_type = NET_TX_LOG;
        break;
      default:
        printf("Unknown argument\n");
        exit(EXIT_FAILURE);
    }
  }

  // Check that log file names were given
  if (!l_opt) {
    printf("Please give -l option.\n\n");
    help();
    return 1;
  }
  
  printf("Log file name: %s\n", log_file);
  printf("Output file name: %s\n", output_file);

  FILE *file_in = fopen(log_file, "rb");
  FILE *file_out = fopen(output_file, "w");
  
  struct ExtensibleCognitiveRadio::metric_s metrics = {};
  struct ExtensibleCognitiveRadio::rx_parameter_s rx_params = {};
  struct ExtensibleCognitiveRadio::tx_parameter_s tx_params = {};
  int i = 1;

  // handle log type
  switch (log_type) {
  // handle PHY RX log
  case PHY_RX_LOG: {
    while (fread((char *)&metrics,
                 sizeof(struct ExtensibleCognitiveRadio::metric_s), 1,
                 file_in)) {
      fread((char *)&rx_params,
            sizeof(struct ExtensibleCognitiveRadio::rx_parameter_s), 1,
            file_in);
      
      // write raw data file
      fprintf(file_out, "phy_rx_t(%i) = %li + %f;\n", i,
              metrics.time_spec.get_full_secs(),
              metrics.time_spec.get_frac_secs());
      fprintf(file_out, "phy_rx_Control_valid(%i) = %i;\n", i,
              metrics.control_valid);
      fprintf(file_out, "phy_rx_Payload_valid(%i) = %i;\n", i,
              metrics.payload_valid);
      fprintf(file_out, "phy_rx_EVM(%i) = %f;\n", i, metrics.stats.evm);
      fprintf(file_out, "phy_rx_RSSI(%i) = %f;\n", i, metrics.stats.rssi);
      fprintf(file_out, "phy_rx_CFO(%i) = %f;\n", i, metrics.stats.cfo);
      fprintf(file_out, "phy_rx_payload_bytes(%i) = %i;\n", i,
              metrics.payload_len);
      fprintf(file_out, "phy_rx_mod_scheme(%i) = %i;\n", i,
              metrics.stats.mod_scheme);
      fprintf(file_out, "phy_rx_BPS(%i) = %i;\n", i, metrics.stats.mod_bps);
      fprintf(file_out, "phy_rx_fec0(%i) = %i;\n", i, metrics.stats.fec0);
      fprintf(file_out, "phy_rx_fec1(%i) = %i;\n\n", i, metrics.stats.fec1);
      fprintf(file_out, "phy_rx_numSubcarriers(%i) = %u;\n", i,
              rx_params.numSubcarriers);
      fprintf(file_out, "phy_rx_cp_len(%i) = %u;\n", i, rx_params.cp_len);
      fprintf(file_out, "phy_rx_taper_len(%i) = %u;\n", i, rx_params.taper_len);
      fprintf(file_out, "phy_rx_gain_uhd(%i) = %f;\n", i,
              rx_params.rx_gain_uhd);
      fprintf(file_out, "phy_rx_freq(%i) = %f;\n", i,
              rx_params.rx_freq - rx_params.rx_dsp_freq);
      fprintf(file_out, "phy_rx_rate(%i) = %f;\n", i, rx_params.rx_rate);
      i++;
    }
    break;
  }

  // handle PHY TX log
  case PHY_TX_LOG: {
    struct timeval log_time;
    ;

    while (fread((struct timeval *)&log_time, sizeof(struct timeval), 1,
                 file_in)) {
      fread((char *)&tx_params,
            sizeof(struct ExtensibleCognitiveRadio::tx_parameter_s), 1,
            file_in);
      fprintf(file_out, "phy_tx_t(%i) = %li + 1e-6*%li;\n", i, log_time.tv_sec,
              log_time.tv_usec);
      fprintf(file_out, "phy_tx_numSubcarriers(%i) = %u;\n", i,
              tx_params.numSubcarriers);
      fprintf(file_out, "phy_tx_cp_len(%i) = %u;\n", i, tx_params.cp_len);
      fprintf(file_out, "phy_tx_taper_len(%i) = %u;\n", i, tx_params.taper_len);
      fprintf(file_out, "phy_tx_gain_uhd(%i) = %f;\n", i,
              tx_params.tx_gain_uhd);
      fprintf(file_out, "phy_tx_gain_soft(%i) = %f;\n", i,
              tx_params.tx_gain_soft);
      fprintf(file_out, "phy_tx_freq(%i) = %f;\n", i,
              tx_params.tx_freq + tx_params.tx_dsp_freq);
      fprintf(file_out, "phy_tx_lo_freq(%i) = %f;\n", i, tx_params.tx_freq);
      fprintf(file_out, "phy_tx_dsp_freq(%i) = %f;\n", i,
              tx_params.tx_dsp_freq);
      fprintf(file_out, "phy_tx_rate(%i) = %f;\n", i, tx_params.tx_rate);
      fprintf(file_out, "phy_tx_mod_scheme(%i) = %i;\n", i,
              tx_params.fgprops.mod_scheme);
      fprintf(file_out, "phy_tx_fec0(%i) = %i;\n", i, tx_params.fgprops.fec0);
      fprintf(file_out, "phy_tx_fec1(%i) = %i;\n\n", i, tx_params.fgprops.fec1);
      i++;
    }

    break;
  }

  // handle interferer tx logs
  case INT_TX_LOG: {
    struct timeval log_time;

    float tx_freq;
    while (fread((struct timeval *)&log_time, sizeof(struct timeval), 1,
                 file_in)) {
      fread((float *)&tx_freq, sizeof(float), 1, file_in);
      fprintf(file_out, "Int_tx_t(%i) = %li + 1e-6*%li;\n", i, log_time.tv_sec,
              log_time.tv_usec);
      fprintf(file_out, "Int_tx_freq(%i) = %f;\n\n", i, tx_freq);
      i++;
    }
    break;
  }

  // handle NET RX data logs
  case NET_RX_LOG: {
    struct timeval log_time;
    int bytes;
    int packet_num;
    while (fread((struct timeval *)&log_time, sizeof(struct timeval), 1,
                 file_in)) {
      fread((int *)&bytes, sizeof(int), 1, file_in);
      fread((int *)&packet_num, sizeof(int), 1, file_in);
      fprintf(file_out, "net_rx_t(%i) = %li + 1e-6*%li;\n", i, log_time.tv_sec,
              log_time.tv_usec);
      fprintf(file_out, "net_rx_bytes(%i) = %i;\n", i, bytes);
      fprintf(file_out, "net_rx_packet_num(%i) = %i;\n", i, packet_num);
      i++;
    }
    break;
  }
  case NET_TX_LOG: {
    struct timeval log_time;
    int bytes;
    int packet_num;
    while (fread((struct timeval *)&log_time, sizeof(struct timeval), 1,
                 file_in)) {
      fread((int *)&bytes, sizeof(int), 1, file_in);
      fread((int *)&packet_num, sizeof(int), 1, file_in);
      fprintf(file_out, "net_tx_t(%i) = %li + 1e-6*%li;\n", i, log_time.tv_sec,
              log_time.tv_usec);
      fprintf(file_out, "net_tx_bytes(%i) = %i;\n\n", i, bytes);
      fprintf(file_out, "net_tx_packet_num(%i) = %i;\n\n", i, packet_num);
      i++;
    }
    break;
  }
  }

  fclose(file_in);
  fclose(file_out);
}
