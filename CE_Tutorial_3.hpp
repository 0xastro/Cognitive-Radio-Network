#ifndef _CE_Tutorial_3_
#define _CE_Tutorial_3_

#include "cognitive_engine.hpp"

class CE_Tutorial_3 : public CognitiveEngine {

private:
  // internal members used by this CE
  int debugLevel;
  //The members which I added later to implement a CE
	const float print_stats_period_s = 1.0;
	timer print_stats_timer;
	const float tx_gain_period_s = 1.0;
	const float tx_gain_increment = 1.0;
	timer tx_gain_timer;
	int frame_counter;
	int frame_errs;
	float sum_evm; //sum the error vector magnitude
	float sum_rssi; //sum the recieved signal strenght indicator

public:
  CE_Tutorial3(int argc, char **argv, ExtensibleCognitiveRadio *_ECR);
  ~CE_Tutorial3();
  virtual void execute();
};

#endif
