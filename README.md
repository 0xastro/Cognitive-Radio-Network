# Cognitive-Radio-Network
## This is an (Undergraduate Thesis) 

This project is Developed under the GNU Radio platform and CRTS 'Cognitive Radio Test System project which is funded By Virginia Tech '.
![alt tag](https://github.com/astro7x/Cognitive-Radio-Network/blob/master/proj_scenario_Ver1.png?raw=true)



### Primary Users Engine:
This engine is dedicated to switch its operational channel by changing its center frequency:  fc.
only 3 states have been specified:
switching in a fixed pattern between channel_5, channel_4 and channel_3
or switching between channel_2, channel_1 and channel_0

### Engine Template:
This Template compine a brief description of how to construct and build engines based on Exensible Cognitive Radio(ECR) which is provided by CRTS and LiquidDSP library for OFDM framing.

### FFT Spectrum Band 700M:
This is designed to be executed on the USRP node which is responsible for Monitoring the proper RF spectrum.
hint: the compiled version of uhd_fft is generally better 
as it cones with command line argument option to configure the center freq, bandwidth and gain
##Proposed ANN algorithm
![alt tag](https://github.com/astro7x/Cognitive-Radio-Network/blob/master/ann.png?raw=true)

# TODO:
1. Design our model (MLP-Multi Layer Percetron) which add the cognition feature to the Secondry users.
2. Generate the Data set which include the main features of the Observed RF   channels[Channel_0,Channel_1,Channel_2,Channel_3,Channel_4,Channel_5]. The Meters/features will be [RSSI, EVM, PSD, NOISE FLOOR, BER, PER]
3. Train our Model to to adjust the proper weights.
4. Write the MAKE FILE  
5. Integrate the System where 5 nodes are involved: [Controller, 2 primary users, 2 secondry users, Spectrum analyzer]

| Hardware        | Software And Tools                      | LAB                             |
| --------------- |:---------------------------------------:|:-------------------------------:|
| USRP2           |GNURadio based on python                 |Remotly Access to CORNET Testbed |
| USRP B100       |CRTS based on C++ and Liquid-DSP         |Cairo University Lab             |
|                 |C programming                            |                                 |
|                 |Shell scripting                          |                                 |
  
  
    
##This project is Delievered as a Undergraduate Thesis in Canadian International College -CIC-
#######ADDED
+ Presentation of the First Semester progress
+ Documentation of Project
+ Documentation Report includes the theory of CR.

####For any question, contact me:
* m.rahm7n@gmail.com
* +20 109 111 4065
* [facebook](https://www.facebook.com/mrxastro)
* [LinkidIn](https://eg.linkedin.com/in/mrastro)


#########UPDATE
I may not be able to integrate the Artificial Inteligent algorithm with the cognitive engine of the SU's, But I am going to design a stand alone version AI in C and train it in offline mode based on the trainig data.
