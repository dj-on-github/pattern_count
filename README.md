# pattern_count
Counts the frequency of a set of bit patterns up to 12 bits long in binary data, on a sliding window basis.

```usage: pattern_count [-hvr] [-c <chunksize>] <filename>
   Count multiple bit patterns within a binary file.
   -r    --reverse      Reverse the bits in each byte to be big endian.
   -v    --verbose      Print out diagnostics.
   -h    --help         Print out this usage information.
   -c n  --chunksize=n  Set the number of bits in each counted chunk. Default 8192.```

