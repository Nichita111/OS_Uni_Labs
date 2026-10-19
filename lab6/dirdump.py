#!/usr/bin/env python3
# dirdump.py fs.img INUM - print the raw directory entries of xv6 inode INUM
import struct, sys
img, inum = open(sys.argv[1], 'rb').read(), int(sys.argv[2])
B = 1024
magic, size, nblocks, ninodes, nlog, logstart, inodestart, bmapstart = struct.unpack('<8I', img[B:B+32])
off = inodestart*B + inum*64                      # struct dinode is 64 bytes
typ, major, minor, nlink, fsize = struct.unpack('<hhhhI', img[off:off+12])
addrs = struct.unpack('<13I', img[off+12:off+64])
print(f"inode {inum}: type {typ}, nlink {nlink}, size {fsize}, addrs[0] = block {addrs[0]}")
data = b''.join(img[a*B:(a+1)*B] for a in addrs[:12] if a)[:fsize]
for i in range(0, fsize, 16):                     # struct dirent is 16 bytes
    e = data[i:i+16]
    n = struct.unpack('<H', e[:2])[0]
    if n: print(f"  {e.hex(' ')}   inum={n:<3} name={e[2:].rstrip(bytes(1)).decode()!r}")
