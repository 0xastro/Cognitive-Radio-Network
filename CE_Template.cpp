#include "extensible_cognitive_radio.hpp"
#include "CE_Template.hpp"
	/*
	███████╗████████╗███████╗██████╗              ██╗
	██╔════╝╚══██╔══╝██╔════╝██╔══██╗            ███║
	███████╗   ██║   █████╗  ██████╔╝            ╚██║
	╚════██║   ██║   ██╔══╝  ██╔═══╝              ██║
	███████║   ██║   ███████╗██║                  ██║
	╚══════╝   ╚═╝   ╚══════╝╚═╝                  ╚═╝   	CognitiveEngine Class Reference 11.1
*/
/* Public Attributes
	•ExtensibleCognitiveRadio *ECR
The base class for the custom cognitive engines built using the ECR (Extensible Cognitive Radio).
This class is used as the base for the custom (user-defined) cognitive engines (CEs) 
placed in the cognitive_engines/ directory of the source tree.
The CEs following this model are event-driven: 
While the radio is running,if certain events occur as defined in ExtensibleCognitiveRadio::Event, 
then the custom-defined execute function
(Cognitive_Engine::execute()) will be called.
*/
//If our Engine is called CE_ANN so We must replace each CE_Template by CE_ANN
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


//All our Meters will be based on Phsical Layer AND maybe MAC will involved
// so We will calculate some statistics on PHY_FRAME_RECEIVED
/*
--------------------------------------------------------------------------
		Meters					-	Knobs								 -
--------------------------------------------------------------------------
PHY:	BER						-	CARRIER FREQUENCY/CHANNEL ALLOCATION -
		SINR					-										 -
		NOISE POWER				-									     -
		RSSI					-										 -
		RECIEVED SIGNAL POWER	-					                     -
		INTERFERENCE POWER		-										 -
		EVM						-										 -
		PER						-										 -
MAC:
		FRAME ERROR RATE     	-										 -
		DATA RATE               -									     -
--------------------------------------------------------------------------
	*/
/*	
███████╗████████╗███████╗██████╗             ██████╗ 
██╔════╝╚══██╔══╝██╔════╝██╔══██╗            ╚════██╗
███████╗   ██║   █████╗  ██████╔╝             █████╔╝
╚════██║   ██║   ██╔══╝  ██╔═══╝             ██╔═══╝ 
███████║   ██║   ███████╗██║                 ███████╗
╚══════╝   ╚═╝   ╚══════╝╚═╝                 ╚══════╝
 CognitiveEngine Class Reference 11.1.2
*/

// execute function
//virtual void execute ()
/*Execute the custom Cognitive Engine as we define
When writing a custom cognitive engine (CE) 
using the Extensible Cognitive Radio (ECR), 
this function should be defined to contain the main processing of the CE. 
An ECR CE is event-driven: When the radio is running,
this Cognitive_Engine::execute() function is called if certain events, 
as defined in ExtensibleCognitiveRadio::Event, occur.
Event could be as below: TIMEOUT, PHY_FRAME_RECEIVED, TX_COMPLETE, UHD_OVERFLOW,UHD_UNDERRUN,USRP_RX_SAMPS ..etc
*/
//Docs for this class generated from: crts/include/cognitive_engine.hpp 
//								 	  crts/src/cognitive_engine.cpp 
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
