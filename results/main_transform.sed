s/ds=blocktree,.*arity=\([0-9]\+\),.*prefix.*=\([0-9]\+\)/A\1PS\2/g
s/ds=blocktree,.*arity=\([0-9]\+\)/A\1PS\*/g

s/ds=lzend.*}/LzEnd}/g
s/ds=sampled.*filetype=rp.*}/Sampled Scan (RePair)}/g
s/ds=sampled.*filetype=seq.*}/Sampled Scan (Sequitur)}/g
s/ds=string.*}/\\texttt{std::string}}/g
