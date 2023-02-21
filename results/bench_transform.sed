# cut off file extension
s/\(input_file=.*\)\.\(seq\|rp\|lzend\|bt\)/\1 filetype=\2/g
s/\(ds=string\|ds=file_access\).*/\0 filetype=plaintext/g

# Separate the block tree parameters from the ds name
s/_arit\([0-9]\+\)/ arity=\1/g
s/_root\([0-9]\+\)/ root_arity=\1/g
s/_leaf\([0-9]\+\)/ leaf_length=\1/g
s/_ps\([0-9]\+\)/ prefix_suffix=\1/g

# rename files
s/dna\.200MB/dna200/g
s/english\.200MB/eng200/g
s/dblp\.xml\.200MB/xml200/g

s/Escherichia_Coli/e\\_coli/g
s/EscherichiaColi/e\\_coli/g

s/einstein\.de\.txt/einstein\\_de/g
s/einstein\.en\.txt/einstein\\_en/g

s/world_leaders/worldleaders/g
