#!/usr/bin/env python3
import sys

def calc(q: str) -> str:
    ql = q.strip().lower()
    # Very small safe eval: only + - * / and integers
    allowed = set("0123456789+-*/(). ")
    if all((c in allowed) for c in ql):
        try:
            val = eval(ql, {"__builtins__":{}}, {})
            return str(val)
        except Exception:
            pass
    if ql.startswith("add "):
        try:
            parts = ql.split()
            return str(float(parts[1]) + float(parts[2]))
        except Exception:
            return "error"
    if ql.startswith("multiply "):
        try:
            parts = ql.split()
            return str(float(parts[1]) * float(parts[2]))
        except Exception:
            return "error"
    return "unknown"

if __name__ == "__main__":
    q = sys.argv[1] if len(sys.argv) > 1 else ""
    print(calc(q))

