#ifndef _CR_HPP_
#define _CR_HPP_

#include <liquid/liquid.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <net/if.h>
#include <uhd/utils/msg.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/types/tune_request.hpp>
#include "crts.hpp"
#include "cognitive_engine.hpp"
#include "timer.h"

#define ECR_CONTROL_INFO_BYTES 6

// thread functions
void *ECR_tx_worker(void *_arg);
void *ECR_rx_worker(void *_arg);
void *ECR_ce_worker(void *_arg);

// function that receives frame from PHY layer
int rxCallback(unsigned char *_header, int _header_valid,
               unsigned char *_payload, unsigned int _payload_len,
               int _payload_valid, framesyncstats_s _stats, void *_userdata);

/// \brief Defines the possible states of the transmitter, which
/// can be either off, continuously transmittering, or transmitting
/// for a finite number of frames.
enum tx_states {
  TX_STOPPED = 0,
  TX_CONTINUOUS,
  TX_BURST
};

/// \brief Defines the possible states of the receiver, which
/// can be either off, or continuously receiving. 
enum rx_states {
  RX_STOPPED = 0,
  RX_CONTINUOUS
};

/// \brief Defines states for worker threads, which can be
/// halted, ready, or running.
enum worker_states {
  WORKER_HALTED = 0,
  WORKER_READY,
  WORKER_RUNNING
};

class ExtensibleCognitiveRadio {
public:
  ExtensibleCognitiveRadio();
  ~ExtensibleCognitiveRadio();

  //=================================================================================
  // Enums and Structs
  //=================================================================================

  /// \brief Defines the different types of CE events.
  ///
  /// The different circumstances under which the CE
  /// can be executed are defined here.
  enum CE_Event {
    /// \brief The CE had not been executed for a period
    /// of time as defined by ExtensibleCognitiveRadio::ce_timeout_ms.
    /// It is now executed as a timeout event.
    TIMEOUT = 0,

    /// \brief A PHY layer frame has been received, causing the execution
    /// of the CE. 
    PHY_FRAME_RECEIVED,

    /// \brief Indicates that the transmit worker has completed
    /// transmission of its final frame
    TX_COMPLETE,
    
    /// \brief The receiver processing is not able to keep up with the
    /// current settings.
    UHD_OVERFLOW,

    /// \brief The transmitter is not providing samples fast enough
    /// the the USRP
    UHD_UNDERRUN,

    /// \brief This event enables the design of custom spectrum sensing
    /// which can be employed without interrupting the normal reception of
    /// frames.
    USRP_RX_SAMPS
  };

  /// \brief Defines the types of frames used by the ECR
  enum FrameType {

    /// \brief The frame contains application
    /// layer data.
    ///
    /// Data frames contain IP packets that are read from
    /// the virtual network interface and subsequently
    /// transmitted over the air.
    DATA = 0,

    /// \brief The frame was sent explicitly at the
    /// behest of another cognitive engine (CE) in the
    /// network and it contains custom data for use
    /// by the receiving CE.
    ///
    /// The handling of ExtensibleCognitiveRadio::DATA
    /// frames is performed automatically by the
    /// Extensible Cognitive Radio (ECR).
    /// However, the CE may initiate the transmission
    /// of a custom control frame containing
    /// information to be relayed to another CE in the
    /// network.
    /// A custom frame can be sent using
    /// ExtensibleCognitiveRadio::transmit_frame().
    CONTROL,

    /// \brief The Extensible Cognitve Radio (ECR) is
    /// unable to determine the type of the received frame.
    ///
    /// The received frame was too corrupted to determine
    /// its type.
    UNKNOWN
  };

  // metric struct
  /// \brief Contains metric information related
  /// to the quality of a received frame.
  /// This information is made available to the
  /// custom Cognitive_Engine::execute() implementation
  /// and is accessed in the instance of this struct:
  /// ExtensibleCognitiveRadio::CE_metrics.
  ///
  /// The members of this struct will be valid when a frame has
  /// been received which will be indicated when the
  /// ExtensibleCognitiveRadio::metric_s.CE_event == PHY.
  /// Otherwise, they will represent results from previous frames.
  ///
  /// The valid members under a
  /// ExtensibleCognitiveRadio::PHY event are:
  ///
  /// ExtensibleCognitiveRadio::metric_s::CE_frame,
  ///
  /// ExtensibleCognitiveRadio::metric_s::control_valid,
  ///
  /// ExtensibleCognitiveRadio::metric_s::control_info,
  ///
  /// ExtensibleCognitiveRadio::metric_s::payload,
  ///
  /// ExtensibleCognitiveRadio::metric_s::payload_valid,
  ///
  /// ExtensibleCognitiveRadio::metric_s::payload_len,
  ///
  /// ExtensibleCognitiveRadio::metric_s::frame_num,
  ///
  /// ExtensibleCognitiveRadio::metric_s::stats, and
  ///
  /// ExtensibleCognitiveRadio::metric_s::time_spec
  struct metric_s {

    /// \brief Specifies the circumstances under which
    /// the CE was executed.
    ///
    /// When the CE is executed, this value is set according
    /// to the type of event that caused the CE execution,
    /// as specified in ExtensibleCognitiveRadio::Event.
    ExtensibleCognitiveRadio::CE_Event CE_event;

    // PHY
    /// \brief Specifies the type of frame received as
    /// defined by ExtensibleCognitiveRadio::FrameType
    ExtensibleCognitiveRadio::FrameType CE_frame;

    /// \brief Indicates whether the \p control information
    /// of the received frame passed error checking tests.
    ///
    /// Derived from
    /// \c <a href="http://liquidsdr.org/">liquid-dsp</a>. See the
    /// <a href="http://liquidsdr.org/doc/tutorial_ofdmflexframe.html">Liquid
    /// Documentation</a>
    /// for more information.
    int control_valid;

    /// \brief The control info of the received frame.
    unsigned char control_info[6];

    /// \brief The payload data of the received frame.
    unsigned char *payload;

    /// \brief Indicates whether the \p payload of the
    /// received frame passed error checking tests.
    ///
    /// Derived from
    /// \c <a href="http://liquidsdr.org/">liquid-dsp</a>. See the
    /// <a href="http://liquidsdr.org/doc/tutorial_ofdmflexframe.html">Liquid
    /// Documentation</a>
    /// for more information.
    int payload_valid;

    /// \brief The number of elements of the \p payload array.
    ///
    /// Equal to the byte length of the \p payload.
    unsigned int payload_len;

    /// \brief The frame number of the received
    /// ExtensibleCognitiveRadio::DATA frame.
    ///
    /// Each ExtensibleCognitiveRadio::DATA frame transmitted
    /// by the ECR is assigned a number, according to the order
    /// in which it was transmitted.
    unsigned int frame_num;

    /// \brief The statistics of the received frame as
    /// reported by
    /// \c <a href="http://liquidsdr.org/">liquid-dsp</a>.
    ///
    /// For information about its members, refer to the
    /// <a
    /// href="http://liquidsdr.org/doc/framing.html#framing:framesyncstats_s">Liquid
    /// Documentation</a>.
    framesyncstats_s stats; // stats used by ofdmtxrx object (RSSI, EVM)

    /// \brief The
    /// <a
    /// href="http://files.ettus.com/manual/classuhd_1_1time__spec__t.html">uhd::time_spec_t</a>
    /// object returned by the
    /// <a href="http://files.ettus.com/manual/index.html">UHD</a> driver upon
    /// reception
    /// of a complete frame.
    ///
    /// This serves as a marker to denote at what time the
    /// end of the frame was received.
    uhd::time_spec_t time_spec;
  };

  /// \brief Contains parameters defining how
  /// to handle frame transmission.
  ///
  /// The member parameters are accessed using the
  /// instance of the struct:
  /// ExtensibleCognitiveRadio::tx_params.
  ///
  /// Note that for frames to be received successfully
  /// These settings must match the corresponding settings
  /// at the receiver.
  struct tx_parameter_s {

    /// \brief The number of subcarriers in the OFDM waveform
    /// generated by
    /// <a href="http://liquidsdr.org/">liquid</a>.
    ///
    /// See the
    /// <a href="http://liquidsdr.org/doc/tutorial_ofdmflexframe.html">OFDM
    /// Framing Tutorial</a>
    /// for details.
    unsigned int numSubcarriers; // number of subcarriers

    /// \brief The length of the cyclic prefix in the OFDM waveform
    /// generator from
    /// <a href="http://liquidsdr.org/">liquid</a>.
    ///
    /// See the
    /// <a href="http://liquidsdr.org/doc/tutorial_ofdmflexframe.html">OFDM
    /// Framing Tutorial</a>
    /// for details.
    unsigned int cp_len; // cyclic prefix length

    /// \brief The overlapping taper length in the OFDM waveform
    /// generator from
    /// <a href="http://liquidsdr.org/">liquid</a>.
    ///
    /// See the
    /// <a href="http://liquidsdr.org/doc/tutorial_ofdmflexframe.html">OFDM
    /// Framing Tutorial</a>
    /// and the
    /// <a
    /// href="http://liquidsdr.org/doc/framing.html#framing:ofdmflexframe:tapering">Liquid
    /// Documentation Reference</a>
    /// for details.
    unsigned int taper_len; // taper length

    /// \brief An array of \p unsigned \p char whose number of elements is
    /// ExtensibleCognitiveRadio::tx_parameter_s::numSubcarriers.
    /// Each element in the array should define that subcarrier's allocation.
    ///
    /// A subcarrier's allocation defines it as a null subcarrier, a pilot
    /// subcarrier, or a data subcarrier.
    ///
    /// See
    /// <a
    /// href="http://liquidsdr.org/doc/framing.html#framing:ofdmflexframe:subcarrier_allocation">Subcarrier
    /// Allocation</a>
    /// in the
    /// <a href="http://liquidsdr.org/">liquid</a>
    /// documentation for details.
    ///
    /// Also refer to the
    /// <a href="http://liquidsdr.org/doc/tutorial_ofdmflexframe.html">OFDM
    /// Framing Tutorial</a>
    /// for more information.
    unsigned char *subcarrierAlloc;

    /// \brief The properties for the OFDM frame generator from
    /// <a href="http://liquidsdr.org/">liquid</a>.
    ///
    /// See the
    /// <a href="http://liquidsdr.org/doc/tutorial_ofdmflexframe.html">Liquid
    /// Documentation</a>
    /// for details.
    ///
    /// Members of this struct can be accessed with the following functions:
    /// - \p check:
    ///     + ExtensibleCognitiveRadio::set_tx_crc()
    ///     + ExtensibleCognitiveRadio::get_tx_crc().
    /// - \p fec0:
    ///     + ExtensibleCognitiveRadio::set_tx_fec0()
    ///     + ExtensibleCognitiveRadio::get_tx_fec0().
    /// - \p fec1:
    ///     + ExtensibleCognitiveRadio::set_tx_fec1()
    ///     + ExtensibleCognitiveRadio::get_tx_fec1().
    /// - \p mod_scheme:
    ///     + ExtensibleCognitiveRadio::set_tx_modulation()
    ///     + ExtensibleCognitiveRadio::get_tx_modulation().
    ofdmflexframegenprops_s fgprops; // frame generator properties

    /// \brief The value of the hardware gain for the transmitter. In dB.
    ///
    /// Sets the gain of the hardware amplifier in the transmit chain
    /// of the USRP.
    /// This value is passed directly to
    /// <a href="http://files.ettus.com/manual/index.html">UHD</a>.
    ///
    /// It can be accessed with ExtensibleCognitiveRadio::set_tx_gain_uhd()
    /// and ExtensibleCognitiveRadio::get_tx_gain_uhd().
    ///
    /// Run
    ///
    ///     $ uhd_usrp_probe
    ///
    /// for details about the particular gain limits of your USRP device.
    double tx_gain_uhd;

    /// \brief The software gain of the transmitter. In dB.
    ///
    /// In addition to the hardware gain
    /// (ExtensibleCognitiveRadio::tx_parameter_s::tx_gain_uhd),
    /// the gain of the transmission can be adjusted in software by
    /// setting this parameter.  It is converted to a linear factor
    /// and then applied to the frame samples before they are sent to
    /// <a href="http://files.ettus.com/manual/index.html">UHD</a>.
    ///
    /// It can be accessed with ExtensibleCognitiveRadio::set_tx_gain_soft()
    /// and ExtensibleCognitiveRadio::get_tx_gain_soft().
    ///
    /// Note that the values of samples sent to
    /// <a href="http://files.ettus.com/manual/index.html">UHD</a>
    /// must be between -1 and 1.
    /// Typically this value is set to around -12 dB based on the peak-
    /// to-average power ratio of OFDM signals. Allowing some slight
    /// clipping can improve overall signal power at the expense of
    /// added distortion.
    double tx_gain_soft;

    /// \brief The transmitter local oscillator frequency in Hertz.
    ///
    /// It can be accessed with ExtensibleCognitiveRadio::set_tx_freq()
    /// and ExtensibleCognitiveRadio::get_tx_freq().
    ///
    /// This value is passed directly to
    /// <a href="http://files.ettus.com/manual/index.html">UHD</a>.
    double tx_freq;

    /// \brief The transmitter NCO frequency in Hertz.
    ///
    /// The USRP has an NCO which can be used to digitally mix
    /// the signal anywhere within the baseband bandwidth of the
    /// USRP daughterboard. This can be useful for offsetting the
    /// tone resulting from LO leakage of the ZIF transmitter
    /// used by the USRP.
    double tx_dsp_freq;

    /// \brief The sample rate of the transmitter in samples/second.
    ///
    /// It can be accessed with ExtensibleCognitiveRadio::set_tx_rate()
    /// and ExtensibleCognitiveRadio::get_tx_rate().
    ///
    /// This value is passed directly to
    /// <a href="http://files.ettus.com/manual/index.html">UHD</a>.
    double tx_rate;

    unsigned int payload_sym_length;
  };

  // rx parameter struct
  /// \brief Contains parameters defining how
  /// to handle frame reception.
  ///
  /// The member parameters are accessed using the
  /// instance of the struct:
  /// ExtensibleCognitiveRadio::tx_params.
  ///
  /// Note that for frames to be received successfully
  /// These settings must match the corresponding settings
  /// at the transmitter.
  struct rx_parameter_s {

    /// \brief The number of subcarriers in the OFDM waveform
    /// generated by
    /// <a href="http://liquidsdr.org/">liquid</a>.
    ///
    /// See the
    /// <a href="http://liquidsdr.org/doc/tutorial_ofdmflexframe.html">OFDM
    /// Framing Tutorial</a>
    /// for details.
    unsigned int numSubcarriers; // number of subcarriers

    /// \brief The length of the cyclic prefix in the OFDM waveform
    /// generator from
    /// <a href="http://liquidsdr.org/">liquid</a>.
    ///
    /// See the
    /// <a href="http://liquidsdr.org/doc/tutorial_ofdmflexframe.html">OFDM
    /// Framing Tutorial</a>
    /// for details.
    unsigned int cp_len; // cyclic prefix length

    /// \brief The overlapping taper length in the OFDM waveform
    /// generator from
    /// <a href="http://liquidsdr.org/">liquid</a>.
    ///
    /// See the
    /// <a href="http://liquidsdr.org/doc/tutorial_ofdmflexframe.html">OFDM
    /// Framing Tutorial</a>
    /// and the
    /// <a
    /// href="http://liquidsdr.org/doc/framing.html#framing:ofdmflexframe:tapering">Liquid
    /// Documentation Reference</a>
    /// for details.
    unsigned int taper_len; // taper length

    /// \brief An array of \p unsigned \p char whose number of elements is
    /// ExtensibleCognitiveRadio::tx_parameter_s::numSubcarriers.
    /// Each element in the array should define that subcarrier's allocation.
    ///
    /// A subcarrier's allocation defines it as a null subcarrier, a pilot
    /// subcarrier, or a data subcarrier.
    ///
    /// See
    /// <a
    /// href="http://liquidsdr.org/doc/framing.html#framing:ofdmflexframe:subcarrier_allocation">Subcarrier
    /// Allocation</a>
    /// in the
    /// <a href="http://liquidsdr.org/">liquid</a>
    /// documentation for details.
    ///
    /// Also refer to the
    /// <a href="http://liquidsdr.org/doc/tutorial_ofdmflexframe.html">OFDM
    /// Framing Tutorial</a>
    /// for more information.
    unsigned char *subcarrierAlloc;

    /// \brief The value of the hardware gain for the receiver. In dB.
    ///
    /// Sets the gain of the hardware amplifier in the receive chain
    /// of the USRP.
    /// This value is passed directly to
    /// <a href="http://files.ettus.com/manual/index.html">UHD</a>.
    ///
    /// It can be accessed with ExtensibleCognitiveRadio::set_rx_gain_uhd()
    /// and ExtensibleCognitiveRadio::get_rx_gain_uhd().
    ///
    /// Run
    ///
    ///     $ uhd_usrp_probe
    ///
    /// for details about the particular gain limits of your USRP device.
    double rx_gain_uhd;

    /// \brief The receiver local oscillator frequency in Hertz.
    ///
    /// It can be accessed with ExtensibleCognitiveRadio::set_rx_freq()
    /// and ExtensibleCognitiveRadio::get_rx_freq().
    ///
    /// This value is passed directly to
    /// <a href="http://files.ettus.com/manual/index.html">UHD</a>.
    double rx_freq;

    /// \brief The transmitter NCO frequency in Hertz.
    ///
    /// The USRP has an NCO which can be used to digitally mix
    /// the signal anywhere within the baseband bandwidth of the
    /// USRP daughterboard. This can be useful for offsetting the
    /// tone resulting from LO leakage of the ZIF receiver
    /// used by the USRP.
    double rx_dsp_freq;

    /// \brief The sample rate of the receiver in samples/second.
    ///
    /// It can be accessed with ExtensibleCognitiveRadio::set_rx_rate()
    /// and ExtensibleCognitiveRadio::get_rx_rate().
    ///
    /// This value is passed directly to
    /// <a href="http://files.ettus.com/manual/index.html">UHD</a>.
    double rx_rate;
  };

  // struct for receive statistics
  struct rx_statistics{
    int frames_received;
    int valid_frames;
    float evm_dB;
    float rssi_dB;
    float per;
    float ber;
    float throughput;
    int uhd_overflows;
  };

  //=================================================================================
  // Cognitive Engine Methods
  //=================================================================================

  void set_ce(char *ce, int argc, char **argv); // method to set CE to custom defined subclass
  void start_ce();
  void stop_ce();

  /// \brief Assign a value to ExtensibleCognitiveRadio::ce_timeout_ms.
  void set_ce_timeout_ms(double new_timeout_ms);

  /// \brief Get the current value of ExtensibleCognitiveRadio::ce_timeout_ms
  double get_ce_timeout_ms();

  /// \brief The instance of
  /// ExtensibleCognitiveRadio::metric_s
  /// made accessible to the Cognitive_Engine.
  struct metric_s CE_metrics;

  /// \brief Allows you to turn on/off the USRP_RX_SAMPLES events which allow
  /// you to perform custom spectrum sensing in the CE while the liquid-ofdm
  /// receiver continues to run
  void set_ce_sensing(int ce_sensing);

  /// \brief USRP samples will be written to this buffer if the ce_sensing_flag
  /// is set.
  std::complex<float> *ce_usrp_rx_buffer;

  /// \brief Length of the buffer for USRP samples
  int ce_usrp_rx_buffer_length;

  //=================================================================================
  // Networking Methods
  //=================================================================================

  /// \brief Used to set the IP of the ECR's virtual network interface
  void set_ip(char *ip);

  /// \brief Allows you to set the tx buffer length for the virtual network interface
  /// This could be useful in trading off between dropped packets and latency with a
  /// UDP connection
  void set_tx_queue_len(int queue_len);

  /// \brief Returns the number of bytes currently queued for transmission
  int get_tx_queued_bytes();

  /// \brief Decrements the count of bytes currently queued for transmission
  /// This function is only used as a work around since tun interfaces don't
  /// allow you to read the number of queued bytes.
  void dec_tx_queued_bytes(int n);

  /// \brief Increments the count of bytes currently queued for transmission
  /// This function is only used as a work around since tun interfaces don't
  /// allow you to read the number of queued bytes.
  void inc_tx_queued_bytes(int n);

  //=================================================================================
  // Transmitter Methods
  //=================================================================================

  /// \brief Set the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_freq.
  void set_tx_freq(double _tx_freq);

  void set_tx_freq(double _tx_freq, double _dsp_freq);

  /// \brief Set the value of ExtensibleCognitiveRadio::tx_parameter_s::tx_rate.
  void set_tx_rate(double _tx_rate);

  /// \brief Set the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::tx_gain_soft.
  void set_tx_gain_soft(double _tx_gain_soft);

  /// \brief Set the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::tx_gain_uhd.
  void set_tx_gain_uhd(double _tx_gain_uhd);

  void set_tx_antenna(char *_tx_antenna);

  /// \brief Set the value of \p mod_scheme in
  /// ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
  void set_tx_modulation(int mod_scheme);

  /// \brief Set the value of \p check in
  /// ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
  void set_tx_crc(int crc_scheme);

  /// \brief Set the value of \p fec0 in
  /// ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
  void set_tx_fec0(int fec_scheme);

  /// \brief Set the value of \p fec1 in
  /// ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
  void set_tx_fec1(int fec_scheme);

  /// \brief Set the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::numSubcarriers.
  void set_tx_subcarriers(unsigned int subcarriers);

  /// \brief Set ExtensibleCognitiveRadio::tx_parameter_s::subcarrierAlloc.
  void set_tx_subcarrier_alloc(char *_subcarrierAlloc);

  /// \brief Set the value of ExtensibleCognitiveRadio::tx_parameter_s::cp_len.
  void set_tx_cp_len(unsigned int cp_len);

  /// \brief Set the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::taper_len.
  void set_tx_taper_len(unsigned int taper_len);

  /// \brief Set the control information used for future transmit frames.
  void set_tx_control_info(unsigned char *_control_info);

  /// \brief Set the number of symbols transmitted in each frame payload.
  /// For now since the ECR does not have any segmentation/concatenation
  /// capabilities,
  /// the actual payload will be an integer number of IP packets, so this value
  /// really
  /// provides a lower bound for the payload length in symbols.
  void set_tx_payload_sym_len(unsigned int len);

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::tx_freq.
  double get_tx_freq();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::tx_freq.
  double get_tx_lo_freq();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::tx_state.
  int get_tx_state();
  
  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::tx_freq.
  double get_tx_dsp_freq();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::tx_rate.
  double get_tx_rate();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::tx_gain_soft.
  double get_tx_gain_soft();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::tx_gain_uhd.
  double get_tx_gain_uhd();

  char *get_tx_antenna();

  /// \brief Return the value of \p mod_scheme in
  /// ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
  int get_tx_modulation();

  /// \brief Return the value of \p check in
  /// ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
  int get_tx_crc();

  /// \brief Return the value of \p fec0 in
  /// ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
  int get_tx_fec0();

  /// \brief Return the value of \p fec1 in
  /// ExtensibleCognitiveRadio::tx_parameter_s::fgprops.
  int get_tx_fec1();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::numSubcarriers.
  unsigned int get_tx_subcarriers();

  /// \brief Get current
  /// ExtensibleCognitiveRadio::tx_parameter_s::subcarrierAlloc.
  ///
  /// \p subcarrierAlloc should be a pointer to an array of size
  /// ExtensibleCognitiveRadio::tx_parameter_s::numSubcarriers.
  /// The array will then be filled with the current subcarrier allocation.
  void get_tx_subcarrier_alloc(char *subcarrierAlloc);

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::cp_len.
  unsigned int get_tx_cp_len();

  /// \breif Return the value of
  /// ExtensibleCognitiveRadio::tx_parameter_s::taper_len.
  unsigned int get_tx_taper_len();

  void get_tx_control_info(unsigned char *_control_info);

  double get_tx_data_rate();
  
  void start_tx();
  void start_tx_burst(unsigned int _num_tx_frames,
                      float _max_tx_time_ms);
  void stop_tx();
  void reset_tx();

  /// \brief Transmit a control frame.
  ///
  /// The cognitive engine (CE) can initiate transmission
  /// of a frame dedicated to control information by 
  /// calling this function.
  /// \p _payload is an array of \c unsigned \c char
  /// and can be any length. It can contain any data
  /// as would be useful to the CE.
  ///
  /// \p _payload_len is the number of elements in
  /// \p _payload.
  void transmit_control_frame(unsigned char *_payload, unsigned int _payload_len);

  //=================================================================================
  // Receiver Methods
  //=================================================================================

  /// \brief Set the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_freq.
  void set_rx_freq(double _rx_freq);

  void set_rx_freq(double _rx_freq, double _dsp_freq);

  /// \brief Set the value of ExtensibleCognitiveRadio::rx_parameter_s::rx_rate.
  void set_rx_rate(double _rx_rate);

  /// \brief Set the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::rx_gain_uhd.
  void set_rx_gain_uhd(double _rx_gain_uhd);

  void set_rx_antenna(char *_rx_antenna);

  /// \brief Set the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::numSubcarriers.
  void set_rx_subcarriers(unsigned int subcarriers);

  /// \brief Set ExtensibleCognitiveRadio::rx_parameter_s::subcarrierAlloc.
  void set_rx_subcarrier_alloc(char *_subcarrierAlloc);

  /// \brief Set the value of ExtensibleCognitiveRadio::rx_parameter_s::cp_len.
  void set_rx_cp_len(unsigned int cp_len);

  /// \brief Set the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::taper_len.
  void set_rx_taper_len(unsigned int taper_len);

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::rx_state.
  int get_rx_state();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::rx_worker_state.
  int get_rx_worker_state();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::rx_freq.
  double get_rx_freq();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::rx_freq.
  double get_rx_lo_freq();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::rx_freq.
  double get_rx_dsp_freq();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::rx_rate.
  double get_rx_rate();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::rx_gain_uhd.
  double get_rx_gain_uhd();

  char *get_rx_antenna();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::numSubcarriers.
  unsigned int get_rx_subcarriers();

  /// \brief Get current
  /// ExtensibleCognitiveRadio::rx_parameter_s::subcarrierAlloc.
  ///
  /// \p subcarrierAlloc should be a pointer to an array of size
  /// ExtensibleCognitiveRadio::rx_parameter_s::numSubcarriers.
  /// The array will then be filled with the current subcarrier allocation.
  void get_rx_subcarrier_alloc(char *subcarrierAlloc);

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::cp_len.
  unsigned int get_rx_cp_len();

  /// \brief Return the value of
  /// ExtensibleCognitiveRadio::rx_parameter_s::taper_len.
  unsigned int get_rx_taper_len();

  void get_rx_control_info(unsigned char *_control_info);

  void reset_rx();
  void start_rx();
  void stop_rx();
  void start_liquid_rx();
  void stop_liquid_rx();

  // functions/variables for tracking rx statistics
  void set_rx_stat_tracking(bool state, float sec);
  float get_rx_stat_tracking_period();
  struct rx_statistics get_rx_stats();
  void reset_rx_stats();
  
  //=================================================================================
  // Print/Log Methods and Variables
  //=================================================================================

  void print_metrics(ExtensibleCognitiveRadio *CR);
  int print_metrics_flag;
  void log_rx_metrics();
  void log_tx_parameters();
  int log_phy_rx_flag;
  int log_phy_tx_flag;
  char phy_rx_log_file[255];
  char phy_tx_log_file[255];
  void reset_log_files();

  std::ofstream log_rx_fstream;
  std::ofstream log_tx_fstream;

  //=================================================================================
  // USRP Objects
  //=================================================================================

  uhd::usrp::multi_usrp::sptr usrp_tx;
  uhd::tx_metadata_t metadata_tx;

  uhd::usrp::multi_usrp::sptr usrp_rx;
  uhd::rx_metadata_t metadata_rx;

private:
  //=================================================================================
  // Private Cognitive Engine Objects
  //=================================================================================

  CognitiveEngine *CE; // pointer to CE object

  /// \brief The maximum length of time to go
  /// without an event before executing the CE
  /// under a timeout event. In milliseconds.
  ///
  /// The CE is executed every time an event occurs.
  /// The CE can also be executed if no event has occured
  /// after some period of time.
  /// This is referred to as a timeout event and this variable
  /// defines the length of the timeout period in milliseconds.
  ///
  /// It can be accessed using ExtensibleCognitiveRadio::set_ce_timeout_ms()
  /// and ExtensibleCognitiveRadio::get_ce_timeout_ms().
  double ce_timeout_ms;

  // variables to enable/disable ce events
  bool ce_phy_events;
  int ce_sensing_flag;

  // cognitive engine threading objects
  pthread_t CE_process;
  pthread_mutex_t CE_mutex;
  /*
   * FFTW planner routines are NOT re-entrant.
   * See http://www.fftw.org/fftw3_doc/Thread-safety.html#Thread-safety
   */
  pthread_mutex_t CE_fftw_mutex;
  pthread_cond_t CE_cond;
  pthread_cond_t CE_execute_sig;
  bool ce_thread_running;
  bool ce_running;
  friend void *ECR_ce_worker(void *);

  // private members for tracking rx statistics
  struct rx_statistics rx_stats;
  bool rx_stat_tracking;
  bool reset_rx_stats_flag;
  float rx_stat_tracking_period;
  void update_rx_stats(bool frame_received);
  char known_net_payload[CRTS_CR_PACKET_LEN];

  //=================================================================================
  // Private Network Interface Objects
  //=================================================================================

  int tunfd; // virtual network interface
  // String for TUN interface name
  char tun_name[IFNAMSIZ];
  int tx_queued_bytes;
  int tx_queue_len;
  // String for commands for TUN interface
  char systemCMD[200];

  friend void uhd_msg_handler(uhd::msg::type_t type, const std::string &msg);
  static int uhd_msg;

  //=================================================================================
  // Private Receiver Objects
  //=================================================================================

  // receiver properties/objects
  struct rx_parameter_s rx_params;
  bool update_rx_flag;
  bool update_usrp_rx;
  bool recreate_fs;
  bool reset_fs;
  void update_rx_params();
  ofdmflexframesync fs; // frame synchronizer object
  unsigned int frame_num;
  unsigned int frame_uhd_overflows; // overflows that have occurred
  std::complex<float> *rx_buffer;
  size_t rx_buffer_len; 
  
  // receiver threading objects
  pthread_t rx_process;     // receive thread
  pthread_mutex_t rx_mutex; // receive mutex
  pthread_mutex_t rx_params_mutex;
  pthread_cond_t rx_cond;   // receive condition
  int rx_state;             // is receiver running?
  int rx_worker_state;
  bool rx_thread_running;   // is receiver thread running?
  friend void *ECR_rx_worker(void *);

  // receiver callback
  friend int rxCallback(unsigned char *, int, unsigned char *, unsigned int,
                        int, framesyncstats_s, void *);

  //=================================================================================
  // Private Transmitter Objects
  //=================================================================================

  // transmitter properties/objects
  unsigned int tx_frame_counter;
  timer tx_timer;
  float max_tx_time_ms;
  tx_parameter_s tx_params;
  tx_parameter_s tx_params_updated;
  bool update_tx_flag;
  bool update_usrp_tx;
  bool recreate_fg;
  bool reset_fg;
  void update_tx_params();
  void transmit_frame(unsigned int frame_type,
                      unsigned char *_payload, 
                      unsigned int _payload_len);
  ofdmflexframegen fg;           // frame generator object
  unsigned int fgbuffer_len;     // length of frame generator buffer
  std::complex<float> *fgbuffer; // frame generator output buffer [size:
                                 // numSubcarriers + cp_len x 1]
  unsigned char tx_header[8];    // header container (must have length 8)
  unsigned int frame_counter;
  unsigned int numDataSubcarriers;	
  double tx_data_rate;
  int update_tx_data_rate;
  unsigned int num_tx_frames;

  // transmitter threading objects
  pthread_t tx_process;
  pthread_mutex_t tx_mutex;
  pthread_mutex_t tx_state_mutex;
  pthread_mutex_t tx_params_mutex;
  pthread_cond_t tx_cond;
  bool tx_complete;
  bool tx_thread_running;
  int tx_worker_state;
  int tx_state;
  friend void *ECR_tx_worker(void *);
};

#endif
