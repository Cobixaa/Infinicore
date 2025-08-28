#!/usr/bin/env python3
import argparse, json, os, hashlib, sys, subprocess

TEMPLATE_MANIFEST = {
    "id": "my_patch",
    "name": "My Patch",
    "version": "0.1.0",
    "author": "you",
    "tags": ["general"],
    "entry_point": "run.py",
    "tests": "tests.jsonl",
    "trust_score": 0.5
}

TEMPLATE_RUN = """#!/usr/bin/env python3
import sys
def main(q: str) -> str:
    return "echo:" + q
if __name__ == "__main__":
    q = sys.argv[1] if len(sys.argv)>1 else ""
    print(main(q))
"""

def sha256_dir(path: str) -> str:
    h = hashlib.sha256()
    for root,_,files in os.walk(path):
        for f in sorted(files):
            fp = os.path.join(root, f)
            if f == 'signature.txt':
                continue
            h.update(f.encode())
            with open(fp,'rb') as r:
                h.update(r.read())
    return h.hexdigest()

def cmd_create(args):
    out = args.path
    os.makedirs(out, exist_ok=True)
    man = TEMPLATE_MANIFEST.copy()
    man["id"] = os.path.basename(out)
    with open(os.path.join(out, 'manifest.json'),'w') as w:
        json.dump(man, w, indent=2)
    with open(os.path.join(out, 'run.py'),'w') as w:
        w.write(TEMPLATE_RUN)
    with open(os.path.join(out, 'tests.jsonl'),'w') as w:
        w.write('{"input":"ping","expected":"echo:ping"}\n')
    print(f"Created patch at {out}")

def cmd_sign(args):
    path = args.path
    digest = sha256_dir(path)
    with open(os.path.join(path,'signature.txt'),'w') as w:
        w.write(digest + "\n")
    print("Signed with SHA256 (dev):", digest)

def cmd_validate(args):
    path = args.path
    man = json.load(open(os.path.join(path,'manifest.json')))
    tests = [json.loads(l) for l in open(os.path.join(path, man.get('tests','tests.jsonl')))]
    total = 0; passed = 0
    for t in tests:
        total += 1
        q = t.get('input','')
        exp = (t.get('expected','') or '').strip().lower()
        # Execute entry
        entry = os.path.join(path, man['entry_point'])
        proc = subprocess.run(['python3', entry, q], capture_output=True, text=True)
        out = (proc.stdout or '').strip().lower()
        if exp == "":
            # wildcard: just expect a non-empty answer
            if out:
                passed += 1
        elif out == exp:
            passed += 1
    rate = passed/total if total else 0
    print(f"Passed {passed}/{total} ({rate:.2%})")
    sys.exit(0 if rate >= 0.95 else 1)

def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest='cmd', required=True)
    c = sub.add_parser('create'); c.add_argument('path'); c.set_defaults(func=cmd_create)
    s = sub.add_parser('sign'); s.add_argument('path'); s.set_defaults(func=cmd_sign)
    v = sub.add_parser('validate'); v.add_argument('path'); v.set_defaults(func=cmd_validate)
    args = ap.parse_args()
    args.func(args)

if __name__ == '__main__':
    main()

