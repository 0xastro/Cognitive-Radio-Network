#ifndef _CE_TX_CHANNEL_X_
#define _CE_TX_CHANNEL_X_

#include <stdio.h>
#include <sys/time.h>
#include "extensible_cognitive_radio.hpp"
#include "cognitive_engine.hpp"
#include "timer.h"

class CE_TX_CHANNEL_X : public CognitiveEngine {
public:
  CE_TX_CHANNEL_X(int argc, char**argv, ExtensibleCognitiveRadio *_ECR);
  ~CE_TX_CHANNEL_X();
  virtual void execute();

private:

float DESIRED_CHANNEL=0;
int Channel_Specified = 0;

};

#endif
 
