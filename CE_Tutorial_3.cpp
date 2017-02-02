#include "extensible_cognitive_radio.hpp"
#include "CE_Tutorial_3.hpp"

// constructor
CE_Tutorial_3::CE_Tutorial_3(int argc, char **argv, ExtensibleCognitiveRadio *_ECR) {

  // save the ECR pointer (this should not be removed)
  ECR = _ECR;
	

  //initialize all of our members --astro 14-Dec-2016
			print_stats_timer = timer_create();
			timer_tic(print_stats_timer);
			tx_gain_timer = timer_create();
			timer_tic(tx_gain_timer);
			frame_counter = 0;
			frame_errs = 0;
			sum_evm = 0.0;
			sum_rssi = 0.0;	
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
CE_Tutorial_3::~CE_Tutorial_3() {
//make sure we clean up the timers --astro 14-Dec-2016
	timer_destroy(print_stats_timer);
	timer_destrpy(tx_gain_timer);
}

// execute function
void CE_Tutorial_3::execute() {

  switch(ECR->CE_metrics.CE_event) {
    case ExtensibleCognitiveRadio::TIMEOUT:
      // handle timeout events
      if (debugLevel>0) {printf("TIMEOUT Event!\n");}
      break;
    case ExtensibleCognitiveRadio::PHY_FRAME_RECEIVED:
      // handle physical layer frame reception events

      if (debugLevel>0) {printf("PHY Event!\n");}
	frame_counter++; //--astro 14-Dec-2016
	if (!ECR->CE_metrics.payload_valid)
	frame_errs++;
	sum_evm += pow(10.0, ECR->CE_metrics.stats.evm/10.0);
	sum_rssi += pow(10.0, ECR->CE_metrics.stats.rssi/10.0);      
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
