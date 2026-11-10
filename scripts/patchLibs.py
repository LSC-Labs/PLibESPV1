"""
patchLibs.py - PlatformIO pre script: fixes known bugs in downloaded libraries.

The libraries in .pio/libdeps are downloaded by PlatformIO, so the fixes are
applied before every build. The patch is idempotent (already patched files are
left untouched). If the library changed and the original code is not found,
a warning is printed - check if the bug is fixed upstream.

ESPAsyncTCP 2.0.0 (ESP8266) - AsyncClient::abort() use-after-free
------------------------------------------------------------------
abort() calls tcp_abort(). lwIP calls the error callback synchronously,
AsyncClient::_error() calls the discard callback and the owner (i.e.
ESPAsyncWebServer: request -> _handleDisconnect -> delete request -> delete client)
deletes the AsyncClient. After tcp_abort() returns, abort() still writes to the
deleted object:  _pcb = NULL;  setCloseError(ERR_ABRT);
_pcb is the first member of AsyncClient - in a freed umm_malloc block these are
the free list links. Writing NULL there corrupts the heap => random crashes in
lwIP / the WiFi SDK later (pbuf_free, memcpy to NULL in ieee80211_setup_ratetable, ...).
Triggered by every invalid request (i.e. binary data / NUL byte) to the web server,
for example by network scanners (nmap -sV like service detection).

Fix: clear the members and keep the error tracker alive BEFORE tcp_abort(),
do not touch the object afterwards.
"""
Import("env")  # noqa: F821 - provided by PlatformIO
import os

PATCH_MARKER = "LSC-PATCH: abort use-after-free"

ESPASYNCTCP_ORIGINAL = """  if(_pcb) {
    tcp_abort(_pcb);
    _pcb = NULL;
    setCloseError(ERR_ABRT);
  }
  return;
}"""

ESPASYNCTCP_PATCHED = """  // """ + PATCH_MARKER + """ (scripts/patchLibs.py):
  // tcp_abort() calls the error callback synchronously - the owner may delete
  // this object there. So clear the members first and do not touch them afterwards.
  if(_pcb) {
    tcp_pcb *pcb = _pcb;
    _pcb = NULL;
    std::shared_ptr<ACErrorTracker> errorTracker = _errorTracker;   // keeps the tracker alive
    errorTracker->setCloseError(ERR_ABRT);
    tcp_abort(pcb);       // may delete this !
  }
  return;
}"""


def patch_file(path, original, patched, name):
    if not os.path.isfile(path):
        print("patchLibs: %s not found (%s) - skipped" % (name, path))
        return
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()
    if PATCH_MARKER in content:
        print("patchLibs: %s already patched" % name)
        return
    if original not in content:
        print("patchLibs: WARNING - %s: original code not found, NOT patched. "
              "Check if the library changed / the bug is fixed upstream." % name)
        return
    with open(path, "w", encoding="utf-8") as f:
        f.write(content.replace(original, patched, 1))
    print("patchLibs: %s patched" % name)


libdeps_dir = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"))  # noqa: F821
patch_file(os.path.join(libdeps_dir, "ESPAsyncTCP", "src", "ESPAsyncTCP.cpp"),
           ESPASYNCTCP_ORIGINAL, ESPASYNCTCP_PATCHED,
           "ESPAsyncTCP AsyncClient::abort()")
