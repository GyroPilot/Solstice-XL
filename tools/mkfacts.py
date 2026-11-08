#!/usr/bin/env python3
"""mkfacts.py: art/facts.txt (one fact per paragraph; # lines are notes) -> src/facts.c (kFacts, kNumFacts), Mac Roman.
   ~/serialenv/bin/python tools/mkfacts.py"""
paras, cur = [], []
for line in open("art/facts.txt", encoding="utf-8"):
    line = line.rstrip("\n")
    if line.startswith("#"): continue
    if line.strip(): cur.append(line.strip())
    elif cur: paras.append(" ".join(cur)); cur = []
if cur: paras.append(" ".join(cur))
def c(s):
    b = s.encode("mac_roman")
    return '"' + "".join(chr(x) if 32 <= x < 127 and chr(x) not in '"\\' else ("\\" + chr(x) if chr(x) in '"\\' else "\\%03o" % x) for x in b) + '"'
with open("src/facts.c", "w") as f:
    f.write("/* facts.c: the fun facts, by tools/mkfacts.py from art/facts.txt (shared with LisaSolstice). */\n")
    f.write("const char *const kFacts[] = {\n")
    for p in paras: f.write("    " + c(p) + ",\n")
    f.write("};\nconst short kNumFacts = %d;\n" % len(paras))
print(len(paras), "facts, longest", max(map(len, paras)), "characters")
