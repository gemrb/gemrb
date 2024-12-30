#! /usr/bin/env python
#coding=utf-8
from base import Base
from struct import unpack, pack
import os
import io
# based on https://gibberlings3.github.io/iesdp/file_formats/ie_formats/tlk_v1.htm

class Tlk(Base, list):
    SIGN = b"TLK V1  "
    def _load(self, io):
        self.language_id, num, offset = unpack("<HII", io.read(0xa))

        for _ in range(num):
            self.append(dict(list(zip(("flag", "sound_name", "volume", "pitch", "offset", "length"), 
                                 unpack("<H 8s 4I", io.read(0x1a))))))

        for t in self:
            io.seek(offset+t["offset"], os.SEEK_SET)
            t["string"] = io.read(t["length"])

    def _save(self, io2):
        offset = len(self)*0x1a + 0x12
        io2.write(pack("<HII", self.language_id, len(self), offset))

        string_io = io.BytesIO()
        for t in self:
            t["length"] = len(t["string"])
            if t["length"] == 0:
                t["offset"] = 0
            else:
                t["offset"] = string_io.tell()

            io2.write(pack("<H 8s 4I", t["flag"], t["sound_name"], t["volume"], t["pitch"], t["offset"], t["length"]))
            string_io.write(t["string"])

        io2.write(string_io.getvalue())

    def __str__(self):
        s = []
        for i, t in enumerate(self):
            s.append("%d %04x %8s %08x %08x %08x %08x %s"%(i, t["flag"], t["sound_name"].strip("\x00"), t["volume"], t["pitch"], t["offset"], t["length"], t["string"]))
        return "\n".join(s)

if __name__ == "__main__":
    import sys
    t = Tlk(open(sys.argv[1], "rb"))
    print(t)
    #t.save(open("1.bin", "wb"))
