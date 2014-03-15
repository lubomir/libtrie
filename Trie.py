#! /usr/bin/env python
# vim: set encoding=utf-8

"""
This module provides access to LIBTRIE shared object. It should be faster than
spawning a process and communicating with it.
"""

from ctypes import cdll, c_char_p, c_void_p, create_string_buffer, cast
import ctypes.util
import os

LIBPATH = os.path.dirname(os.path.abspath(__file__)) + '/libtrie.so'
LIBTRIE = cdll.LoadLibrary(LIBPATH)
LIBTRIE.trie_load.argtypes = [c_char_p]
LIBTRIE.trie_load.restype = c_void_p
LIBTRIE.trie_lookup.argtypes = [c_void_p, c_char_p]
LIBTRIE.trie_lookup.restype = c_void_p
LIBTRIE.trie_get_last_error.restype = c_char_p

LIBC = ctypes.CDLL(ctypes.util.find_library('c'))


class Trie(object):
    """
    Trie class encapsulates the underlying Trie structure. It is created from
    file and only provides means to query a key. There are no modifications
    possible.
    """

    def __init__(self, filename):
        self.free_func = LIBTRIE.trie_free
        self.ptr = LIBTRIE.trie_load(filename)
        if self.ptr == 0:
            err = LIBTRIE.trie_get_last_error()
            raise IOError(str(err))

    def __del__(self):
        if self:
            self.free_func(self.ptr)

    def lookup(self, key, encoding='utf8'):
        """
        Check that `key` is present in the trie. If so, return list of strings
        that are associated with this key. Otherwise return empty list.
        """
        res = LIBTRIE.trie_lookup(self.ptr, key)
        if res:
            result = cast(res, c_char_p).value
            LIBC.free(res)
            return [s for s in result.decode(encoding).split('\n')]
        else:
            return []


def test_main():
    """
    This function creates a storage backed by a file and tests it by retrieving
    a couple of records.
    """
    import sys
    t = Trie('prijmeni6.trie')

    for name in sys.stdin:
        name = name.strip()
        for s in t.lookup(name):
            print s.encode('utf8')

if __name__ == '__main__':
    test_main()
