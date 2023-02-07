# gracli

Gracli (GRammar ACcess LIbrary) is a library that allows random access on compressed strings.

## Build Instructions

1. Clone the repository using: `git clone --recurse-submodules https://github.com/Skadic/gracli.git`
2. Run the following commands:

```sh
mkdir build
cd build
cmake ..
make malloc_count
make
```

If you use [ninja](https://github.com/ninja-build/ninja) as your CMake backend,
replace the `make` calls with `ninja` respectively.

3. The binary is then in the `build` directory.

## Data Structures

The available data structures/random access methods along with the data formats they work on are listed below.

| $i$ | Data Structure   | File Type |
|-----|------------------|-----------|
| $0$ | `std::string`    | Plaintext |
| $1$ | Naive Grammar    | Grammar   |
| $2$ | Sample Scan 512  | Grammar   |
| $3$ | Sample Scan 6400 | Grammar   |
| $4$ | Sample Scan 25600| Grammar   |
| $5$ | LzEnd            | LzEnd     |
| $6$ | File on Disk     | Plaintext |
| $7$ | Blocktree        | Blocktree |

To see where/how to source these files, see [here](#sourcing-compressed-files).

## Usage

Running `./gracli --help` yields the following help message:

```txt
Usage: gracli [PARAM=VALUE]... [FILE]...

Options for gracli -- Offers various data structures for random access on compressed sequences:
  -S, --source_file       The uncompressed reference file for use with -v (string, default: )
  -d, --data_structure    The Access Data Structure to use. (0 = String, 1 = Naive, 2 = Sampled Scan 512, 3 = Sampled Scan 6400, 4 = Sampled Scan 25600, 5 = LzEnd, 6 = File on Disk, 7 = Block Trees) (non-negative integer, default: 0)
  -f, --file              The compressed input file (string, default: )
  -i, --interactive       Starts interactive mode in which interactive queries can be made using syntax <from>:<to> (flag, default: off)
  -l, --substring_length  Length of the substrings while benchmarking substring queries. (non-negative integer, default: 10)
  -n, --num_queries       Amount of benchmark queries (non-negative integer, default: 100)
  -r, --random_access     Benchmarks runtime of a Grammar's random access queries. Value is the number of queries. (flag, default: off)
  -s, --substring         Benchmarks runtime of a Grammar's substring queries. Value is the number of queries. (flag, default: off)
  -v, --verify            Verifies that the given compressed file reprocudes the same characters as a given (uncompressed) reference file. (flag, default: off)

Options for Application -- Command line parser of oocmd:
  -h, --help  Shows this help. (flag, default: off)
```

The compressed file is supplied via the `-f` parameter.

The data structure to be used is determined using the `-d` parameter.
Refer to [the table above](#data-structures) or the `help` command to get more information about which id corresponds to which data structure.

### Interactive Random Access

To get a repl-like interface to interact with a compressed file, use the `-i` flag.
The following command opens the file `my_file.bt` (a block tree file) in a repl using the block tree data structure (id = `7`). 

```sh
./gracli -d 7 -i -f "my_file.bt"
```

Further information is found in the help command inside the repl:

```
> help
exit/quit => stop interactive mode
bounds => print the bounds of the string
<from>:<to> => access a substring
<index> => access a character
```

### Verification of Compressed files

A a compressed file can be cursorily verified using the `-v` flag 
and by additionally providing a plaintext file via the `-S` parameter, 
whose content should be equivalent to the compressed file's original string.

```sh
./gracli -d 5 -v -f "my_file.lzend" -S "my_file.txt"
```

Which results in such an output:

```txt
Checking Random Access...
[==================================] 100%
Checking substrings...
[==================================] 100%
Verification successful!
```

### Benchmarking Data Structures

The data structures can be benchmarked in terms of speed of their random access and substring queries but also their space usage in RAM.
The number of queries can be set using the `-n` parameter.

#### Random Access

To benchmark random access queries, use the `-r` flag.
For example, this will use the Sampled Scan 512 data structure to run 10000 random access (that is, single character) queries at uniformly random positions.

```sh
./gracli -d 2 -r -f "my_file.rp" -n 10000
```

This will output a result line for use with [sqlplot-tools](https://github.com/bingmann/sqlplot-tools`).
However, data can also be manually extracted. An example result line for the above call looks like this:

```txt
RESULT type=random_access ds=sampled_scan_512 input_file=my_file.rp input_size=1234 num_queries=10000 construction_time=0 decode_space_delta=4321 construction_space_delta=0 decode_time=5 query_time_total=21
```

The format and content of this line are subject to change as there are some unused legacy-fields in here.

#### Substring

Similarly, substring queries use the `-s` flag.
In addition, you supply the desired sunbstring length using the `-l` parameter

This benchmarks substring queries on the C++ `std::string` type (using `std::copy`) with 10000 queries extracting a 100 character-long susbtring each.

```sh
./gracli -d 0 -s -f "my_file.txt" -n 10000 -l 100
```

This results in an example result line like this:

```txt
RESULT type=substring ds=string input_file=my_file.txt input_size=1234 num_queries=10000 substring_length=100 construction_time=0 decode_time=0 decode_space_delta=114466 construction_space_delta=0 query_time_total=65
```

As before, the content of these result lines is subject to change.

## Sourcing Compressed Files

Since gracli does not compress files itself, the compressed files need to be sourced from elsewhere.

### Grammar

RePair-compressed files can be created using [rreader](https://github.com/Skadic/rreader) (RePair).
The file format is described in its repository.

Alternatively, [tudocomp](https://github.com/tudocomp/tudocomp) (`grammar-comp` branch) can be used
to create grammar-compressed files compressed by any grammar compression algorithm supported by tudocomp.
The required coder is the `grammar_tuple` coder, using the `u8` coder as its `terminal_coder`, and the `u32` coder for the other two required coders.

### LzEnd

LzEnd-Compressed files can be generated using [LZ-End Toolkit](https://github.com/Skadic/lz-end-toolkit).

### Blocktree

Blocktree files can be generated using my fork of [MinimalistBlockTrees](https://github.com/Skadic/MinimalistBlockTrees) with added support for faster substring queries.
The blocktree files can be created using the `build_bt` executable.

## Attributions

### LZ-End-Toolkit

We use the [LZ-End Toolkit](https://github.com/dominikkempa/lz-end-toolkit) by Dominik Kempa and Dmitry Kosobolov:
Dominik Kempa and Dmitry Kosolobov: LZ-End Parsing in Compressed Space.
Data Compression Conference (DCC), IEEE, 2017.

### BitMagic

We use the [BitMagic](https://github.com/tlk00/BitMagic) library for succinct bitvectors.
