#! /usr/bin/env python
#coding=utf-8
import os

class Base:
    SIGN = ""
    def __init__(self, io=None):
        if io:
            self.load(io)

    def load(self, io):
        if io.read(len(self.SIGN)) != self.SIGN:
            raise TypeError

        self._load(io)

    def _load(self, io):
        raise NotImplementedError

    def save(self, io):
        io.write(self.SIGN)
        self._save(io)

    def _save(self, io):
        raise NotImplementedError

def BaseFactory(io, class_list):
    pos = io.tell()
    for c in class_list:
        sign = io.read(len(c.SIGN))
        io.seek(pos, os.SEEK_SET)
        if sign == c.SIGN:
            return c(io)