#!/usr/bin/env python3
import sys

FACTS = {
    "earth": "Earth is the third planet from the Sun.",
    "moon": "The Moon is Earth's only natural satellite.",
}

def lookup(q: str) -> str:
    ql = q.strip().lower()
    for k,v in FACTS.items():
        if k in ql:
            return v
    return "unknown"

if __name__ == "__main__":
    q = sys.argv[1] if len(sys.argv) > 1 else ""
    print(lookup(q))

