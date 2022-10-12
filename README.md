# gracli

Gracli (GRammar ACcess LIbrary) is a library that allows random access on grammar-compressed strings.

## Build Instructions

1. Clone the repository using: `git clone --recurse-submodules https://github.com/Skadic/gracli.git`
2. Run the following commands: 
```
mkdir build
cd build
cmake ..
make
```
3. The binary is then in the `build` directory.

## Attributions

### LZ-End-Toolkit

We use the [LZ-End Toolkit](https://github.com/dominikkempa/lz-end-toolkit) by Dominik Kempa and Dmitry Kosobolov:
Dominik Kempa and Dmitry Kosolobov: LZ-End Parsing in Compressed Space. Data Compression Conference (DCC), IEEE, 2017.

### BitMagic

We use the [BitMagic](https://github.com/tlk00/BitMagic) library for succinct bitvectors.
