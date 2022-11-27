# True Random Number Generation Using DNA Sequencing

Any practical DNA sequencing process can be accurately modelled as random sampling of molecules in a noisy multiset. That means that the order in which molecules (data chunks) are listed in the readout is to a large extent random. In other words, every sequencing run contains a huge amount of randomness associated with the order in which the molecules are sequenced. The large amounts of sequenced data expected to be produced continuously in operation of various DNA storage systems, present an opportunity for a constant and free supply of huge amounts of truly random numbers greatly needed in applications such as cryptography. Your task is to design a true random number generator based on sequenced data originating from DNA data storage. Your generator should seek to maximize the length of the truly random number sequence that can be extracted from a given sequencing run, free of any bias that may exist in the process. (Hint: a recent paper made an observation that a true random number generator can be designed based on random DNA synthesis [3].)


## Statistics of Edit Distance on Primer

We adopts one nanopore sequencing dataset from Microsoft[1] to evaluate the probability distribution of noise. The edit distance calculation code is referred to [2].

To run the code and generate the figure, please first run the following command to install required packages.
```
pip install latex
pip install SciencePlots
sudo apt install texlive texlive-latex-extra texlive-fonts-recommended dvipng cm-super
```
Then, with the dataset that is already contained in our repository,

```
./edit_distance_sts.py
``` 



## Empirical Randomness Tests

First, you need to generate the mbedtls library:
```
cd 3rd-party/mbedtls-3.2.1
python -m pip install -r scripts/basic.requirements.txt
make -j 
```

Then, to install TestU01 library: 
```
cd 3rd-party
testu01_install.sh
```

To build:

```
mkdir build && cd build
cmake ..

```


If there is no file input, all the test will be conducted using our biased sequence sampler.

To run statistical test:


```
# smallCrush
./rbg_test -v -s
# Crush
./rbg_test -v -m
# bigCrush
./rbg_test -v -b
```



To run test with nanopore dataset:

```
./rbg ../../clustered-nanopore-reads-dataset/Clusters.txt
```





The NIST package contains 15 tests, while TestU01 contains most existing RNGs and statistical tests ("Small Crush” (10 tests), “Crush” (96 tests), and “Big Crush” (106 tests)). Besides, it break limitations other suites suffered, such as the test function parameters are fixed in package, the library directory need to be prefixed before calling tests, the testing integer is required to be in specific length, poor portability, etc


## Reference
[1] Nanopore sequencing dataset https://github.com/microsoft/clustered-nanopore-reads-dataset
[2] Minimum edit distance calculateion https://github.com/HongshiTan/CS6219-Project.git