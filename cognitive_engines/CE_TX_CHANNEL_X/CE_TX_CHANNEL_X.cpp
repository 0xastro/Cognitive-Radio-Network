//
#include "CE_TX_CHANNEL_X.hpp"

// constructor
CE_TX_CHANNEL_X::CE_TX_CHANNEL_X(int argc, char **argv, ExtensibleCognitiveRadio *_ECR) {
  ECR = _ECR;
}

CE_TX_CHANNEL_X::~CE_TX_CHANNEL_X() {}


// execute function
void CE_TX_CHANNEL_X::execute() {

  ECR->stop_rx();
  if (!Channel_Specified){
  	ECR->stop_tx();
	printf("Desired Tx Channel:\t");
	scanf("%f",&DESIRED_CHANNEL);
	Channel_Specified=1;
	ECR->start_tx();
	}
    printf("Transmit frequency: %f\n", ECR->get_tx_freq());
}


/*
Stop the Receiver
Initialize Channel_Specified to zero

while the Channel_Specified has not as yet specified
	stop transmitter 
	Input the Desired Channel
	switch Channel_Specified flag into 1
	start transmitter on the Desired Channel
*/
