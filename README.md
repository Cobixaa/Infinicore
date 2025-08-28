## ARCH4 - Trustworthy Low-FLOP Patch System (v4)

ARCH4 is a small, portable framework to manage, route, and safely execute modular "patches" (skills) with:
- pre-install validation and tests
- hierarchical routing (metadata → graph → ANN → selector)
- deterministic aggregation with provenance
- sandboxed execution and signatures (dev-mode default)
- rollout/rollback instrumentation and telemetry

Targets laptops, desktops, and Android Termux (aarch64). No Make/CMake required: build with a single g++ command.

### Quick Start (Linux/Termux)
```bash
# Build (clang++ preferred)
clang++ -std=gnu++17 -O3 -Ofast -flto -march=native -mtune=native -pthread \
  arch4/src/main_demo.cpp \
  arch4/src/module_manager.cpp \
  arch4/src/router.cpp \
  arch4/src/aggregator.cpp \
  arch4/src/sandbox.cpp \
  arch4/src/test_runner.cpp \
  -o arch4_chatbot

# If clang++ is not available, fallback to g++
# g++ -std=gnu++17 -O3 -flto -march=native -mtune=native -pthread \
#   arch4/src/main_demo.cpp arch4/src/module_manager.cpp arch4/src/router.cpp \
#   arch4/src/aggregator.cpp arch4/src/sandbox.cpp arch4/src/test_runner.cpp \
#   -o arch4_chatbot

# Run demo (installs example patches, then interactive chatbot)
./arch4_chatbot
```

Termux notes:
- Ensure `pkg install clang python` is installed.
- The sandbox uses RLIMIT and an alarm backstop; networking is best-effort disabled.

### Layout
```
arch4/
  src/                # C++ core
  sdk/                # Python SDK tools
  examples/patches/   # Example patches with tests
  state/              # Local registry and telemetry (created at runtime)
```

### Tutorial: Build Your Own Patch
1) Scaffold a new patch with the SDK:
```bash
python3 arch4/sdk/packager.py create arch4/examples/patches/my_patch
python3 arch4/sdk/packager.py sign arch4/examples/patches/my_patch --dev
python3 arch4/sdk/packager.py validate arch4/examples/patches/my_patch
```
2) Edit `arch4/examples/patches/my_patch/run.py` to implement your logic. The process receives a single CLI arg: the user query, and must print a result to stdout.
3) Add tests in JSONL form to `tests.jsonl`:
```json
{"input":"ping","expected":"echo:ping"}
```
4) Run the demo. The ModuleManager will auto-install your patch if it has a `manifest.json` and passes tests.

### C++ API Overview
- `ModuleManager` (install/verify/test/enable/disable, execute)
```cpp
arch4::ModuleManager mm("arch4/state");
std::string err;
bool ok = mm.install("arch4/examples/patches/math", err);
auto out = mm.run("math", "2+2", 1200);
```
- `Router` (hierarchical scoring)
```cpp
arch4::Router router({.ann_top_k=50, .final_top_m=2}, mm);
auto candidates = router.route("what is earth?");
```
- `Aggregator` (deterministic consensus + provenance)
```cpp
auto answer = arch4::Aggregator{}.aggregate({{candidates[0].record, "foo"}});
```

### Python SDK
- Create/sign/validate patches:
```bash
python3 arch4/sdk/packager.py create arch4/examples/patches/new_patch
python3 arch4/sdk/packager.py sign arch4/examples/patches/new_patch
python3 arch4/sdk/packager.py validate arch4/examples/patches/new_patch
```

### Production Hardening (roadmap hooks in code)
- Signatures: set `ARCH4_REQUIRE_SIG=1` to enforce `signature.txt` (dev placeholder).
- Rollout/rollback: add metrics loop to auto-disable on regressions.
- Telemetry: write events under `arch4/state/telemetry/` (see `sdk/telemetry.py`).

### Performance Notes
- Router caches concept seeds parsed from tests to reduce per-query overhead.
- Sandbox uses RLIMIT_AS and RLIMIT_CPU with an alarm backstop for wall time.
- Build with `-O3 -Ofast -flto -march=native` for maximum performance on your device.

# Infinicore