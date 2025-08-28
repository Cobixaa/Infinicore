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
# Build
g++ -std=gnu++17 -O2 -pthread \
  arch4/src/main_demo.cpp \
  arch4/src/module_manager.cpp \
  arch4/src/router.cpp \
  arch4/src/aggregator.cpp \
  arch4/src/sandbox.cpp \
  arch4/src/test_runner.cpp \
  -o arch4_chatbot

# Run demo (installs example patches, then interactive chatbot)
./arch4_chatbot
```

Termux notes:
- Ensure `pkg install clang python` is installed.
- The sandbox tries to use network namespace; if unavailable, it falls back automatically.

### Layout
```
arch4/
  src/                # C++ core
  sdk/                # Python SDK tools
  examples/patches/   # Example patches with tests
  state/              # Local registry and telemetry (created at runtime)
```

### Python SDK (packager)
```bash
python3 arch4/sdk/packager.py create examples/patches/my_patch
python3 arch4/sdk/packager.py sign examples/patches/my_patch --dev
```

### Security
This MVP enables signatures in "dev mode" (placeholder). For production, enable Ed25519 verification via `libsodium` or an external verifier and set `ARCH4_REQUIRE_SIG=1`.

### Status
MVP: ModuleManager, Sandbox, Router (naive ANN), Aggregator, TestRunner, Python SDK, and interactive demo.

# Infinicore