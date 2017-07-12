#include "extensible_cognitive_radio.hpp"
#include "CE_Template.hpp"

// constructor
CE_Template::CE_Template(int argc, char **argv, ExtensibleCognitiveRadio *_ECR) {

  // save the ECR pointer (this should not be removed)
  ECR = _ECR;

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
CE_Template::~CE_Template() {}

// execute function
void CE_Template::execute() {

  switch(ECR->CE_metrics.CE_event) {
    case ExtensibleCognitiveRadio::TIMEOUT:
      // handle timeout events
      if (debugLevel>0) {printf("TIMEOUT Event!\n");}
      break;
    case ExtensibleCognitiveRadio::PHY_FRAME_RECEIVED:
      // handle physical layer frame reception events
      if (debugLevel>0) {printf("PHY Event!\n");}
      break;
    case ExtensibleCognitiveRadio::TX_COMPLETE:
      // handle transmission complete events
      if (debugLevel>0) {printf("TX_COMPLETE Event!\n");}
      break;
    case ExtensibleCognitiveRadio::UHD_OVERFLOW:
      // handle UHD overflow events
      if (debugLevel>1) {printf("UHD_OVERFLOW Event!\n");}
      break;
    case ExtensibleCognitiveRadio::UHD_UNDERRUN:
      // handle UHD underrun events
      if (debugLevel>1) {printf("UHD_UNDERRUN Event!\n");}
      break;
    case ExtensibleCognitiveRadio::USRP_RX_SAMPS:
      // handle samples received from the USRP when simultaneously
      // running the receiver and performing additional sensing
      if (debugLevel>0) {printf("USRP_RX_SAMPS Event!\n");}
      break;
  }
}
