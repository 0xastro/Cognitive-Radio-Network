/*
  Author: Astro
  
  This Engine is based on a simple cognitive cycle [Observe, analyse, learn and predict]
  to make an optimum Decision about the RF environment
  Features: Sensing 3 channels simultenously 
  To be added: +Making a decision based on Neural Net outputs
		+Training The Neural Net under many conditions like adding an interferer to the scenario
		+The generated Data_Set
		+Neural Network Training Model
*/



#include "CE_Predictive_Node.hpp"

// constructor
CE_Predictive_Node::CE_Predictive_Node(
    int argc, char **argv, ExtensibleCognitiveRadio *_ECR) {

  ECR = _ECR;

  // initialize counter to 0
  fft_counter = 0;
  config = 0;

  // create timer to enable/disable sensing
  sensing_timer = timer_create();
  timer_tic(sensing_timer);

  struct timeval tv;
  gettimeofday(&tv, NULL);
  sense_time_s = tv.tv_sec;
  sense_time_us = tv.tv_usec;

  // initialize buffers to 0
  memset(buffer, 0, fft_length * sizeof(float _Complex));
  memset(buffer_F, 0, fft_length * sizeof(float _Complex));
  memset(fft_avg, 0, fft_length * sizeof(float));

  // create fft plan
  fft = fft_create_plan(fft_length,
                        reinterpret_cast<liquid_float_complex *>(buffer),
                        reinterpret_cast<liquid_float_complex *>(buffer_F),
                        LIQUID_FFT_FORWARD, 0);
}

// destructor
CE_Predictive_Node::~CE_Predictive_Node() {}



// execute function
void CE_Predictive_Node::execute() {

/*
  _______________________________________________________
  ENGINE CONFIURATION
  Neural Network's weights Initialization
  B.W. and fc configuration
  _______________________________________________________
  */



  if(config == 0){
    ECR->stop_tx(); 
    ECR->set_rx_freq(Desired_fc);
    ECR->set_rx_rate(Desired_BW);

    /*
  _______________________________________________________
  Memory Construction based on adjusted weights/synapsis
  Error = 0.000100 after  63.145737 Milion Epoch   
  _______________________________________________________
  */

  WeightIH[0][1]   =        -0.188208;
  WeightIH[1][1]   =        -0.106634;
  WeightIH[2][1]   =        0.005650;
  WeightIH[3][1]   =        -0.057578;
  WeightIH[4][1]   =        0.092680;
  WeightIH[0][2]   =        -0.170684;
  WeightIH[1][2]   =        -0.415470;
  WeightIH[2][2]   =        0.741944;
  WeightIH[3][2]   =        0.621154;
  WeightIH[4][2]   =        0.809336;
  WeightIH[0][3]   =        -0.024726;
  WeightIH[1][3]   =        0.309261;
  WeightIH[2][3]   =        0.006133;
  WeightIH[3][3]   =        -0.048268;
  WeightIH[4][3]   =        -0.010821;
  WeightIH[0][4]   =        0.001448;
  WeightIH[1][4]   =        0.159974;
  WeightIH[2][4]   =        -0.620100;
  WeightIH[3][4]   =        -0.249186;
  WeightIH[4][4]   =        -0.546496;
  WeightIH[0][5]   =        0.015983;
  WeightIH[1][5]   =        0.212781;
  WeightIH[2][5]   =        0.669892;
  WeightIH[3][5]   =        0.734475;
  WeightIH[4][5]   =        0.609384;
  WeightHO[0][1]   =        -7.033320;
  WeightHO[1][1]   =        10.857465;
  WeightHO[2][1]   =        -6.848443;
  WeightHO[3][1]   =        17.053079;
  WeightHO[4][1]   =        0.087664;
  WeightHO[5][1]   =        -6.552455;
  WeightHO[0][2]   =        2.726400;
  WeightHO[1][2]   =        -18.452471;
  WeightHO[2][2]   =        2.053071;
  WeightHO[3][2]   =        -13.375309;
  WeightHO[4][2]   =        -0.269499;
  WeightHO[5][2]   =        2.655529;
  WeightHO[0][3]   =        -2.590206;
  WeightHO[1][3]   =        15.609466;
  WeightHO[2][3]   =        -2.929559;
  WeightHO[3][3]   =        -15.703407;
  WeightHO[4][3]   =        0.407028;
  WeightHO[5][3]   =        -2.552555;

    config = 1;
  }



  struct timeval tv;
  gettimeofday(&tv, NULL);

  // turn on sensing after once the required time has past
  if ((tv.tv_sec > sense_time_s) || ((tv.tv_sec == sense_time_s) && (tv.tv_usec >= sense_time_us))) {
   // printf("Turning on sensing\n");
    ECR->stop_tx();
    ECR->set_ce_sensing(1);
//OR
    //ECR->stop_rx(); /*stopping rx enables forwarding samples to CE*/

    // calculate next sense time
    sense_time_s = tv.tv_sec + (long int)floorf(sensing_delay_ms / 1e3);
    sense_time_us = tv.tv_usec + (long int)floorf(sensing_delay_ms * 1e3);
  }



  // handle samples
  if (ECR->CE_metrics.CE_event == ExtensibleCognitiveRadio::USRP_RX_SAMPS) {

    fft_counter++;
    memcpy(buffer, ECR->ce_usrp_rx_buffer, ECR->ce_usrp_rx_buffer_length * sizeof(float _Complex));
    fft_execute(fft);

    for (int i = 0; i < fft_length; i++) {
      fft_avg[i] += cabsf(buffer_F[i]) / (float)fft_averaging;
    }

    // reset once averaging has finished
    if (fft_counter == fft_averaging) {
      // stop forwarding usrp samples to CE
      ECR->set_ce_sensing(0);



float M1,M2,M3,CH1,CH2,CH3,NOISE_FLOOR,NF;
   M1 = M2 = M3 = NF= 0.0f; //initialize frequncy bins 
   CH1 = CH2 = CH3 = NOISE_FLOOR = 0.0f; //initialize channels Power

/*
Summing frequency bins amplitude around Channel 1: [833e6] and store it in M1
Channel 2: [835e6] -> M2
Channel 3: [838e6] -> M3
NoiseFloor         -> NF
*/
  for (int i = 0; i < 16; i++) {
    M1 += cabsf(fft_avg[i]);
  }

  for (int i = 496; i < 511; i++) {
    M1 += cabsf(fft_avg[i]);
  }

    for (int i = 55; i < 85; i++) {
    M2 += cabsf(fft_avg[i]);
  }

    for (int i = 189; i < 222; i++) {
    M3 += cabsf(fft_avg[i]);
  }

    for (int i = 300; i < 310; i++) {
    NF += cabsf(fft_avg[i]);
  }

  //Getting Channels power
  CH1=(M1*M1);
  CH2=(M2*M2);
  CH3=(M3*M3);
  NOISE_FLOOR=(NF*NF);

  //initialize Features Buffer to be used in prediction process
  double  Features_Buffer[INPUTS+1]={0,NOISE_FLOOR,CH1,CH2,CH3};

  printf("--------------------------------------------------------------\n");
  printf("-            		FEATURES BUFFER 	               -\n");
  printf("--------------------------------------------------------------\n");

//Display the Measured Features
printf("NOISE FLOOR   %.2e\nCH1           %.2e\nCH2           %.2e\nCH3           %.2e\n ",NOISE_FLOOR,CH1,CH2,CH3);

            //*******************************************************/
            // propagate the inputs into our Hidden Layer and Then
            // Process them by activation using SIGMOID function
            //*******************************************************/

  for( j = 1 ; j <= NumHidden ; j++ ) {    // compute Sigmoid_HA unit activations 
      SumH[j] = WeightIH[0][j] ;
      for( i = 1 ; i <= NumInput ; i++ ) {
          SumH[j] += Features_Buffer[i] * WeightIH[i][j] ;
      }
      Sigmoid_HA[j] = 1.0/(1.0 + exp(-SumH[j])); //Hidden Layer Activation 
  }

            //*******************************************************/
            // propagate the Activated output into our output Layer
            // and Then
            // Process them by activation using SIGMOID function
            // finally we get the network output
            //*******************************************************/

  for( k = 1 ; k <= NumOutput ; k++ ) {   
      SumO[k] = WeightHO[0][k] ;
      for( j = 1 ; j <= NumHidden ; j++ ) {
          SumO[k] += Sigmoid_HA[j] * WeightHO[j][k] ;
      }
      Output[k] = 1.0/(1.0 + exp(-SumO[k])) ; //Output Layer Activation    
  }



  printf("\n \n \n --------------------------------------------------------------\n");
  printf("-            		 REAL TIME PREDICTION                  -\n");
  printf("--------------------------------------------------------------\n");



     if (Output[1] >= 0.8 ){
      printf("Channel_State[1]: OCCUPIED \nChannel_State[2]: FREE \nChannel_State[3]: FREE \n \n \n");
	ECR->set_tx_freq(CHANNEL2);
    }

    else if (Output[2] >= 0.8 ) {
      printf("Channel_State[1]: FREE \nChannel_State[2]: OCCUPIED \nChannel_State[3]: FREE \n \n \n");
	ECR->set_tx_freq(CHANNEL1);
    }

    else if (Output[3] >= 0.8 ){ 
      printf("Channel_State[1]: FREE \nChannel_State[2]: FREE \nChannel_State[3]: OCCUPIED \n \n \n");
      ECR->set_tx_freq(CHANNEL2);
    }

    else
	  printf("ALL BUSY, SENSE AND OBSERVE AGAIN \n");


/* Will be configured later to reduce the probability of false
alarms and missed detection 
*/
    //*******************************************************/
    // Taking Action Based the network output
    //*******************************************************/
/*
    float tx_freq =  ECR->get_tx_freq();

    timer_tic(sensing_timer);
    ECR->start_tx();

    if (tx_freq == CHANNEL1) printf("transmitting on CHANNEL1\n");
    else if (tx_freq == CHANNEL2) printf("transmitting on CHANNEL2\n");
    else  printf("transmitting on CHANNEL3\n");
	  

    while(timer_toc(sensing_timer) >= 1){ //transmit for 1 sec
      ECR->stop_tx();
    }

*/
      // reset counter and average fft buffer
      memset(fft_avg, 0, fft_length * sizeof(float));
      fft_counter = 0;
    }
  }

}
