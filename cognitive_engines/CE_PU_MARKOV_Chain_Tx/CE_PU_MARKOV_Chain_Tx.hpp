#ifndef _CE_PU_MARKOV_CHAIN_TX_
#define _CE_PU_MARKOV_CHAIN_TX_

#include <stdio.h>
#include <sys/time.h>
#include "extensible_cognitive_radio.hpp"
#include "cognitive_engine.hpp"
#include "timer.h"

//#define STATES 3
#define CH_1 833e6
#define CH_2 836e6
#define CH_3 838e6



class CE_PU_MARKOV_Chain_Tx : public CognitiveEngine {
public:
  CE_PU_MARKOV_Chain_Tx(int argc, char**argv, ExtensibleCognitiveRadio *_ECR);
  ~CE_PU_MARKOV_Chain_Tx();
  virtual void execute();

private:

//STATE_SPACE={CHANNEL_1, CHANNEL_2, CHANNEL_3}
static constexpr float CHANNEL_1 =CH_1;
static constexpr float CHANNEL_2 =CH_2;
static constexpr float CHANNEL_3 =CH_3;

float tx_freq,rx_freq;
float state_probability;
short outcome;
short rx_flag;
int hopping =0;

//OS time related parameters for synch
struct timeval tv;
time_t switch_time_s;
int period_s;
int first_execution;

//memeber fuctions
void RANDOM_OUTOCME(ExtensibleCognitiveRadio *ECR); //produce random variable R.V.
void PU_TX_Behaviour(ExtensibleCognitiveRadio *ECR);//change the TX states based on R.V.
//void PU_RX_Behaviour(ExtensibleCognitiveRadio *ECR);//change the RX states based on R.V.



};

#endif
 
