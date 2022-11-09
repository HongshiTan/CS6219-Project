# CS6219-Project

Any practical DNA sequencing process can be accurately modelled as random sampling of molecules in a noisy multiset. That means that the order in which molecules (data chunks) are listed in the readout is to a large extent random. In other words, every sequencing run contains a huge amount of randomness associated with the order in which the molecules are sequenced. The large amounts of sequenced data expected to be produced continuously in operation of various DNA storage systems, present an opportunity for a constant and free supply of huge amounts of truly random numbers greatly needed in applications such as cryptography. Your task is to design a true random number generator based on sequenced data originating from DNA data storage. Your generator should seek to maximize the length of the truly random number sequence that can be extracted from a given sequencing run, free of any bias that may exist in the process. (Hint: a recent paper made an observation that a true random number generator can be designed based on random DNA synthesis [3].)


## How to use

To recompile and install TestU01 library, 
```
cd 3rd-party
testu01_install.sh
```