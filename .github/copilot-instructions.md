## Key commands & dev workflows
- Build (use helper): **Always** use the fast helper to build: `./acore.sh compiler build`. This is the standard workflow for day-to-day development — do not run manual cmake/make unless you have a special reason.
- Testing by running `./env/dist/bin/worldserver --dry-run` and inspecting the output for errors to check for invalid data, missing scripts, or other issues.
- Cleaning is rarely required; if needed use `./acore.sh compiler clean`.
- VSCode tasks are pre-configured (`.vscode/tasks.json`). Use tasks named `AzerothCore: Build`, `AzerothCore: Run worldserver`, `AzerothCore: Check codestyle cpp/sql`.
- Configuration: the helper reads `conf/config.sh` (absolute path example: `/home/pc/wd/wow/azerothcore-wotlk/conf/config.sh`). Defaults live in `conf/dist/config.sh`. You can also set configuration via environment variables prefixed with `AC_` (e.g., `AC_*`). If `conf/config.sh` is missing, copy `conf/dist/config.sh` to `conf/config.sh` and edit.
- Unit tests: enable with `-DBUILD_TESTING=ON` in CMake, build, then `ctest` or run `./src/test/unit_tests` directly.

## Project-specific patterns & conventions (follow these exactly) 🔧
- Script registration: always add an `AddSC_*()` in the appropriate loader (e.g., `src/server/scripts/<region>/*_script_loader.cpp`). Look at `src/server/scripts/Outland/*` for examples.
- Packet handlers: one per system file under `src/server/game/Handlers/` (e.g., `MovementHandler.cpp`, `SpellHandler.cpp`). These are methods on `WorldSession`.
- SQL update flow: new SQL must go into `data/sql/updates/pending_db_*` until merged; use consistent naming and include them via the `updates_include.sql` mechanism (see `data/sql/base/*/updates_include.sql`).
- Codestyle enforcement: Python scripts in `apps/codestyle/` (`codestyle-cpp.py`, `codestyle-sql.py`) are run in CI — fix issues these report before opening a PR.
- C++ style specifics: 4-space indent, LF, max 120 chars, no tabs, and CI treats warnings as errors. Check `apps/codestyle` and `.github/workflows` for enforced checks.

## CI & PR notes
You are NOT allowed to automaticly open pull requests or merge code. Always ask for human review before opening a PR, and ensure the build passes and codestyle checks are clean before requesting a review.
- Commit messages: include a **Type(Scope/Subscope)** and keep the subject line to a maximum of **72 characters**; follow the formatting and examples in `.git_commit_template.txt`. 

## Quick references (use these as sources of truth)
- Build & helpers: `./acore.sh` (use `./acore.sh compiler build`), `apps/installer/`, `apps/startup-scripts/` (see README inside that dir)
- Scripts examples: `src/server/scripts/Outland/` and `src/server/scripts/*_script_loader.cpp`
- Codestyle checks: `apps/codestyle/codestyle-cpp.py`, `apps/codestyle/codestyle-sql.py`
- MCP tools: use `azerothcore-mcp` (MCP tool) for contextual investigations and the GitHub MCP integration to fetch issue/PR context when available.
- Skills: local agent skills for DB/Wiki/DBC inspection live under
  `.github/skills/azeroth_mcp/SKILL.md` and `.github/skills/wrath_dbcs_sqlite/SKILL.md`.

> Notes for the agent: be conservative with invasive changes to core game logic (e.g., anything in `src/server/game/*`) without tests and a provided in-game reproduction case. Prefer creating small, well-scoped patches with tests and SQL in `pending_db_*` when needed.
