#! /usr/bin/env python
# vim: set encoding=utf-8

from ctypes import *
import copy

libtrie = cdll.LoadLibrary("./libtrie.so")
libtrie.trie_load.argtypes = [c_char_p]
libtrie.trie_load.restype = c_void_p
libtrie.trie_lookup.argtypes = [ c_void_p, c_char_p, c_char_p ]
libtrie.trie_lookup.restype = c_void_p
libtrie.trie_get_last_error.restype = c_char_p


class Trie(object):
    def __init__(self, filename):
        self.free_func = libtrie.trie_free
        self.ptr = libtrie.trie_load(filename)
        if self.ptr == 0:
            err = libtrie.trie_get_last_error()
            raise IOError(str(err))

    def __del__(self):
        if self:
            self.free_func(self.ptr)

    def lookup(self, key):
        s = create_string_buffer('\000' * 256)
        res = libtrie.trie_lookup(self.ptr, key, s)
        if res:
            return [s.decode('utf8') for s in s.value.split('\n')]
        else:
            return []


def test_main():
    """
    This function creates a storage backed by a file and tests it by retrieving
    a couple of records.
    """
    import sys
    t = Trie('prijmeni5.trie')

    for name in sys.stdin.readlines():
        name = name.strip()
        for s in t.lookup(name):
            print s

if __name__ == '__main__':
    test_main()
