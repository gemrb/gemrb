#! /usr/bin/env python
#coding=utf-8
from tlk import Tlk

PUNCTUATIONS = u"，。！“”－…,.!"

def insert_space(utf16_str, interval=1, codec = None):
    if codec:
        utf16_str = utf16_str.decode(codec)

    utf16_str = utf16_str.replace(u" ", u"　")
    words = []
    word = u""
    for i, u in enumerate(utf16_str):
        word += u
        if ord(u) > 0x100 \
           and len(word) >= interval \
           and (i+1 < len(utf16_str) and utf16_str[i+1] not in PUNCTUATIONS):
            words.append(word)
            word = u""
    if len(word) > 0:
        words.append(word)
    s = u" ".join(words)
    if codec:
        s = s.encode(codec)
    return s

def convert_to_utf8(tlk_name, codec = "GBK", need_space = True):
    tlk = Tlk(open(tlk_name, "rb"))
    tlk.save(open(tlk_name+".bak", "wb"))
    for i, t in enumerate(tlk):
        try:
            txt = t["string"].decode(codec).encode("utf-8")
        except:
            print "Warning: ", i
            continue
        if need_space:
            txt = insert_space(txt, codec="utf-8")
        t["string"] = txt
    tlk.save(open(tlk_name, "wb"))

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()

    parser.add_argument("name", action="store", nargs = 1)
    parser.add_argument("codec", action="store", nargs = "?")
    parser.add_argument("--disable_space", action="store_true", default = False)

    args = parser.parse_args()
    codec = args.codec
    if not codec:
        codec = "GBK"

    convert_to_utf8(args.name[0], codec, not args.disable_space)

