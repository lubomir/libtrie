# Libtrie

This is an implementation of the prefix tree structure usable for looking up
string keys and associated string data. The main goal of this library is to
provide a mean to query a structure without actually reading all the data in
memory first.

This is achieved by first precomputing the trie from a plain text file. The
created structure is than saved into a binary file on disk. When loading the
file, it is `mmap`ed and lazily read into memory when needed.

Therefore, this library provides an O(1) lookup in terms of number of keys.

The size of the compiled trie on disk varies widely to the original file based
on the actual keys and values. It can be as much as 15 time smaller or 10 times
as large. It is good for the keys to have lots of common prefixes and for
values to be equal.


## Command line utilities

Apart from the shared object, this library provides two command line utilities.
While there is no inherent problem with using the tools with data in any
encoding, it was not tested and probably won't work out of the box. You should
use [UTF-8 everywhere](http://www.utf8everywhere.org/) anyway.

### list-compile

This utility is used to compile the original file into a trie and serialize it.
The input file should be a text file with unix line endings and two columns
delimited by any byte. The delimiter can be specified via `-d` command line
argument. The other available option is `-e`. This signifies that no data will
be associated with the keys. There can still be the delimiter after the key or
even some data. It will be ignored.

Should there be more occurrences of the same key, the data will be concatenated
as lines and stored together.

### list-query

This tool can be used to query the compiled files. It has no command line
options, just give it a file name to work with. It will read keys from standard
input (one on a line) and for each output the associated data.


## Python interface

There is a Python module interface for the library written using the
[ctypes](http://docs.python.org/2/library/ctypes.html) foreign function
library. It exposes a single module `Trie` with one useful method `lookup`.

The constructor of the class expects one positional argument – the filename to
be loaded – and one optional keyword argument `encoding` specifying the
encoding of the file. The default encoding is UTF-8. Should the loading fail,
an `IOError` with detailed description is thrown.

The `lookup` method needs one positional argument – the `unicode` key to be
looked up in the trie. This method returns a set of strings associated with the
key. The set is empty if the key was not present in the trie.


## C API

For details of exported C functions, see the `trie.h` header file.