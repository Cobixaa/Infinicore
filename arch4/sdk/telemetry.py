#!/usr/bin/env python3
import os, json, time

def log_event(state_dir: str, kind: str, payload: dict):
    d = os.path.join(state_dir, 'telemetry')
    os.makedirs(d, exist_ok=True)
    rec = {"ts": time.time(), "kind": kind, "payload": payload}
    with open(os.path.join(d, 'events.log'), 'a') as w:
        w.write(json.dumps(rec) + "\n")

