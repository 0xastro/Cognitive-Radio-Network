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

/*
███████╗████████╗███████╗██████╗             ██████╗ 
██╔════╝╚══██╔══╝██╔════╝██╔══██╗            ╚════██╗
███████╗   ██║   █████╗  ██████╔╝             █████╔╝
╚════██║   ██║   ██╔══╝  ██╔═══╝              ╚═══██╗
███████║   ██║   ███████╗██║                 ██████╔╝
╚══════╝   ╚═╝   ╚══════╝╚═╝                 ╚═════╝ 
*/	//Events are defined as enum data type
/*	enum CE_Event {TIMEOUT = 0, PHY_FRAME_RECEIVED, TX_COMPLETE, UHD_OVERFLOW, UHD_UNDERRUN, USRP_RX_SAMPS }
	//Defines the different types of CE events.

	enum FrameType { DATA = 0, CONTROL, UNKNOWN }
	//Defines the types of frames used by the ECR.
					=============================================
					11.2 ExtensibleCognitiveRadio Class Reference
					=============================================
										Classes
										
//•struct metric_s
/*Contains metric information related to the quality of a received frame.
 This information is made available to the custom Cognitive_Engine::execute() implementation
and is accessed in the instance of this struct: ExtensibleCognitiveRadio-::CE_metrics.
*/

//•struct rx_parameter_s
/* 
Contains parameters defining how to handle frame reception.
*/

//• struct rx_statistics
//• struct tx_parameter_s
/*Contains parameters defining how to handle frame transmission.
*/




/*
███████╗████████╗███████╗██████╗             ██╗  ██╗
██╔════╝╚══██╔══╝██╔════╝██╔══██╗            ██║  ██║
███████╗   ██║   █████╗  ██████╔╝            ███████║
╚════██║   ██║   ██╔══╝  ██╔═══╝             ╚════██║
███████║   ██║   ███████╗██║                      ██║
╚══════╝   ╚═╝   ╚══════╝╚═╝                      ╚═╝Diving Deep Into The public member functions Which We could be used
*///hint this is not a code, but it's wrote as a one to be clear.
• void set_ce (char *ce, int argc, char **argv)
• void start_ce ()
• void stop_ce ()

• void set_ce_timeout_ms (double new_timeout_ms)
//	Assign a value to ExtensibleCognitiveRadio::ce_timeout_ms.
• double get_ce_timeout_ms ()
//	Get the current value of ExtensibleCognitiveRadio::ce_timeout_ms.

• void set_ce_sensing (int ce_sensing)
//	Allows you to turn on/off the USRP_RX_SAMPLES events which allow you to perform custom spectrum sensing in
	the CE while the liquid-ofdm receiver continues to run.

• void set_ip (char *ip)
//	Used to set the IP of the ECR’s virtual network interface.

• void set_tx_queue_len (int queue_len)
//	Allows you to set the tx buffer length for the virtual network interface This could be useful in trading off between
//	dropped packets and latency with a UDP connection.
• int get_tx_queued_bytes ()
//	Returns the number of bytes currently queued for transmission.
• void dec_tx_queued_bytes (int n)
//	Decrements the count of bytes currently queued for transmission This function is only used as a work around since
//	tun interfaces don’t allow you to read the number of queued bytes.
• void inc_tx_queued_bytes (int n)
//	Increments the count of bytes currently queued for transmission This function is only used as a work around since tun
//	interfaces don’t allow you to read the number of queued bytes.

/*
             _     _______                                                         _                   
            | |   |__   __|                                                       | |                  
  ___   ___ | |_     | |__  __          _ __    __ _  _ __  __ _  _ __ ___    ___ | |_  ___  _ __  ___ 
 / __| / _ \| __|    | |\ \/ /         | '_ \  / _` || '__|/ _` || '_ ` _ \  / _ \| __|/ _ \| '__|/ __|
 \__ \|  __/| |_     | | >  <          | |_) || (_| || |  | (_| || | | | | ||  __/| |_|  __/| |   \__ \
 |___/ \___| \__|    |_|/_/\_\         | .__/  \__,_||_|   \__,_||_| |_| |_| \___| \__|\___||_|   |___/
                                       | |                                                             
                                       |_|  
*/
• void set_tx_freq (double _tx_freq)
//	Set the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_freq.
• void set_tx_freq (double _tx_freq, double _dsp_freq)
• void set_tx_rate (double _tx_rate)
//	Set the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_rate.
• void set_tx_gain_soft (double _tx_gain_soft)
//	Set the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_gain_soft.
• void set_tx_gain_uhd (double _tx_gain_uhd)
//	Set the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_gain_uhd.
• void set_tx_antenna (char *_tx_antenna)
• void set_tx_modulation (int mod_scheme)
//	Set the value of mod_scheme in ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
• void set_tx_crc (int crc_scheme)
//	Set the value of check in ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
• void set_tx_fec0 (int fec_scheme)
//	Set the value of fec0 in ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
• void set_tx_fec1 (int fec_scheme)
//	Set the value of fec1 in ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
• void set_tx_subcarriers (unsigned int subcarriers)
//	Set the value of ExtensibleCognitiveRadio::tx_parameter_s::numSubcarriers.
• void set_tx_subcarrier_alloc (char *_subcarrierAlloc)
//	Set ExtensibleCognitiveRadio::tx_parameter_s::subcarrierAlloc.
• void set_tx_cp_len (unsigned int cp_len)
	//Set the value of ExtensibleCognitiveRadio::tx_parameter_s::cp_len.
• void set_tx_taper_len (unsigned int taper_len)
	//Set the value of ExtensibleCognitiveRadio::tx_parameter_s::taper_len.
• void set_tx_control_info (unsigned char *_control_info)
//	Set the control information used for future transmit frames.
• void set_tx_payload_sym_len (unsigned int len)
//	Set the number of symbols transmitted in each frame payload. For now since the ECR does not have any segmentation/
//  concatenation capabilities, the actual payload will be an integer number of IP packets, so this value really provides
//	a lower bound for the payload length in symbols.

/*
               _     _______                                                         _                   
              | |   |__   __|                                                       | |                  
   __ _   ___ | |_     | |__  __          _ __    __ _  _ __  __ _  _ __ ___    ___ | |_  ___  _ __  ___ 
  / _` | / _ \| __|    | |\ \/ /         | '_ \  / _` || '__|/ _` || '_ ` _ \  / _ \| __|/ _ \| '__|/ __|
 | (_| ||  __/| |_     | | >  <          | |_) || (_| || |  | (_| || | | | | ||  __/| |_|  __/| |   \__ \
  \__, | \___| \__|    |_|/_/\_\         | .__/  \__,_||_|   \__,_||_| |_| |_| \___| \__|\___||_|   |___/
   __/ |                                 | |                                                             
  |___/
  */
• double get_tx_freq ()
//	Return the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_freq.
• double get_tx_lo_freq ()
//	Return the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_freq.
• int get_tx_state ()
//	Return the value of ExtensibleCognitiveRadio::tx_state.
• double get_tx_dsp_freq ()
//	Return the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_freq.
• double get_tx_rate ()
//	Return the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_rate.
• double get_tx_gain_soft ()
//	Return the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_gain_soft.
• double get_tx_gain_uhd ()
//	Return the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_gain_uhd.
• char * get_tx_antenna ()
• int get_tx_modulation ()
//	Return the value of mod_scheme in ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
• int get_tx_crc ()
//	Return the value of check in ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
• int get_tx_fec0 ()
//	Return the value of fec0 in ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
• int get_tx_fec1 ()
//	Return the value of fec1 in ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
• unsigned int get_tx_subcarriers ()
//	Return the value of ExtensibleCognitiveRadio::tx_parameter_s::numSubcarriers.
• void get_tx_subcarrier_alloc (char *subcarrierAlloc)
//	Get current ExtensibleCognitiveRadio::tx_parameter_s::subcarrierAlloc.
• unsigned int get_tx_cp_len ()
//	Return the value of ExtensibleCognitiveRadio::tx_parameter_s::cp_len.
• unsigned int get_tx_taper_len ()
• void get_tx_control_info (unsigned char *_control_info)
• double get_tx_data_rate ()



• void start_tx ()
• void start_tx_burst (unsigned int _num_tx_frames, float _max_tx_time_ms)
• void stop_tx ()
• void reset_tx ()
• void transmit_control_frame (unsigned char *_payload, unsigned int _payload_len)
//	Transmit a control frame.


/*
             _      _____                                                             _                   
            | |    |  __ \                                                           | |                  
  ___   ___ | |_   | |__) |__  __          _ __    __ _  _ __  __ _  _ __ ___    ___ | |_  ___  _ __  ___ 
 / __| / _ \| __|  |  _  / \ \/ /         | '_ \  / _` || '__|/ _` || '_ ` _ \  / _ \| __|/ _ \| '__|/ __|
 \__ \|  __/| |_   | | \ \  >  <          | |_) || (_| || |  | (_| || | | | | ||  __/| |_|  __/| |   \__ \
 |___/ \___| \__|  |_|  \_\/_/\_\         | .__/  \__,_||_|   \__,_||_| |_| |_| \___| \__|\___||_|   |___/
                                          | |                                                             
                                          |_|
*/
• void set_rx_freq (double _rx_freq)
//	Set the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_freq.
• void set_rx_freq (double _rx_freq, double _dsp_freq)
• void set_rx_rate (double _rx_rate)
//	Set the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_rate.
• void set_rx_gain_uhd (double _rx_gain_uhd)
//	Set the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_gain_uhd.
• void set_rx_antenna (char *_rx_antenna)
• void set_rx_subcarriers (unsigned int subcarriers)
//	Set the value of ExtensibleCognitiveRadio::rx_parameter_s::numSubcarriers.
• void set_rx_subcarrier_alloc (char *_subcarrierAlloc)
//	Set ExtensibleCognitiveRadio::rx_parameter_s::subcarrierAlloc.
• void set_rx_cp_len (unsigned int cp_len)
//	Set the value of ExtensibleCognitiveRadio::rx_parameter_s::cp_len.
• void set_rx_taper_len (unsigned int taper_len)
//	Set the value of ExtensibleCognitiveRadio::rx_parameter_s::taper_len.

/*
   _____        _      _____                                                             _                   
  / ____|      | |    |  __ \                                                           | |                  
 | |  __   ___ | |_   | |__) |__  __          _ __    __ _  _ __  __ _  _ __ ___    ___ | |_  ___  _ __  ___ 
 | | |_ | / _ \| __|  |  _  / \ \/ /         | '_ \  / _` || '__|/ _` || '_ ` _ \  / _ \| __|/ _ \| '__|/ __|
 | |__| ||  __/| |_   | | \ \  >  <          | |_) || (_| || |  | (_| || | | | | ||  __/| |_|  __/| |   \__ \
  \_____| \___| \__|  |_|  \_\/_/\_\         | .__/  \__,_||_|   \__,_||_| |_| |_| \___| \__|\___||_|   |___/
                                             | |                                                             
                                             |_|
*/
• int get_rx_state ()
//	Return the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_state.
• int get_rx_worker_state ()
//	Return the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_worker_state.
• double get_rx_freq ()
//	Return the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_freq.
• double get_rx_lo_freq ()
//	Return the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_freq.
• double get_rx_dsp_freq ()
//	Return the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_freq.
• double get_rx_rate ()
//	Return the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_rate.
• double get_rx_gain_uhd ()
//	Return the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_gain_uhd.
• char * get_rx_antenna ()
• unsigned int get_rx_subcarriers ()
//	Return the value of ExtensibleCognitiveRadio::rx_parameter_s::numSubcarriers.
• void get_rx_subcarrier_alloc (char *subcarrierAlloc)
//	Get current ExtensibleCognitiveRadio::rx_parameter_s::subcarrierAlloc.
• unsigned int get_rx_cp_len ()
//	Return the value of ExtensibleCognitiveRadio::rx_parameter_s::cp_len.
• unsigned int get_rx_taper_len ()
//	Return the value of ExtensibleCognitiveRadio::rx_parameter_s::taper_len.
• void get_rx_control_info (unsigned char *_control_info)


• void reset_rx ()
• void start_rx ()
• void stop_rx ()
• void start_liquid_rx ()
• void stop_liquid_rx ()


• void set_rx_stat_tracking (bool state, float sec)
• float get_rx_stat_tracking_period ()
• struct rx_statistics get_rx_stats ()
• void reset_rx_stats ()
• void print_metrics (ExtensibleCognitiveRadio CR)


/*
  _       ____    _____   _____ 
 | |     / __ \  / ____| / ____|
 | |    | |  | || |  __ | (___  
 | |    | |  | || | |_ | \___ \ 
 | |____| |__| || |__| | ____) |
 |______|\____/  \_____||_____/ 
                                */
• void log_rx_metrics ()
• void log_tx_parameters ()
• void reset_log_files ()


/*
███████╗████████╗███████╗██████╗             ██╗  ██╗
██╔════╝╚══██╔══╝██╔════╝██╔══██╗            ██║  ██║
███████╗   ██║   █████╗  ██████╔╝            ███████║
╚════██║   ██║   ██╔══╝  ██╔═══╝             ╚════██║
███████║   ██║   ███████╗██║                      ██║
╚══════╝   ╚═╝   ╚══════╝╚═╝                      ╚═╝
Diving Deep Into The publiC Attributes Which We could be used*/

struct metric_s CE_metrics
//The instance of ExtensibleCognitiveRadio::metric_s made accessible to the Cognitive_Engine.
std::complex< float > * ce_usrp_rx_buffer
//USRP samples will be written to this buffer if the ce_sensing_flag is set.

//Other Attributes are accessible via configuration files
