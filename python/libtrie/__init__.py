#! /usr/bin/env python
# vim: set encoding=utf-8

"""
This module provides access to libtrie shared object. Using it should be faster
than spawning a process and communicating with it.

This Python interface only allows for querying the trie. It is not possible to
create new tries via Python.
"""

from ctypes import cdll, c_char_p, c_void_p, cast
import ctypes.util
import os

from libtrie.config import LIB_PATH

LIBTRIE = cdll.LoadLibrary(LIB_PATH)
LIBTRIE.trie_load.argtypes = [c_char_p]
LIBTRIE.trie_load.restype = c_void_p
LIBTRIE.trie_lookup.argtypes = [c_void_p, c_char_p]
LIBTRIE.trie_lookup.restype = c_void_p
LIBTRIE.trie_get_last_error.restype = c_char_p

LIBC = ctypes.CDLL(ctypes.util.find_library('c'))


class Trie(object):
    """
    Trie class encapsulates the underlying trie structure. It is created from
    file and only provides means to query a key. There are no modifications
    possible.
    """

    def __init__(self, filename, encoding='utf8'):
        """
        Create new trie from given file. The optional `encoding` parameter
        specifies how to encode keys before looking them pu.
        """
        self.encoding = encoding
        self.free_func = LIBTRIE.trie_free
        self.ptr = LIBTRIE.trie_load(filename)
        if not self.ptr:
            err = LIBTRIE.trie_get_last_error()
            raise IOError(str(err))

    def __del__(self):
        if self and self.ptr:
            self.free_func(self.ptr)

    def lookup(self, key):
        """
        Check that `key` is present in the trie. If so, return list of strings
        that are associated with this key. Otherwise return empty list.

        The key should be a unicode object.
        """
        res = LIBTRIE.trie_lookup(self.ptr, key.encode(self.encoding))
        if res:
            result = cast(res, c_char_p).value.decode(self.encoding)
            LIBC.free(res)
            return [s for s in result.split('\n')]
        else:
            return []


def test_main():
    """
    This function creates a storage backed by a file and tests it by retrieving
    a couple of records.
    """
    import sys
    if len(sys.argv) != 2:
        sys.stderr.write('Need one command line argument - trie file\n')
        sys.exit(1)
    t = Trie(sys.argv[1])

    for name in sys.stdin:
        name = name.strip().decode('utf8')
        res = t.lookup(name)
        if res:
            print '\n'.join(res).encode('utf8')
        else:
            print 'Not found'

if __name__ == '__main__':
    test_main()
