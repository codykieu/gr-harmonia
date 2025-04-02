# gr-harmonia: A GNU Radio module for synchronizing software-defined radios (SDRs) in a distributed network

The following features have been implemented:

- 
- 
- 

**NOTE**: 

## Installation

To install gr-harmonia system-wide, you should first install
[plasma-dsp](https://github.com/ShaneFlandermeyer/plasma-dsp) and [gr-plasma](https://github.com/ShaneFlandermeyer/gr-plasma) using the
instructions in their README. Additional required dependencies should be installed as:


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
