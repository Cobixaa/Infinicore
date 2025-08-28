#!/usr/bin/env python3
import sys, datetime

def handle(q: str) -> str:
    ql = q.strip().lower()
    now = datetime.datetime.utcnow()
    if "date" in ql and "time" in ql:
        return now.strftime("%Y-%m-%d %H:%M UTC")
    if "date" in ql:
        return now.strftime("%Y-%m-%d")
    if "time" in ql:
        return now.strftime("%H:%M UTC")
    return "unknown"

if __name__ == "__main__":
    q = sys.argv[1] if len(sys.argv) > 1 else ""
    print(handle(q))

