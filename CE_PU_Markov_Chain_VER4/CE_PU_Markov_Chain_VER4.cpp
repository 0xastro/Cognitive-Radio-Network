/*
 * Author: astro
 * Purpose: Design an engine for primary user (member of the cognitive radio network)
            which is based on markov chain model
 * implemented as part of undergraduatation thesis at CIC.
 * email: m.rahm7n@gmail.com.  abdelrahman_sayed@cic-cairo.com 
 * Blog: https://astro0blog.wordpress.com
 * Language:  C/C++
 * Program subroutine:

//no need to design the transition matrix, just demo.
/*with 3 STATES theres is 8 possible transitions

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


#include "CE_PU_Markov_Chain_VER4.hpp"

// constructor
CE_PU_Markov_Chain_VER4::CE_PU_Markov_Chain_VER4(int argc, char **argv, ExtensibleCognitiveRadio *_ECR) {
  ECR = _ECR;
  first_execution = 1;
  period_s = 5;
}

CE_PU_Markov_Chain_VER4::~CE_PU_Markov_Chain_VER4() {}


// execute function
void CE_PU_Markov_Chain_VER4::execute() {
  
  gettimeofday(&tv, NULL);

  if (first_execution) {
    switch_time_s = tv.tv_sec + period_s;
    ECR->set_ce_timeout_ms(100.0);
    first_execution = 0;
  }

  if (tv.tv_sec >= switch_time_s) {
    // update switch time
    switch_time_s += period_s;

/*    static constexpr float Sample_Space[10]={0,1,2,3,4,5,6,7,8,9};
    outcome= rand() % 10;
    float state_probability = Sample_Space[outcome]; //Sample_Space[0],...Sample_Space[9]
*/ 
    if(!Channels_Defined){
      ECR->stop_tx();
      ECR->stop_rx();
   //printf("Define your channels value:\n as follow e.g if: Channel1: 600Mhz Channel2: 700Mhz Channel3: 800Mhz \n enter 600e6 then 700e6 then 800e6\n  ");
   //scanf("%f %f %f",&CHANNEL_1,&CHANNEL_2,&CHANNEL_3);
      printf("CHANNEL 1: > ");
      scanf("%f",&CHANNEL_1);
      printf("CHANNEL 2: >");
      scanf("%f",&CHANNEL_2);
      printf("CHANNEL 3: >");
      scanf("%f",&CHANNEL_3);
     Channels_Defined=1;
     ECR->start_rx();
     ECR->start_tx();
    }

    /*TODO
    1. ADD command line arguments Feature to define channels for the ease of use
    2. improve the subroutine:programm is really slow during of my dirty code subroutine "will be improved later"
    3. define the setling time for USRP for accurate freq tuning
    4. I will not do what I mentioned in 1, 2 and 3 cuz I have no time :} :}
    */

    hopping++;
    printf("\t \t \t===================\t \t \t \n");
    printf("\t \t \t=++HOPPING: # %d++=\t \t \t \n",hopping);
    printf("\t \t \t===================\t \t \t \n");

    if (ECR->get_tx_freq() == CHANNEL_1) 
      printf("\t \t \t*Current State: CHANNEL 1:(%-.2e) :::::",CHANNEL_1);
    else if (ECR->get_tx_freq()== CHANNEL_2) 
      printf("\t \t \t*Current State: CHANNEL 2:(%-.2e) :::::",CHANNEL_2);
    else 
      printf("\t \t \t*Current State: CHANNEL 3:(%-.2e) :::::",CHANNEL_3);
    RANDOM_OUTOCME(ECR);
    PU_TX_Behaviour(ECR);
    PU_RX_Behaviour(ECR);

    if (ECR->get_tx_freq() == CHANNEL_1)
      printf("\t \t \t*Next State: CHANNEL 1:(%-.2e) :::::\n",CHANNEL_1);
    else if (ECR->get_tx_freq()== CHANNEL_2) 
      printf("\t \t \t*Next State: CHANNEL 2:(%-.2e) :::::\n",CHANNEL_2);
    else 
      printf("\t \t \t*Next State: CHANNEL 3:(%-.2e) :::::\n",CHANNEL_3);

/*
    printf("Transmit frequency: %f\n", ECR->get_tx_freq());
    printf("Receiver frequency: %f\n\n", ECR->get_rx_freq());
*/

  }
}



void CE_PU_Markov_Chain_VER4::RANDOM_OUTOCME(ExtensibleCognitiveRadio *ECR) {
    static constexpr float Sample_Space[10]={0,1,2,3,4,5,6,7,8,9};
    outcome= rand() % 10;
    state_probability = Sample_Space[outcome]; //Sample_Space[0],...Sample_Space[9]
}




void CE_PU_Markov_Chain_VER4::PU_TX_Behaviour(ExtensibleCognitiveRadio *ECR) {
  tx_freq = ECR->get_tx_freq();

                    //checking for first state CHANNEL_1
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
                  else
                  {
                    if (state_probability == 0)
                      ECR->set_tx_freq(CHANNEL_1);
                    else if(state_probability>=1 || state_probability<4)
                      ECR->set_tx_freq(CHANNEL_2);
                    else 
                      ECR->set_tx_freq(CHANNEL_3);
                  }
}




void CE_PU_Markov_Chain_VER4::PU_RX_Behaviour(ExtensibleCognitiveRadio *ECR) {
  rx_freq = ECR->get_rx_freq();

                    //checking for first state CHANNEL_1
                    if (rx_freq == CHANNEL_1){
                    if (state_probability == 0)
                      ECR->set_rx_freq(CHANNEL_1);
                    else if(state_probability>=1 || state_probability<4)
                      ECR->set_rx_freq(CHANNEL_2);
                    else 
                      ECR->set_rx_freq(CHANNEL_3);
                  }
                    //checking for second state CHANNEL_2
                  else if (rx_freq == CHANNEL_2){
                    if (state_probability == 0 )    
                      ECR->set_rx_freq(CHANNEL_1);
                    else if(state_probability>=1 || state_probability<6)
                      ECR->set_rx_freq(CHANNEL_2);
                    else 
                      ECR->set_rx_freq(CHANNEL_3);
                  }
                    //checking for Third state CHANNEL_3
                  else
                  {
                    if (state_probability == 0)
                      ECR->set_rx_freq(CHANNEL_1);
                    else if(state_probability>=1 || state_probability<4)
                      ECR->set_rx_freq(CHANNEL_2);
                    else 
                      ECR->set_rx_freq(CHANNEL_3);
                  }
}
