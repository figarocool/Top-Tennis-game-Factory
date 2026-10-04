#!/usr/bin/env python3
"""Regenerate re/decomp_clean.c from re/decomp.c + re/strings.txt"""
import re
strs={}
for l in open('re/strings.txt'):
    p=l.rstrip('\n').split('\t')
    if len(p)>=2:
        seg,off=p[0].split(':'); strs[int(off,16)]=p[1]
t=open('re/decomp.c').read()
t=re.sub(r',?\s*\(char \*\)s_RANKING__1030_102c \+ 4','',t)          # DS selector noise
def rep(m):
    base=int(m.group(2),16); add=m.group(3)
    if add is not None:                      # symbol+addend => integer constant, not a string
        return hex(base+int(add,0))
    s=strs.get(base)
    return s if s else m.group(0)
# an exact string symbol used as the right-hand side of an assignment is really an integer constant
t=re.sub(r'=\s*\(char \*\)s_\w+?_1030_([0-9a-f]+);',lambda m:'= 0x'+m.group(1)+';',t)
t=re.sub(r'\(char \*\)(s_\w+?)_1030_([0-9a-f]+)(?: \+ (0x[0-9a-f]+|\d+))?',rep,t)
t=re.sub(r'\(char \*\)(0x[0-9a-f]+)',r'\1',t)
t=t.replace('unaff_SS,','')
open('re/decomp_clean.c','w').write(t)
