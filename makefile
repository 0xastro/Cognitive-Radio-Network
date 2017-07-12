FLAGS = -I include -Wall -fPIC -std=c++11 -g
LIBS = lib/tun.o lib/ecr.o -lliquid -luhd -lpthread -lm -lc -lconfig

#EDIT CE OBJECT LIST START FLAG
CEs = src/cognitive_engine.cpp lib/CE_Template.o lib/CE_Subcarrier_Alloc.o lib/CE_Throughput_Test.o lib/CE_Control_and_Feedback_Test.o lib/CE_Simultaneous_RX_And_Sensing.o lib/CE_Two_Channel_DSA_Spectrum_Sensing.o lib/CE_Mod_Adaptation.o lib/CE_Network_Loading.o lib/CE_Predictive_Node.o lib/CE_FEC_Adaptation.o lib/CE_Two_Channel_DSA_Link_Reliability.o lib/CE_Random_Behaviour_PU.o lib/CE_PU_MARKOV_Chain_Tx.o lib/CE_Two_Channel_DSA_PU.o 
CE_srcs =  cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp
CE_Headers = cognitive_engines/CE_Template/CE_Template.hpp cognitive_engines/test_engines/CE_Subcarrier_Alloc/CE_Subcarrier_Alloc.hpp cognitive_engines/test_engines/CE_Throughput_Test/CE_Throughput_Test.hpp cognitive_engines/test_engines/CE_Control_and_Feedback_Test/CE_Control_and_Feedback_Test.hpp cognitive_engines/test_engines/CE_Simultaneous_RX_And_Sensing/CE_Simultaneous_RX_And_Sensing.hpp cognitive_engines/example_engines/CE_Two_Channel_DSA_Spectrum_Sensing/CE_Two_Channel_DSA_Spectrum_Sensing.hpp cognitive_engines/example_engines/CE_Mod_Adaptation/CE_Mod_Adaptation.hpp cognitive_engines/example_engines/CE_Network_Loading/CE_Network_Loading.hpp cognitive_engines/example_engines/CE_Predictive_Node/CE_Predictive_Node.hpp cognitive_engines/example_engines/CE_FEC_Adaptation/CE_FEC_Adaptation.hpp cognitive_engines/example_engines/CE_Two_Channel_DSA_Link_Reliability/CE_Two_Channel_DSA_Link_Reliability.hpp cognitive_engines/primary_user_engines/CE_Random_Behaviour_PU/CE_Random_Behaviour_PU.hpp cognitive_engines/primary_user_engines/CE_PU_MARKOV_Chain_Tx/CE_PU_MARKOV_Chain_Tx.hpp cognitive_engines/primary_user_engines/CE_Two_Channel_DSA_PU/CE_Two_Channel_DSA_PU.hpp cognitive_engines/CE_Template/CE_Template.hpp cognitive_engines/include/AMC.hpp cognitive_engines/test_engines/CE_Subcarrier_Alloc/CE_Subcarrier_Alloc.hpp cognitive_engines/test_engines/CE_Throughput_Test/CE_Throughput_Test.hpp cognitive_engines/test_engines/CE_Control_and_Feedback_Test/CE_Control_and_Feedback_Test.hpp cognitive_engines/test_engines/CE_Simultaneous_RX_And_Sensing/CE_Simultaneous_RX_And_Sensing.hpp cognitive_engines/example_engines/CE_Two_Channel_DSA_Spectrum_Sensing/CE_Two_Channel_DSA_Spectrum_Sensing.hpp cognitive_engines/example_engines/CE_Mod_Adaptation/include/Mod_Select.hpp cognitive_engines/example_engines/CE_Mod_Adaptation/CE_Mod_Adaptation.hpp cognitive_engines/example_engines/CE_Network_Loading/CE_Network_Loading.hpp cognitive_engines/example_engines/CE_Predictive_Node/CE_Predictive_Node.hpp cognitive_engines/example_engines/CE_FEC_Adaptation/CE_FEC_Adaptation.hpp cognitive_engines/example_engines/CE_FEC_Adaptation/include/FEC_Select.hpp cognitive_engines/example_engines/CE_Two_Channel_DSA_Link_Reliability/CE_Two_Channel_DSA_Link_Reliability.hpp cognitive_engines/primary_user_engines/CE_Random_Behaviour_PU/CE_Random_Behaviour_PU.hpp cognitive_engines/primary_user_engines/CE_PU_MARKOV_Chain_Tx/CE_PU_MARKOV_Chain_Tx.hpp cognitive_engines/primary_user_engines/CE_Two_Channel_DSA_PU/CE_Two_Channel_DSA_PU.hpp 
#EDIT CE OBJECT LIST END FLAG

#EDIT SC START FLAG
SCs = src/scenario_controller.cpp scenario_controllers/SC_Performance_Sweep_Utility/SC_Performance_Sweep_Utility.cpp scenario_controllers/SC_BER_Sweep/SC_BER_Sweep.cpp scenario_controllers/SC_Template/SC_Template.cpp scenario_controllers/SC_Network_Loading/SC_Network_Loading.cpp scenario_controllers/SC_CORNET_Display/SC_CORNET_Display.cpp scenario_controllers/SC_Control_and_Feedback_Test/SC_Control_and_Feedback_Test.cpp scenario_controllers/SC_CORNET_Tutorial/SC_CORNET_Tutorial.cpp 
SC_Headers = scenario_controllers/SC_Performance_Sweep_Utility/SC_Performance_Sweep_Utility.hpp scenario_controllers/SC_BER_Sweep/SC_BER_Sweep.hpp scenario_controllers/SC_Template/SC_Template.hpp scenario_controllers/SC_Network_Loading/SC_Network_Loading.hpp scenario_controllers/SC_CORNET_Display/SC_CORNET_Display.hpp scenario_controllers/SC_Control_and_Feedback_Test/SC_Control_and_Feedback_Test.hpp scenario_controllers/SC_CORNET_Tutorial/SC_CORNET_Tutorial.hpp scenario_controllers/SC_Performance_Sweep_Utility/SC_Performance_Sweep_Utility.hpp scenario_controllers/SC_BER_Sweep/SC_BER_Sweep.hpp scenario_controllers/SC_Template/SC_Template.hpp scenario_controllers/SC_Network_Loading/SC_Network_Loading.hpp scenario_controllers/SC_CORNET_Display/SC_CORNET_Display.hpp scenario_controllers/SC_Control_and_Feedback_Test/SC_Control_and_Feedback_Test.hpp scenario_controllers/SC_CORNET_Tutorial/SC_CORNET_Tutorial.hpp 
#EDIT SC END FLAG

all: lib/crts.o config_cognitive_engines config_scenario_controllers lib/tun.o lib/timer.o lib/ecr.o lib/interferer.o logs/convert_logs_bin_to_octave $(CEs) crts_interferer crts_cognitive_radio crts_controller

lib/crts.o: include/crts.hpp src/crts.cpp
	g++ $(FLAGS) -c -o lib/crts.o src/crts.cpp

config_cognitive_engines: src/config_cognitive_engines.cpp
	g++ $(FLAGS) -o config_cognitive_engines src/config_cognitive_engines.cpp lib/crts.o -lliquid -lconfig -lboost_system 

config_scenario_controllers: src/config_scenario_controllers.cpp
	g++ $(FLAGS) -o config_scenario_controllers src/config_scenario_controllers.cpp lib/crts.o -lconfig -lliquid -lboost_system

lib/tun.o: include/tun.hpp src/tun.cpp
	g++ $(FLAGS) -c -o lib/tun.o src/tun.cpp

lib/timer.o: include/timer.h src/timer.cc
	g++ $(FLAGS) -c -o lib/timer.o src/timer.cc

lib/ecr.o: include/extensible_cognitive_radio.hpp src/extensible_cognitive_radio.cpp 
	g++ $(FLAGS) -c -o lib/ecr.o src/extensible_cognitive_radio.cpp

lib/interferer.o: include/interferer.hpp src/interferer.cpp 
	g++ $(FLAGS) -c -o lib/interferer.o src/interferer.cpp

logs/convert_logs_bin_to_octave: src/convert_logs_bin_to_octave.cpp
	g++ $(FLAGS) -o logs/convert_logs_bin_to_octave src/convert_logs_bin_to_octave.cpp -luhd -lboost_system

crts_interferer: include/interferer.hpp include/crts.hpp src/crts_interferer.cpp src/interferer.cpp src/crts.cpp 
	g++ $(FLAGS) -o crts_interferer src/crts_interferer.cpp lib/crts.o lib/interferer.o lib/timer.o -luhd -lc -lconfig -lliquid -lpthread -lboost_system

crts_cognitive_radio: include/extensible_cognitive_radio.hpp src/tun.cpp src/extensible_cognitive_radio.cpp src/crts_cognitive_radio.cpp  $(CEs) $(CE_srcs) $(CE_Headers)
	g++ $(FLAGS) -o crts_cognitive_radio src/crts_cognitive_radio.cpp lib/crts.o lib/timer.o $(CEs) $(CE_srcs) $(LIBS) -lboost_system

crts_controller: include/crts.hpp src/crts.cpp src/crts_controller.cpp $(SCs) $(SC_Headers)
	g++ $(FLAGS) -o crts_controller src/crts_controller.cpp lib/crts.o lib/timer.o -lconfig -lliquid -lpthread -lboost_system $(SCs)

install:
	cp ./.crts_sudoers /etc/sudoers.d/crts # Filename must not have '_' or '.' in name.
	chmod 440 /etc/sudoers.d/crts

uninstall:
	rm -rf /etc/sudoers.d/crts

.PHONY: doc
doc:
	$(MAKE) -C doc all
cleandoc:
	$(MAKE) -C doc clean

clean:
	rm -rf lib/*.o
	rm -rf crts_cognitive_radio
	rm -rf crts_interferer
	rm -rf crts_controller
	rm -rf logs/convert_logs_bin_to_octave
	rm -rf config_cognitive_engines
	rm -rf config_scenario_controllers
	$(MAKE) -C doc clean

#EDIT CE COMPILATION START FLAG
lib/CE_Template.o: cognitive_engines/CE_Template/CE_Template.cpp cognitive_engines/CE_Template/CE_Template.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Template.o cognitive_engines/CE_Template/CE_Template.cpp 

lib/CE_Subcarrier_Alloc.o: cognitive_engines/test_engines/CE_Subcarrier_Alloc/CE_Subcarrier_Alloc.cpp cognitive_engines/test_engines/CE_Subcarrier_Alloc/CE_Subcarrier_Alloc.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Subcarrier_Alloc.o cognitive_engines/test_engines/CE_Subcarrier_Alloc/CE_Subcarrier_Alloc.cpp 

lib/CE_Throughput_Test.o: cognitive_engines/test_engines/CE_Throughput_Test/CE_Throughput_Test.cpp cognitive_engines/test_engines/CE_Throughput_Test/CE_Throughput_Test.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Throughput_Test.o cognitive_engines/test_engines/CE_Throughput_Test/CE_Throughput_Test.cpp 

lib/CE_Control_and_Feedback_Test.o: cognitive_engines/test_engines/CE_Control_and_Feedback_Test/CE_Control_and_Feedback_Test.cpp cognitive_engines/test_engines/CE_Control_and_Feedback_Test/CE_Control_and_Feedback_Test.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Control_and_Feedback_Test.o cognitive_engines/test_engines/CE_Control_and_Feedback_Test/CE_Control_and_Feedback_Test.cpp 

lib/CE_Simultaneous_RX_And_Sensing.o: cognitive_engines/test_engines/CE_Simultaneous_RX_And_Sensing/CE_Simultaneous_RX_And_Sensing.cpp cognitive_engines/test_engines/CE_Simultaneous_RX_And_Sensing/CE_Simultaneous_RX_And_Sensing.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Simultaneous_RX_And_Sensing.o cognitive_engines/test_engines/CE_Simultaneous_RX_And_Sensing/CE_Simultaneous_RX_And_Sensing.cpp 

lib/CE_Two_Channel_DSA_Spectrum_Sensing.o: cognitive_engines/example_engines/CE_Two_Channel_DSA_Spectrum_Sensing/CE_Two_Channel_DSA_Spectrum_Sensing.cpp cognitive_engines/example_engines/CE_Two_Channel_DSA_Spectrum_Sensing/CE_Two_Channel_DSA_Spectrum_Sensing.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Two_Channel_DSA_Spectrum_Sensing.o cognitive_engines/example_engines/CE_Two_Channel_DSA_Spectrum_Sensing/CE_Two_Channel_DSA_Spectrum_Sensing.cpp 

lib/CE_Mod_Adaptation.o: cognitive_engines/example_engines/CE_Mod_Adaptation/CE_Mod_Adaptation.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/CE_Mod_Adaptation.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Mod_Adaptation.o cognitive_engines/example_engines/CE_Mod_Adaptation/CE_Mod_Adaptation.cpp 

lib/CE_Network_Loading.o: cognitive_engines/example_engines/CE_Network_Loading/CE_Network_Loading.cpp cognitive_engines/example_engines/CE_Network_Loading/CE_Network_Loading.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Network_Loading.o cognitive_engines/example_engines/CE_Network_Loading/CE_Network_Loading.cpp 

lib/CE_Predictive_Node.o: cognitive_engines/example_engines/CE_Predictive_Node/CE_Predictive_Node.cpp cognitive_engines/example_engines/CE_Predictive_Node/CE_Predictive_Node.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Predictive_Node.o cognitive_engines/example_engines/CE_Predictive_Node/CE_Predictive_Node.cpp 

lib/CE_FEC_Adaptation.o: cognitive_engines/example_engines/CE_FEC_Adaptation/CE_FEC_Adaptation.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/CE_FEC_Adaptation.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_FEC_Adaptation.o cognitive_engines/example_engines/CE_FEC_Adaptation/CE_FEC_Adaptation.cpp 

lib/CE_Two_Channel_DSA_Link_Reliability.o: cognitive_engines/example_engines/CE_Two_Channel_DSA_Link_Reliability/CE_Two_Channel_DSA_Link_Reliability.cpp cognitive_engines/example_engines/CE_Two_Channel_DSA_Link_Reliability/CE_Two_Channel_DSA_Link_Reliability.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Two_Channel_DSA_Link_Reliability.o cognitive_engines/example_engines/CE_Two_Channel_DSA_Link_Reliability/CE_Two_Channel_DSA_Link_Reliability.cpp 

lib/CE_Random_Behaviour_PU.o: cognitive_engines/primary_user_engines/CE_Random_Behaviour_PU/CE_Random_Behaviour_PU.cpp cognitive_engines/primary_user_engines/CE_Random_Behaviour_PU/CE_Random_Behaviour_PU.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Random_Behaviour_PU.o cognitive_engines/primary_user_engines/CE_Random_Behaviour_PU/CE_Random_Behaviour_PU.cpp 

lib/CE_PU_MARKOV_Chain_Tx.o: cognitive_engines/primary_user_engines/CE_PU_MARKOV_Chain_Tx/CE_PU_MARKOV_Chain_Tx.cpp cognitive_engines/primary_user_engines/CE_PU_MARKOV_Chain_Tx/CE_PU_MARKOV_Chain_Tx.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_PU_MARKOV_Chain_Tx.o cognitive_engines/primary_user_engines/CE_PU_MARKOV_Chain_Tx/CE_PU_MARKOV_Chain_Tx.cpp 

lib/CE_Two_Channel_DSA_PU.o: cognitive_engines/primary_user_engines/CE_Two_Channel_DSA_PU/CE_Two_Channel_DSA_PU.cpp cognitive_engines/primary_user_engines/CE_Two_Channel_DSA_PU/CE_Two_Channel_DSA_PU.hpp cognitive_engines/src/AMC.cpp cognitive_engines/example_engines/CE_Mod_Adaptation/src/Mod_Select.cpp cognitive_engines/example_engines/CE_FEC_Adaptation/src/FEC_Select.cpp 
	g++ $(FLAGS) -c -o lib/CE_Two_Channel_DSA_PU.o cognitive_engines/primary_user_engines/CE_Two_Channel_DSA_PU/CE_Two_Channel_DSA_PU.cpp 

#EDIT CE COMPILATION END FLAG
    

