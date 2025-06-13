# Module Overview

The modular build located in `ouro_mod/` demonstrates the use of
C++23 modules.  Each module is kept intentionally small so that the
build system remains portable:

* `ouro.foundation.lexer` – tokenises source code for the demo
* `ouro.essentials.environment` – minimal runtime environment helpers

When running `zig build mod-run` the modules are compiled then linked
into a single executable.  The same modules are reused by the small
test suite invoked via `zig build mod-test`.
