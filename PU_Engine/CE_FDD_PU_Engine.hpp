#ifndef _CE_FDD_PU_ENGINE_
#define _CE_FDD_PU_ENGINE_

#include <stdio.h>
#include <sys/time.h> //system call 
#include "extensible_cognitive_radio.hpp"
#include "cognitive_engine.hpp"
#include "timer.h"

class CE_FDD_PU_Engine : public CognitiveEngine {
public:
  CE_FDD_PU_Engine(int argc, char**argv, ExtensibleCognitiveRadio *_ECR);
  ~CE_FDD_PU_Engine();
  virtual void execute();

private:
  //Defining 3 transitions for primary user
  static constexpr float channel_5 = 770e6;
  static constexpr float channel_4 = 769e6;
  static constexpr float channel_3 = 768e6;

  static constexpr float channel_2 = 765e6;
  static constexpr float channel_1 = 764e6;
  static constexpr float channel_0 = 763e6;
};

#endif
