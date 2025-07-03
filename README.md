# gr-harmonia: A GNU Radio module for synchronizing software-defined radios (SDRs) in a distributed network

The following features have been implemented:

- PDU Single Tone Waveform Source
- PDU Linear Frequency Modulated Waveform Source
- PDU Peak Frequency Estimator
- PDU Clock Drift Estimator
- PDU Peak Time Estimator
- PDU Clock Bias and Phase Estimator
- PDU Clock Drift, Clock Bias, and Carrier Phase Compensation
- UHD: USRP Block


**NOTE**: 

## Installation

To install gr-harmonia system-wide, you should first install
[plasma-dsp](https://github.com/ShaneFlandermeyer/plasma-dsp) and [gr-plasma](https://github.com/ShaneFlandermeyer/gr-plasma) using the
instructions in their README. 
**NOTE**: The following versions were used in developing this code:
Ubuntu 24.0
UHD 4.7
GNURadio 

Next, run the following from the top-level directory of the module:

```[bash]
mkdir build
cd build
cmake ..
make
sudo make install
sudo ldconfig
```

The module can similarly be uninstalled from the top-level directory as shown below:

```[bash]
cd build
sudo make uninstall
sudo ldconfig
```
