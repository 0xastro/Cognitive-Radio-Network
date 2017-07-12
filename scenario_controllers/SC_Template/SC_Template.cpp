#include <stdio.h>
#include <liquid/liquid.h>
#include "SC_Template.hpp"

// constructor
SC_Template::SC_Template(int argc, char **argv) {

  // default Debug Message Level
  // 0: No output
  // 1: Output on TX/RX-related events
  // 2: Also output on UHD Buffer events
  debugLevel = 0; 

  // interpret command line options
  int o;
  while ((o = getopt(argc, argv, "d:")) != EOF) {
    switch (o) {
      case 'd':
        debugLevel = atoi(optarg);
        break;
    }
  }
}

// destructor
SC_Template::~SC_Template() {}

// setup feedback enables for each fb.node
void SC_Template::initialize_node_fb() {
  // Initialize node feedback settings here. Since this template
  // is the default scenario controller, we don't define any feedback.
  // Examples of how to enable feedback can be seen in the other
  // example scenario controllers.
}

// execute function
void SC_Template::execute() {

  if (sc_event == TIMEOUT) {
    // handle scenario controller timeout event
  }

  if (sc_event == FEEDBACK) {
    // handle all possible feedback types
    switch (fb.fb_type) {
      case CRTS_TX_STATE:
        if (debugLevel>0){
          if (*(int*)fb.arg == TX_STOPPED)
            printf("Node %i has stopped transmitting\n", fb.node);
          if (*(int*)fb.arg == TX_CONTINUOUS)
            printf("Node %i has started transmitting\n", fb.node);
        }
        break;
      case CRTS_TX_FREQ:
        if (debugLevel>0)
          printf("Node %i has updated it's transmit frequency to %.1e\n", fb.node, *(double*)fb.arg);
        break;
      case CRTS_TX_RATE:
        if (debugLevel>0)
          printf("Node %i has updated it's transmit rate to %.3e\n", fb.node, *(double*)fb.arg);
        break;
      case CRTS_TX_GAIN:
        if (debugLevel>0)
          printf("Node %i has updated it's transmit gain to %.3f\n", fb.node, *(double*)fb.arg);
        break;
      case CRTS_TX_MOD:
        if (debugLevel>0)
          printf("Node %i has updated it's transmit modulation to %s\n", fb.node, 
                 modulation_types[*(int*)fb.arg].name);
        break;
      case CRTS_TX_FEC0:
        if (debugLevel>0)
          printf("Node %i has updated it's inner FEC scheme  to %s\n", fb.node, 
                 fec_scheme_str[*(int*)fb.arg][0]);
        break;
      case CRTS_TX_FEC1:
        if (debugLevel>0)
          printf("Node %i has updated it's outter FEC scheme to %s\n", fb.node, 
                 fec_scheme_str[*(int*)fb.arg][0]);
        break;
      case CRTS_RX_STATE:
        if (debugLevel>0) {
          if (*(int*)fb.arg == RX_STOPPED)
            printf("Node %i has stopped receiving\n", fb.node);
          if (*(int*)fb.arg == RX_CONTINUOUS)
            printf("Node %i has started receiving\n", fb.node);
        }
        break;
      case CRTS_RX_FREQ:
        if (debugLevel>0)
          printf("Node %i has updated it's receive frequency to %.3e\n", fb.node, *(double*)fb.arg);
        break;
      case CRTS_RX_RATE:
        if (debugLevel>0)
          printf("Node %i has updated it's receive rate to %.3e\n", fb.node, *(double*)fb.arg);
        break;
      case CRTS_RX_GAIN:
        if (debugLevel>0)
          printf("Node %i has updated it's receive gain to %.1f\n", fb.node, *(double*)fb.arg);
        break;
      case CRTS_RX_STATS:
        if (debugLevel>1) {
          struct ExtensibleCognitiveRadio::rx_statistics rx_stats = 
            *(struct ExtensibleCognitiveRadio::rx_statistics*) fb.arg;
          printf("Node %i has sent updated receive statistics:\n", fb.node);
          printf("  Number of frames received: %i\n", rx_stats.frames_received);
          printf("  Average EVM:               %.3f\n", rx_stats.evm_dB);
          printf("  Average RSSI:              %.3f\n", rx_stats.rssi_dB);
          printf("  Average PER:               %.3f\n", rx_stats.per);
          printf("  Average throughput:        %.3e\n", rx_stats.throughput);
        }
        break;
    }
  }
}



