
/*
 * Author: astro

The uniform random variable generator simply generates a number evenly distributed
in [CHANNEL1, CHANNEL2, CHANNEL3]
using the standard rand() method for generating random integers and then divides by the maximum number 
that can be generated which is 3.

*/

#include "CE_Random_Behaviour_PU.hpp"


// constructor
CE_Random_Behaviour_PU::CE_Random_Behaviour_PU(int argc, char **argv, ExtensibleCognitiveRadio *_ECR) {
  ECR = _ECR;
  first_execution = 1;
  period_s = 2;
//  sleeping_time = timer_create();
//  timer_tic(sleeping_time);
}

CE_Random_Behaviour_PU::~CE_Random_Behaviour_PU() {}


// execute function
void CE_Random_Behaviour_PU::execute() {
  
  gettimeofday(&tv, NULL);

  if (first_execution) {
    switch_time_s = tv.tv_sec + period_s;
    ECR->set_ce_timeout_ms(0);
    first_execution = 0;
  }

//timer_tic(sleeping_time);

//while( timer_toc(sleeping_time) <= 2 ){
  if (tv.tv_sec >= switch_time_s) {
    // update switch time
    switch_time_s += period_s;

    float tx_freq = ECR->get_tx_freq();

    float channels[] = { CHANNEL1 , CHANNEL2 , CHANNEL3 };
    int randomIndex = rand() % 3;
    int randomValue = channels[randomIndex];


    // switch tx frequency
    if (tx_freq == CHANNEL3|| CHANNEL2 || CHANNEL1 ) ECR->set_tx_freq(randomValue);
  
    tx_freq = ECR->get_tx_freq();
    if(tx_freq == CHANNEL1) printf("STATE: CHANNEL 1\n");
    else if(tx_freq == CHANNEL2) printf("STATE: CHANNEL 2\n");
    else printf("STATE: CHANNEL 3\n");

    printf("Transmit frequency: %f\n", tx_freq);
  }
//}
/*
  tx_state = ECR->get_tx_state();
  while (tx_state == OFF){
    printf("Sleeping\n");
  }
*/
}
