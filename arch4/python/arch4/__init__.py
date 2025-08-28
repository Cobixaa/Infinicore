import ctypes, os

_lib_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..', 'libarch4.so'))
_lib = ctypes.CDLL(_lib_path)

_lib.arch4_manager_create.restype = ctypes.c_void_p
_lib.arch4_manager_create.argtypes = [ctypes.c_char_p]
_lib.arch4_manager_destroy.argtypes = [ctypes.c_void_p]
_lib.arch4_manager_install.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
_lib.arch4_manager_install.restype = ctypes.c_int
_lib.arch4_manager_run.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
_lib.arch4_manager_run.restype = ctypes.c_char_p
_lib.arch4_router_create.argtypes = [ctypes.c_void_p]
_lib.arch4_router_create.restype = ctypes.c_void_p
_lib.arch4_router_destroy.argtypes = [ctypes.c_void_p]
_lib.arch4_route_answer.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_char_p]
_lib.arch4_route_answer.restype = ctypes.c_char_p

class Manager:
    def __init__(self, state_dir: str = 'arch4/state') -> None:
        self._h = _lib.arch4_manager_create(state_dir.encode())
        if not self._h:
            raise RuntimeError('failed to create manager')

    def __del__(self):
        if getattr(self, '_h', None):
            _lib.arch4_manager_destroy(self._h)
            self._h = None

    def install(self, patch_dir: str) -> None:
        buf = ctypes.create_string_buffer(512)
        rc = _lib.arch4_manager_install(self._h, patch_dir.encode(), buf, ctypes.sizeof(buf))
        if rc != 0:
            raise RuntimeError(buf.value.decode() or 'install failed')

    def run(self, patch_id: str, query: str, time_limit_ms: int = 1200) -> str:
        out = _lib.arch4_manager_run(self._h, patch_id.encode(), query.encode(), int(time_limit_ms))
        return out.decode() if out else ''

class Router:
    def __init__(self, manager: Manager) -> None:
        self._h = _lib.arch4_router_create(manager._h)
        if not self._h:
            raise RuntimeError('failed to create router')
        self._m = manager

    def __del__(self):
        if getattr(self, '_h', None):
            _lib.arch4_router_destroy(self._h)
            self._h = None

    def answer(self, query: str) -> str:
        out = _lib.arch4_route_answer(self._h, self._m._h, query.encode())
        return out.decode() if out else ''

__all__ = ['Manager', 'Router']

