#ifndef _CE_RANDOM_BEHAVIOUR_PU_
#define _CE_RANDOM_BEHAVIOUR_PU_

#include <stdio.h>
#include <sys/time.h>
#include "extensible_cognitive_radio.hpp"
#include "cognitive_engine.hpp"
#include "timer.h"

#define OFF 0

class CE_Random_Behaviour_PU : public CognitiveEngine {
public:
  CE_Random_Behaviour_PU(int argc, char**argv, ExtensibleCognitiveRadio *_ECR);
  ~CE_Random_Behaviour_PU();
  virtual void execute();

private:

	//int STATES= 3;
  static constexpr float CHANNEL1 =833e6;
  static constexpr float CHANNEL2 =835e6;
  static constexpr float CHANNEL3 =838e6;
//  timer sleeping_time;

 
//  int tx_state;
//  float tx_freq;


  struct timeval tv;
  time_t switch_time_s;
  int period_s;
  int first_execution;

};

#endif
