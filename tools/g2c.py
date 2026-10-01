#!/usr/bin/env python3
"""g2c.py FUN_1000_xxxx : turn a Ghidra decompilation into readable draft C.
Replaces raw offset accesses with struct field names (per-variable type map), global DAT_ addresses with
names, and well-known helper calls. Output is a *draft* that is then reviewed by hand."""
import re, sys

PLAYER = {0x15:'input',0x17:'id',0x18:'spr',0x19:'anim',0x1a:'frame',0x1b:'tick',0x1c:'pos',0x1d:'variant',
          0x1e:'cpu',0x1f:'side',0x20:'human',0x21:'timer_a',0x22:'timer_b',0x39:'flag39'}
SPRITE = {2:'x',4:'y',6:'w',7:'h'}
BALL   = {0x16:'x',0x18:'y',0x24:'h',0x26:'amp',0x34:'speed',0x35:'bounces',0x39:'dir',0x3b:'alive'}
GLOBALS = {
 '9b38':'g_h','9b3a':'g_amp','9b3c':'g_x','9b3e':'g_y','9b40':'g_bounce_x','9b42':'g_bounce_y','9b44':'g_first_x1',
 '9b48':'g_x1','9b4a':'g_y1','9b4e':'g_y_pred','9b2c':'g_doubles','9b2d':'g_rally','9b2e':'g_first_serve',
 '9835':'ball.bounces','9838':'ball.past_apex','9839':'ball.dir','983b':'ball.alive','9b66':'g_out_flag',
 '9b67':'g_replay_off','9b68':'g_quit_match','9b64':'g_rec_count','9b62':'g_rec_idx','9b54':'g_frame_timer',
 '9518':'g_court_type',
}
FIELDMAPS = {'P':PLAYER,'S':SPRITE,'B':BALL}

def conv(text, vars_):
    # vars_: {'iVar6':'P', 'iVar4':'S'}
    for v,t in vars_.items():
        m = FIELDMAPS[t]
        def rep(mo):
            ty, off = mo.group(1), int(mo.group(2),0)
            name = m.get(off)
            if name is None: return mo.group(0)
            return f'{v_prefix[v]}->{name}'
        pat = re.compile(r'\*\(\s*(?:byte|char|uint|int|undefined1|undefined2|undefined \*|ushort|short)\s*\*\)\s*\(\s*'+v+r'\s*\+\s*(0x[0-9a-f]+|\d+)\s*\)')
        text = re.sub(pat, lambda mo: f'{v_prefix[v]}->{m.get(int(mo.group(1),0), "off_"+mo.group(1))}', text)
    for a,n in GLOBALS.items():
        text = re.sub(r'DAT_1030_'+a+r'\b', n, text)
    text = text.replace('FUN_1028_1815(','rnd(').replace('FUN_1008_29aa(','timer_elapsed(')
    text = re.sub(r'FUN_1000_33e3\(0x9800,','ball_x_at_y(&ball,',text)
    text = text.replace('FUN_1028_05eb();','')
    return text

v_prefix = {}
if __name__=='__main__':
    name = sys.argv[1]
    src = open('/home/stefano/Scrivania/sorgenti/topten/re/show.py').read()
    import subprocess
    txt = subprocess.run(['/home/stefano/Scrivania/sorgenti/topten/re/show.py',name],capture_output=True,text=True).stdout
    # variable roles: given as pairs var=type=prefix e.g. iVar6=P=p
    for a in sys.argv[2:]:
        v,t,pre = a.split('=')
        v_prefix[v]=pre
    vars_ = {a.split('=')[0]:a.split('=')[1] for a in sys.argv[2:]}
    print(conv(txt, vars_))
