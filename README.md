# Cognitive-Radio-Network

This project was developed based on the [GNU Radio](https://github.com/gnuradio/gnuradio) and [CRTS-Cognitive Radio Test System](https://github.com/ericps1/crts). Under CORNET Testbed.

# Installation {#Installation}
## Dependencies {#Dependencies}

- [UHD Version 3.8.4](https://files.ettus.com/manual/page_build_guide.html)
- [liquid-dsp](http://liquidsdr.org/doc/installation/)
- [libconfig-dev](https://packages.ubuntu.com/xenial/libdevel/libconfig-dev)

[FOR DEPENDENCIES REFERENCE SETUP](https://github.com/astro7x/Cognitive-Radio-Network/tree/master/HardwareSetup)

## How to Build
```Shell
1. git clone https://github.com/astro7x/Cognitive-Radio-Network.git
2. cd Cognitive-Radio-Network
3. make
```

## How to Run
### to run the predictive scenario on CORNET platform
1. open a remote connection to port 7032, 7033, 7034 and 7035

```Shell
ssh -v -p 7032 <usr_name>@128.173.221.40 
ssh -v -p 7033 <usr_name>@128.173.221.40 
ssh -v -p 7034 <usr_name>@128.173.221.40 
ssh -v -p 7035 <usr_name>@128.173.221.40 -XC //for Forwarding Display in a compressed mode

```
2. on port 7035 run 
```Shell
python spectrum_analyzer.py
```
3. on port 7032 run 
```Shell
./crts_controller -m
```
4. on port 7033 run 
```Shell
./crts_cognitive_radio -a <controller_ip_address>
```
5. on port 7034 run 
```Shell
./crts_cognitive_radio -a <controller_ip_address>
```

<p align="center">

## [Watch the video] https://youtu.be/QdT6wcxbOYQ For Demonstration :+1:

</p>


#### ------------------------------------------------Project Scenario--------------------------------------------------
<p align="center">
  <img src="https://github.com/astro7x/Cognitive-Radio-Network/blob/master/pics/demo.png?raw=true"/>
</p>


## Primary Users Engines:
### 1. ```CE_PU_MARKOV_Chain_Tx```

This engine is dedicated to switch its operational channel by changing its center frequency/hoppoing between 3 channels in a random maner based on MARKOV chain.
![alt tag](https://github.com/astro7x/Cognitive-Radio-Network/blob/master/pics/CH_States.png?raw=true)
where the PU change the Center Frequency according to the the given probability[1].
[1]

                  |               | CHANNEL 1           | CHANNEL 2          | CHANNEL 3        |
                  |---------------|:-------------------:|:------------------:|:----------------:|
                  |CHANNEL 1      | P(CH_1/CH_1)= 0.1   | P(CH_1/CH_2)=0.3   | P(CH_1/CH_3)=0.6 |
                  |CHANNEL 1      | P(CH_2/CH_1)= 0.1   | P(CH_2/CH_2)=0.5   | P(CH_2/CH_3)=0.4 |
                  |CHANNEL 1      | P(CH_3/CH_1)= 0.1   | P(CH_3/CH_2)=0.2   | P(CH_3/CH_3)=0.7 |             


The implementation uses 2 member function:
``` C++
1. RANDOM_OUTOCME(ECR);   //for generating the random variable from a uniform distribution
2. PU_TX_Behaviour(ECR);        //for selecting the channel according the given probabilities 
```
CE-Document and sample output:
[Primary user Engine.pdf](https://github.com/ericps1/crts/files/1082167/Primary.user.Engine.pdf)

### 2. ```CE_Random_Behaviour_PU```

This engine generates a random channel to operate on it **_[CHANNEL1, CHANNEL2, CHANNEL3]_** without any predetermined probabilities using the standard rand() method for generating random integers and then divides by the maximum number that can be generated which is 3.

## Cognitive/Secondary Users Engines:

### ```CE_Predictive_Node```
This engine is based on Neural Network, it senses 3 channels simultenously at the spectrum Band 800MHz at _**fc**_=833e6 and _**B.W**_=13e6, then pass the measured features to the Neural Network to make a prediction indicates channels status.

## Engine Template:
### ```CE_Template```
This Template compine a brief description of how to construct and build engines based on Exensible Cognitive Radio(ECR) which is provided by [CRTS](https://github.com/ericps1/crts)and [LiquidDSP](https://github.com/jgaeddert/liquid-dsp) library for Digital Signal Processing and specifically OFDM framing.

## ```spectrum_analyzer.py Band 800M```  script:
This is designed to be executed on the USRP node which is responsible for Monitoring the proper RF spectrum.
hint: the compiled version of uhd_fft is generally better 
as it comes with command line arguments option to configure the center freq, bandwidth and gain but we it always hanging and not responding as it launched on WX-GUI so we will develop our own vesion based on QT-GUI.

### DATA SET
Data Set is based on extracting the features from the RF channel, where about 400 examples are produced under prescence of channel usage based on PU activity in the channels.
<p align="center">
  <img src="https://github.com/astro7x/Cognitive-Radio-Network/blob/master/pics/DATA_SET.png?raw=true"/>
</p>

<p align="center">

  <img src="https://github.com/astro7x/Cognitive-Radio-Network/blob/master/pics/ann2.png?raw=true"/>

</p>


| Hardware        | Software And Tools                      | LAB                             |
| --------------- |:---------------------------------------:|:-------------------------------:|
| USRP2           |GNURadio based on python                 |Remotly Access to CORNET Testbed |
| USRP B100       |CRTS based on C++ and Liquid-DSP         |Cairo University Lab             |
|                 |C programming                            |                                 |
|                 |Shell scripting                          |                                 |
  
  
    
## This project is Delievered as a Graduation Thesis to Canadian International College -CIC-
Code is merged to [crts repository](https://github.com/ericps1/crts)


