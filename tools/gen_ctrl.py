#!/usr/bin/env python3
"""Generate src/ctrl.c from the Ghidra decompilation of the four player controllers."""
import re, subprocess, sys
sys.path.insert(0,'tools')
import g2c

def body(fn, vmap):
    g2c.v_prefix.clear()
    for k,(t,pre) in vmap.items(): g2c.v_prefix[k]=pre
    txt = subprocess.run(['re/show.py',fn],capture_output=True,text=True).stdout
    txt = g2c.conv(txt, {k:t for k,(t,_) in vmap.items()})
    lines = txt.splitlines()
    # keep from first '{' line after signature to the matching end
    i = next(n for n,l in enumerate(lines) if l.strip()=='{')
    out = []
    for l in lines[i+1:]:
        out.append(l)
    # drop the trailing '}' of the function
    while out and out[-1].strip() in ('}',''): out.pop()
    b = '\n'.join(out)
    b = re.sub(r'^\s*(uVar\d+|iVar\d+|lVar\d+) = \(\(ulong\)param_1 >> 0x10\);\n','',b,flags=re.M)
    b = re.sub(r'^\s*uVar\d+ = \(\(ulong\)lVar\d+ >> 0x10\);\n','',b,flags=re.M)
    b = re.sub(r'^\s*iVar\d+ = \(int\)param_1;\n','',b,flags=re.M)
    b = re.sub(r'^\s*uVar5 = \(\(ulong\)lVar8 >> 0x10\);\n','',b,flags=re.M)
    b = b.replace('iVar4 = (int)lVar8;','')
    b = re.sub(r'FUN_1008_309f\(CONCAT11\([^;]*?,p->spr\)\)','spr_get(p->spr)',b)
    b = re.sub(r'FUN_1008_309f\(CONCAT11\(extraout_AH,p->spr\)\)','spr_get(p->spr)',b)
    b = b.replace('lVar8 == 0','s == NULL').replace('lVar8 != 0','s != NULL')
    b = re.sub(r'\biVar4 = rnd\(','rv = rnd(',b)
    b = re.sub(r'\biVar4 ==','rv ==',b)
    b = re.sub(r'\blVar8 = timer_elapsed\(CONCAT11\([^;]*?,p->timer_([ab])\)\);',r'tm = timer_elapsed(p->timer_\1);',b)
    b = re.sub(r'\blVar8 = timer_elapsed\(p->timer_([ab])\);',r'tm = timer_elapsed(p->timer_\1);',b)
    b = re.sub(r'\blVar8 <','tm <',b)
    b = re.sub(r'\blocal_6\b','ret',b)
    b = re.sub(r'\buVar2\b','in',b)
    b = re.sub(r'\bcVar1\b','anim',b)
    b = re.sub(r'\(int\)\(\(uint\)\(s->w >> 1\) \+ s->x\)','(int)((s->w >> 1) + s->x)',b)
    b = b.replace("'\\x15'","0x15")
    return b

HDR = '''/* Player controllers: given a player, return the next animation: (anim << 8) | flag.
 * Generated from the decompilation of 1000:408b, 486a (human) and 5049, 5a42 (CPU) by tools/gen_ctrl.py,
 * then compiled as is; thresholds are those of the original. Side "down" = near the camera. */
#include "player.h"
#include "game.h"
#include "sprites.h"
#include "platform.h"

'''
funcs = [
 ('ctrl_human_down','1000_408b'), ('ctrl_human_up','1000_486a'),
 ('ctrl_cpu_down','1000_5049'),   ('ctrl_cpu_up','1000_5a42'),
]
out = HDR
for name,fn in funcs:
    # variable roles differ between functions: find the player var (reads +0x19) and the sprite var
    raw = subprocess.run(['re/show.py',fn],capture_output=True,text=True).stdout
    pv = re.search(r'\*\(char \*\)\((iVar\d+) \+ 0x19\)',raw).group(1)
    sv = re.search(r'\*\(int \*\)\((iVar\d+) \+ 4\)',raw).group(1)
    b = body(fn, {pv:('P','p'), sv:('S','s')})
    out += f'uint16_t {name}(TPlayer *p)\n{{\n    uint16_t ret = 0x1800, in = p->input;\n    Sprite *s;\n    uint8_t anim;\n    int rv, iVar3, iVar6, iVar8;\n    int32_t tm;\n    int local_e;\n'
    out += b.replace('{','{',1) + '\n    return ret;\n}\n\n'
open('src/ctrl.c','w').write(out)
print('ok')
