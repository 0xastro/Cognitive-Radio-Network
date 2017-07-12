#ifndef _CE_PREDICTIVE_NODE_
#define _CE_PREDICTIVE_NODE_

#include <liquid/liquid.h>
#include <sys/time.h>
#include "extensible_cognitive_radio.hpp"
#include "cognitive_engine.hpp"
#include "timer.h"
#include <complex.h>
#include <complex>

/*Libraries RELATED TO PREDICTIVE MODEL*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>

/*MACROS RELATED TO PREDICTIVE MODEL*/
#define INPUTS  4
#define HIDDEN_NEURONS 5
#define OUTPUT_NEURONS 3


class CE_Predictive_Node : public CognitiveEngine {

private:
  
  // sensing parameters
  static constexpr float sensing_delay_ms = 1e2;
  static constexpr int fft_length = 512;
  static constexpr int fft_averaging = 10;
  static constexpr float measurementDelay_ms = 200.0;

  // timer to start and stop sensing
  timer sensing_timer;
  long int sense_time_s;
  long int sense_time_us;

  //sensing Configuration
  int config;
  static constexpr float Desired_fc  = 833e6;
  static constexpr float Desired_BW = 13e6;

  // counter for fft averaging
  int fft_counter;

  // USRP2, FFT output, and average FFT  sample buffers
  float _Complex buffer[fft_length];
  float _Complex buffer_F[fft_length];
  float fft_avg[fft_length];

//Hint: to change the channels value, you have to reconfigure the frequency bins to get the 
  //right channels power
  static constexpr float CHANNEL1 = 833e6;
  static constexpr float CHANNEL2 = 835e6;
  static constexpr float CHANNEL3 = 838e6;


/*Neural Net parameters*/

  int NumInput   = INPUTS;
  int NumHidden  = HIDDEN_NEURONS;
  int NumOutput  = OUTPUT_NEURONS;

  double WeightIH[INPUTS+1][HIDDEN_NEURONS+1]; //weights between input and hidden layer
  double SumH[HIDDEN_NEURONS+1];
  double Sigmoid_HA[HIDDEN_NEURONS+1];  //activation of hidden layer neurons


  double WeightHO[HIDDEN_NEURONS+1][OUTPUT_NEURONS+1]; //weights between  hidden and output layer
  double SumO[OUTPUT_NEURONS+1];
  double Output[OUTPUT_NEURONS+1]; //activation of output layer neurons

  int i, j, k;


  // fft plan for spectrum sensing
  fftplan fft;

public:
  CE_Predictive_Node(int argc, char ** argv, ExtensibleCognitiveRadio *_ECR);
  ~CE_Predictive_Node();
  virtual void execute();
};

#endif
