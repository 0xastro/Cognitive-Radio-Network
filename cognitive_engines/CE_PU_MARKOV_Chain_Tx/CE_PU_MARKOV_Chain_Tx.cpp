/*
 * Author: astro
 * Purpose: Design an engine for primary user (member of the cognitive radio network)
            which is based on markov chain model
 * Language:  C/C++
 * Dependencies: CRTS, UHD DRIVERS, LIQUID DSP
 *               This version is stable only for USRP 1 AND USRP 2 
                  AND it doesnt work on USRPX SERIES.
 * Development:  CORNET TESTBED
 * Program subroutine:

no need to design the transition matrix, just demo.
with 3 STATES theres is 8 possible transitions

                +-------------------------++--------++--------++---------+
                |        MARKOV Chain Probability                        |
                |           states transition                            |
                +-------------------------++--------++--------++---------+
                | CHANNEL 1.   |   CHANNEL 2       |  CHANNEL 3          |
----------------+--------------------------------------------------------+
|  CHANNEL 1  |  P(CH_1|CH_1)= 0.1| P(CH_1|CH_2)=0.3  | P(CH_1|CH_3)=0.6 |
+-------------+-------+--------+--------++--------++--------++-----------+
|  CHANNEL 2  |  P(CH_2|CH_1)= 0.1| P(CH_2|CH_2)=0.5  | P(CH_2|CH_3)=0.4 |
+-------------+-------+--------+--------++--------++--------++--------+--+
|  CHANNEL 3  |  P(CH_3|CH_1)= 0.1| P(CH_3|CH_2)=0.2  | P(CH_3|CH_3)=0.7 |
+-------------+-------+--------+--------++--------++--------++-----------+


*/


#include "CE_PU_MARKOV_Chain_Tx.hpp"

// constructor
CE_PU_MARKOV_Chain_Tx::CE_PU_MARKOV_Chain_Tx(int argc, char **argv, ExtensibleCognitiveRadio *_ECR) {
  ECR = _ECR;
  first_execution = 1;
  period_s = 5;
  rx_flag=1;
}

CE_PU_MARKOV_Chain_Tx::~CE_PU_MARKOV_Chain_Tx() {}


// execute function
void CE_PU_MARKOV_Chain_Tx::execute() {
  
  if (rx_flag){
    ECR->stop_rx();
    rx_flag = 0;
  } 

  gettimeofday(&tv, NULL);

  if (first_execution) {
    switch_time_s = tv.tv_sec + period_s;
    ECR->set_ce_timeout_ms(100.0);
    first_execution = 0;
  }

  if (tv.tv_sec >= switch_time_s) {
    // update switch time
    switch_time_s += period_s;

    hopping++;
    printf("+--------------------------------------------------------------------------------\n \t \t HOPPING \t \t # %d \n+--------------------------------------------------------------------------------\n",hopping);

    if (ECR->get_tx_freq() == CHANNEL_1) printf("Current State: CHANNEL 1 ::::: ");
    else if (ECR->get_tx_freq()== CHANNEL_2) printf("Current State: CHANNEL 2 ::::: ");
    else printf("Current State: CHANNEL 3 ::::: ");
   // printf("process ................. \n");
    RANDOM_OUTOCME(ECR);
    PU_TX_Behaviour(ECR);
    //PU_RX_Behaviour(ECR);

    if (ECR->get_tx_freq() == CHANNEL_1) printf("Next State: CHANNEL 1\n");
    else if (ECR->get_tx_freq()== CHANNEL_2) printf("Next State: CHANNEL 2\n");
    else printf("Next State: CHANNEL 3\n");

/*
    printf("Transmit frequency: %f\n", ECR->get_tx_freq());
    printf("Receiver frequency: %f\n\n", ECR->get_rx_freq());
*/

  }
}

void CE_PU_MARKOV_Chain_Tx::RANDOM_OUTOCME(ExtensibleCognitiveRadio *ECR) {
    static constexpr float Sample_Space[10]={0,1,2,3,4,5,6,7,8,9};
    outcome= rand() % 10;
    state_probability = Sample_Space[outcome]; //Sample_Space[0],...Sample_Space[9]
}




void CE_PU_MARKOV_Chain_Tx::PU_TX_Behaviour(ExtensibleCognitiveRadio *ECR) {
  tx_freq = ECR->get_tx_freq();

//checking for second state CHANNEL_1
if (tx_freq == CHANNEL_1){
    if (state_probability == 0)
        ECR->set_tx_freq(CHANNEL_1);
    else if(state_probability>=1 || state_probability<4)
        ECR->set_tx_freq(CHANNEL_2);
    else 
        ECR->set_tx_freq(CHANNEL_3);
    }

//checking for second state CHANNEL_2
else if (tx_freq == CHANNEL_2){
    if (state_probability == 0 )    
        ECR->set_tx_freq(CHANNEL_1);
    else if(state_probability>=1 || state_probability<6)
        ECR->set_tx_freq(CHANNEL_2);
    else 
        ECR->set_tx_freq(CHANNEL_3);
    }
//checking for Third state CHANNEL_3
else{
    if (state_probability == 0)
        ECR->set_tx_freq(CHANNEL_1);
    else if(state_probability>=1 || state_probability<4)
        ECR->set_tx_freq(CHANNEL_2);
    else 
        ECR->set_tx_freq(CHANNEL_3);
    }
}

