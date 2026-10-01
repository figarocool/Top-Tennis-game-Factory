#!/usr/bin/env python3
"""show.py FUN_xxxx [...] : print cleaned decompilation without local declarations / noise"""
import re,sys
txt=open('/home/stefano/Scrivania/sorgenti/topten/re/decomp_clean.c').read()
fs=re.split(r'^// ==== ',txt,flags=re.M)[1:]
F={re.match(r'(\w+) @',b).group(1):b for b in fs}
decl=re.compile(r'^\s+(undefined\d?|byte|uint|int|char|bool|long|ulong|code|ushort|short|float10|float|double)\b[ \*\w\[\],]*;$')
def show(n):
    b=F[n]; out=[]
    for l in b.splitlines():
        if not l.strip(): continue
        if decl.match(l) and '=' not in l: continue
        if 'FUN_1028_05eb();' in l: continue
        l=re.sub(r'\(undefined\d\)','',l)
        l=l.replace('0','0')
        out.append(l)
    print('\n'.join(out)); print()
for n in sys.argv[1:]:
    n=n if n.startswith('FUN') or n=='entry' else 'FUN_'+n
    show(n)
