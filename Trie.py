#! /usr/bin/env python
# vim: set encoding=utf-8

from ctypes import *

libtrie = cdll.LoadLibrary("./libtrie.so")
libtrie.trie_lookup.restype = c_char_p
libtrie.trie_get_last_error.restype = c_char_p


class Trie(object):
    def __init__(self, filename):
        self.ptr = libtrie.trie_load(filename)
        if self.ptr == 0:
            err = libtrie.trie_get_last_error()
            raise IOError(str(err))

    def lookup(self, key):
        res = libtrie.trie_lookup(self.ptr, c_char_p(key))
        if res:
            return [s.decode('utf8') for s in str(res).split('\n')]
        else:
            return []


t = Trie('prijmeni2.trie')
for s in t.lookup('Sedlář'):
    print s
print '%s' % t.lookup('blah')
