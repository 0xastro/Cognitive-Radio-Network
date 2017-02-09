#include "CE_Spectrum_Sensingv4.hpp"

#define DEBUG 0
#if DEBUG == 1
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

// constructor
CE_Spectrum_Sensingv4::CE_Spectrum_Sensingv4(
    int argc, char **argv, ExtensibleCognitiveRadio *_ECR) {
  ECR = _ECR;
  t1 = timer_create();
  timer_tic(t1);
  tx_is_on = 1;
  noise_floor_measured = 0;

  // initialize buffers to 0
  memset(buffer, 0, 512 * sizeof(float _Complex));
  memset(buffer_F, 0, 512 * sizeof(float _Complex));

  // create fft plan to be used for channel power measurements
  fft = fft_create_plan(512, reinterpret_cast<liquid_float_complex *>(buffer),
                        reinterpret_cast<liquid_float_complex *>(buffer_F),
                        LIQUID_FFT_FORWARD, 0);
}






// destructor
CE_Spectrum_Sensingv4::~CE_Spectrum_Sensingv4() {}







// execute function
void CE_Spectrum_Sensingv4::execute() {
  
  // If the noise floor hasn't been measured yet, do so now.
  if (noise_floor_measured == 0) {
    dprintf("Stopping transceiver\n");
    ECR->stop_tx();
    ECR->stop_rx();

    // Change rx freq to current tx freq
    float rx_freq = ECR->get_rx_freq();
    float tx_freq = ECR->get_tx_freq();

 /**************************************************************/
    if (rx_freq > 765e6) {
      fc = 750e6;
      fshift = -15e6;
    } else {
      fc = 780e6;
      fshift = 15e6;
    }
    printf("\nfc: %.2e\n\n", fc);
    rx_foff = fc - rx_freq;
    tx_foff = -fc + tx_freq;
    ECR->set_tx_freq(fc, fshift);
    //printf("TX FREQ: %e\n",ECR->set_tx_freq(fc, fshift));
    ECR->set_rx_freq(fc, -tx_foff);
    //printf("RX FREQ: %e\n",ECR->set_rx_freq(fc, -tx_foff));
    dprintf("Measuring noise floor\n");
    measureNoiseFloor(ECR); //call the measureNoiseFloor function
/**************************************************************/



/**************************************************************/
    // check if channel is currently occuppied
    int PUDetected = PUisPresent(ECR); //call the PUisPresent Function
    ECR->set_tx_freq(fc, tx_foff);
    // ECR->set_tx_freq(tx_freq);
    ECR->set_rx_freq(fc, rx_foff);
    // restart receiver
    ECR->start_rx();
    // restart transmitter if the channel is unoccupied
    if (!PUDetected) {
      ECR->start_tx();
      tx_is_on = 1;
    }
/**************************************************************/


    // This CE should be executed immediately when run, so the timeout should
    // be set to 0 in the scenario file. After the first run, a new timeout
    // value should be set.
    ECR->set_ce_timeout_ms(desired_timeout_ms);

    noise_floor_measured = 1;
    timer_tic(t1);
  }

  // If it's time to sense the spectrum again
  if (timer_toc(t1) > 1.0 / sensingFrequency_Hz) {
    timer_tic(t1);

    // stop data receiver to enable spectrum sensing
    ECR->stop_rx();
    if (tx_is_on) {
      // Pause Transmission
      ECR->stop_tx();
      tx_is_on = 0;
    }

    // flag indicating to change to transmit frequency
    int switch_tx_freq = 0;

    // Change rx freq to current tx freq
    float rx_freq = fc - rx_foff;
    float tx_freq = fc + tx_foff;
    rx_foff = fc - rx_freq;
    tx_foff = -fc + tx_freq;
    // ECR->set_tx_freq(fc, fshift);
    ECR->set_rx_freq(fc, -tx_foff);

    // pause to allow the frequency to settle
    while (true) {
      if (timer_toc(t1) >= (tune_settling_time_ms / 1e3))
        break;
    }
    timer_tic(t1);

    int PUDetected = PUisPresent(ECR);

    // reset to original frequencies if no PU was detected
    if (!PUDetected) {
      ECR->set_rx_freq(fc, rx_foff);
      ECR->set_tx_freq(fc, tx_foff);
    }
    // Check for PU on other possible tx freq.
    else {
      // printf("\nPU detected in current channel (%-.2f), checking other
      // channel\n", tx_freq);

      if (tx_freq == freq_a)
        tx_freq = freq_b;
      else if (tx_freq == freq_b)
        tx_freq = freq_a;
      else if (tx_freq == freq_x)
        tx_freq = freq_y;
      else if (tx_freq == freq_y)
        tx_freq = freq_x;

      tx_foff = -fc + tx_freq;
      ECR->set_rx_freq(fc, -tx_foff);

      // pause to allow the frequency to settle
      while (true) {
        if (timer_toc(t1) >= (tune_settling_time_ms / 1e3))
          break;
      }
      timer_tic(t1);

      PUDetected = PUisPresent(ECR);
      if (!PUDetected)
        switch_tx_freq = 1;

      // reset receiver frequency to current channel
      ECR->set_rx_freq(fc, rx_foff);
    }

    // restart receiver
    // dprintf("Restarting receiver\n");
    ECR->start_rx();

    // resume transmissions if open channel
    if (!PUDetected) {
      printf("Starting transmitter\n");

      // switch to other channel if applicable
      if (switch_tx_freq) {
        printf("Switching transmit frequency to %-.2f\n", tx_freq);
        ECR->set_tx_freq(fc, tx_foff);
      }

      ECR->start_tx();
      tx_is_on = 1;
    }
  }

  // Receiver frequency selection based on timeouts and bad frames
  if (ECR->CE_metrics.CE_event == ExtensibleCognitiveRadio::TIMEOUT ||
      !ECR->CE_metrics.payload_valid) {
    no_sync_counter++;
    if (no_sync_counter >= no_sync_threshold) {
      float rx_freq = fc - rx_foff;
      no_sync_counter = 0;

      if (rx_freq == freq_a)
        rx_freq = freq_b;
      else if (rx_freq == freq_b)
        rx_freq = freq_a;
      else if (rx_freq == freq_x)
        rx_freq = freq_y;
      else if (rx_freq == freq_y)
        rx_freq = freq_x;

      printf("\nSwitching rx freq to: %f\n", rx_freq);
      rx_foff = fc - rx_freq;
      ECR->set_rx_freq(fc, rx_foff);
    }
  } else
    no_sync_counter = 0;
}





/**************************************************************/
/*
OooOOo.  O       o       o.OOOo.   o.OOoOoo oOoOOoOOo o.OOoOoo  .oOOOo.  oOoOOoOOo ooOoOOo  .oOOOo.  o.     O 
O     `O o       O        O    `o   O           o      O       .O     o      o        O    .O     o. Oo     o 
o      O O       o        o      O  o           o      o       o             o        o    O       o O O    O 
O     .o o       o        O      o  ooOO        O      ooOO    o             O        O    o       O O  o   o 
oOooOO'  o       O        o      O  O           o      O       o             o        o    O       o O   o  O 
o        O       O        O      o  o           O      o       O             O        O    o       O o    O O 
O        `o     Oo        o    .O'  O           O      O       `o     .o     O        O    `o     O' o     Oo 
o'        `OoooO'O        OooOO'   ooOooOoO     o'    ooOooOoO  `OoooO'      o'    ooOOoOo  `OoooO'  O     `o 

*/

// Check if current channel power is signficantly higher than measured noise
// power.
// Return 0 if channel is empty
int CE_Spectrum_Sensingv4::PUisPresent(
    ExtensibleCognitiveRadio *ECR) {
  // set up receive buffers to sense for time specified by sensingPeriod_ms
  const size_t max_samps_per_packet =
      ECR->usrp_rx->get_device()->get_max_recv_samps_per_packet();
  size_t numSensingSamples =
      (long unsigned int)((sensingPeriod_ms / 1000.0) * ECR->get_rx_rate());
  unsigned int numFullPackets = numSensingSamples / max_samps_per_packet;
  size_t samps_per_last_packet = numSensingSamples % max_samps_per_packet;

  uhd::stream_cmd_t s(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
  s.num_samps = numSensingSamples;
  s.stream_now = true;
  ECR->usrp_rx->issue_stream_cmd(s);

  // Calculate channel power
  float channelPower = 0.0;
  float X[512];
  memset(X, 0, 512 * sizeof(float));
  // Get spectrum samples
  for (unsigned int j = 0; j < numFullPackets; j++) {
    ECR->usrp_rx->get_device()->recv(
        buffer, max_samps_per_packet, ECR->metadata_rx,
        uhd::io_type_t::COMPLEX_FLOAT32, uhd::device::RECV_MODE_ONE_PACKET);

    // calculate and sum fft of zero-padded sample buffer
    fft_execute(fft);
    for (unsigned int k = 0; k < 512; k++)
      X[k] += cabsf(buffer_F[k]);
  }
  // If number of samples in last packet is nonzero, get them as well.
  if (samps_per_last_packet) {
    ECR->usrp_rx->get_device()->recv(
        buffer, samps_per_last_packet, ECR->metadata_rx,
        uhd::io_type_t::COMPLEX_FLOAT32, uhd::device::RECV_MODE_ONE_PACKET);

    // calculate and sum fft of zero-padded sample buffer
    fft_execute(fft);
    for (unsigned int k = 0; k < 512; k++)
      X[k] += cabsf(buffer_F[k]);
  }

  // sum absolute value of fft points (discard low frequency content)
  float M = 0.0f;
  for (unsigned int k = 16; k < 496; k++) {
    // printf("FFT pt %i: %f\n", k, cabsf(X[k]));
    M += X[k];
  }

  channelPower = M * M;
  printf("Measured channel power: %.2e\n", channelPower);

  return (channelPower > threshold_coefficient * noise_floor);
}
/**************************************************************/










/**************************************************************/
/*
o.     O  .oOOOo.  ooOoOOo .oOOOo.  o.OOoOoo       OOooOoO  o       .oOOOo.   .oOOOo.  `OooOOo.  
Oo     o .O     o.    O    o     o   O             o       O       .O     o. .O     o.  o     `o 
O O    O O       o    o    O.        o             O       o       O       o O       o  O      O 
O  o   o o       O    O     `OOoo.   ooOO          oOooO   o       o       O o       O  o     .O 
O   o  O O       o    o          `O  O             O       O       O       o O       o  OOooOO'  
o    O O o       O    O           o  o             o       O       o       O o       O  o    o   
o     Oo `o     O'    O    O.    .O  O             o       o     . `o     O' `o     O'  O     O  
O     `o  `OoooO'  ooOOoOo  `oooO'  ooOooOoO       O'      OOoOooO  `OoooO'   `OoooO'   O      o 
                                                                                                                                                                                                
*/

// Try to evaluate the noise floor power
void CE_Spectrum_Sensingv4::measureNoiseFloor(
    ExtensibleCognitiveRadio *ECR) {
  // set up receive buffers to sense for time specified by sensingPeriod_ms
  const size_t max_samps_per_packet =
      ECR->usrp_rx->get_device()->get_max_recv_samps_per_packet();
  size_t numSensingSamples =
      (long unsigned int)((sensingPeriod_ms / 1000.0) * ECR->get_rx_rate());
  unsigned int numFullPackets = numSensingSamples / max_samps_per_packet;
  size_t samps_per_last_packet = numSensingSamples % max_samps_per_packet;

  // USRP samples are always magnitude <= 1 so measured power will always
  // be less than this
  float noisePowerMin = 1e32; //(float)max_samps_per_packet + 1.0;
  // Make numMeasurements measueremnts, each measurementDelay_ms apart
  for (unsigned int i = 0; i < numMeasurements; i++) {
    uhd::stream_cmd_t s(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    s.num_samps = numSensingSamples;
    s.stream_now = true;
    ECR->usrp_rx->issue_stream_cmd(s);

    // Calculate channel power
    float noisePower = 0;
    float X[512];
    memset(X, 0, 512 * sizeof(float));
    // Get spectrum samples
    for (unsigned int j = 0; j < numFullPackets; j++) {
      ECR->usrp_rx->get_device()->recv(
          buffer, max_samps_per_packet, ECR->metadata_rx,
          uhd::io_type_t::COMPLEX_FLOAT32, uhd::device::RECV_MODE_ONE_PACKET);

      // calculate and sum fft of zero-padded sample buffer
      fft_execute(fft);
      for (unsigned int k = 0; k < 512; k++)
        X[k] += cabsf(buffer_F[k]);
    }
    // If number of samples in last packet is nonzero, get them as well.
    if (samps_per_last_packet) {
      ECR->usrp_rx->get_device()->recv(
          buffer, samps_per_last_packet, ECR->metadata_rx,
          uhd::io_type_t::COMPLEX_FLOAT32, uhd::device::RECV_MODE_ONE_PACKET);

      // calculate and sum fft of zero-padded final sample buffer
      fft_execute(fft);
      for (unsigned int k = 0; k < 512; k++)
        X[k] += cabsf(buffer_F[k]);
    }

    // sum magnitude over fft (disregard low frequencies due to ZIF DC offset)
    float M = 0.0f;
    for (unsigned int k = 16; k < 496; k++) {
      // printf("FFT pt %i: %f\n", k, cabsf(X[k]));
      M += X[k];
    }

    // square to calculate power
    noisePower = M * M;
    printf("Measured noise floor %u: %.2e\n", i, noisePower);

    // If channel power is lower than before, than
    // consider it the new minimum noise power
    if (noisePower < noisePowerMin) {
      noisePowerMin = noisePower;
    }

    // Pause before measuring again
    while (true) {
      if (timer_toc(t1) >= (measurementDelay_ms / 1e3))
        break;
    }
    timer_tic(t1);
  }
  noise_floor = noisePowerMin;

  // Lower bound the noise floor (based on experimental values)
  // if(noise_floor < 5e2)
  //	noise_floor = 5e2;

  printf("Measured Noise Floor: %f\n", noise_floor);
}
/**************************************************************/
