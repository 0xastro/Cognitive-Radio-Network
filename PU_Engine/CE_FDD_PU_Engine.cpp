#include "CE_FDD_PU_Engine.hpp"

// constructor
CE_FDD_PU_Engine::CE_FDD_PU_Engine(int argc, char **argv, ExtensibleCognitiveRadio *_ECR) {
  ECR = _ECR;
  first_execution = 1;
  period_s = 5;
}

// destructor
CE_FDD_PU_Engine::~FDD_PU_Engine() {}

// execute function
void CE_FDD_PU_Engine::execute() {
  
  gettimeofday(&tv, NULL);

  if (first_execution) {
    switch_time_s = tv.tv_sec + period_s;
    ECR->set_ce_timeout_ms(100.0);
    first_execution = 0;
  }

  if (tv.tv_sec >= switch_time_s) {
    // update switch time
    switch_time_s += period_s;

    float tx_freq = ECR->get_tx_freq();
    float rx_freq = ECR->get_rx_freq();

    // switch tx frequency
    if (tx_freq == channel_5)
      ECR->set_tx_freq(channel_4);
    else if (tx_freq == channel_4)
      ECR->set_tx_freq(channel_3);
    else if (tx_freq == channel_3)
      ECR->set_tx_freq(channel_5);
  
    else if (tx_freq == channel_2)
      ECR->set_tx_freq(channel_1);
    else if (tx_freq == channel_1)
      ECR->set_tx_freq(channel_0);
    else if (tx_freq == channel_0)
      ECR->set_tx_freq(channel_2);

    // switch rx frequency

    if (rx_freq == channel_5)
      ECR->set_rx_freq(channel_4);
    else if (rx_freq == channel_4)
      ECR->set_rx_freq(channel_3);
    else if (rx_freq == channel_3)
      ECR->set_rx_freq(channel_5);
  
    else if (rx_freq == channel_2)
      ECR->set_rx_freq(channel_1);
    else if (rx_freq == channel_1)
      ECR->set_rx_freq(channel_0);
    else if (rx_freq == channel_0)
      ECR->set_rx_freq(channel_2);

    printf("Transmit frequency: %f\n", ECR->get_tx_freq());
    printf("Receiver frequency: %f\n\n", ECR->get_rx_freq());
  }
}

// custom function definitions
