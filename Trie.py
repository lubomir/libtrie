#! /usr/bin/env python
# vim: set encoding=utf-8

from ctypes import *
import copy

libtrie = cdll.LoadLibrary("./libtrie.so")
libtrie.trie_load.argtypes = [c_char_p]
libtrie.trie_load.restype = c_void_p
libtrie.trie_lookup.argtypes = [ c_void_p, c_char_p ]
libtrie.trie_lookup.restype = c_void_p
libtrie.trie_get_last_error.restype = c_char_p
libtrie.trie_free_data.argtypes = [ c_void_p ]


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
        res = libtrie.trie_lookup(self.ptr, key)
        if res:
            data = copy.deepcopy(c_char_p(res).value)
            libtrie.trie_free_data(res)
            return [s.decode('utf8') for s in data.split('\n')]
        else:
            return []


t = Trie('prijmeni5.trie')
for s in t.lookup('Sedlář'):
    print s
print '%s' % t.lookup('blah')
