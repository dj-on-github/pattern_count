Unless you have the need to efficiently count the frequency of 94 very specific bit patterns between 1 and 12 bits in length, in a binary file, using a sliding window, this might not be the program for you.

If, on the other hand, that is the bag you're into, you might want to marvel at the neat tree construction that enables it to count O(n), rather than O(n^(something bigger than 1 and closer to 2)). Thus on my crusty old Linux box, it does the job in 0.22 seconds for a 1MibiByte file rather than 10 minutes like the python script that preceeded it.

The output is CSV, one line per chunk, with the counts for the 94 patterns in the columns.

I took the arbitary binary symbol size file reading code from djent, which is a much more useful program. I recommend it.

# pattern_count
Counts the frequency of a set of bit patterns up to 12 bits long in binary data, on a sliding window basis.

```
usage: pattern_count [-hvr] [-c <chunksize>] <filename>
   Count multiple bit patterns within a binary file.
   -r    --reverse      Reverse the bits in each byte to be big endian.
   -v    --verbose      Print out diagnostics.
   -h    --help         Print out this usage information.
   -c n  --chunksize=n  Set the number of bits in each counted chunk. Default 8192.
```

