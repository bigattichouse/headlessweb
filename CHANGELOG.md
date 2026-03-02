# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-03-02

### Added
- **HTTP header capture**: Session now records request/response headers for every navigation via WebKit's resource load callbacks (`Session::addHttpHeader`, `Session::getHttpHeaders`)
- **Header export** (`--export-headers <file>`): Save all captured headers to a JSON file for inspection and sharing
- **Header import** (`--import-headers <file>`): Load headers from a JSON file into the session for replay or assertion testing
- **`--assert-url <expected>`**: Assert the current browser URL matches an expected value (supports operator prefixes)
- **`--assert-title <expected>`**: Assert the current page title matches an expected value (supports operator prefixes)
- **`--assert-response-header <name> <value>`**: Assert a response header from the most recent request matches an expected value
- **`--assert-request-header <name> <value>`**: Assert a request header from the most recent request matches an expected value
- **`--assert-status-code <code>`**: Assert the HTTP status code from the most recent request (supports `>=`, `!=`, etc.)
- `HeaderService`: new service class for header file I/O, validation, filtering, and statistics
- 25 new unit tests for the new assertion types (`tests/assertion/test_new_assertion_types.cpp`)
- Session JSON format bumped to version 4 with `http_headers` array persistence

### Fixed
- **CRITICAL**: `getenv("HOME")` return value was not checked for NULL — would crash if `HOME` is unset
- **CRITICAL build**: `Services/HeaderService.cpp` was missing from `hweb_core` library in `src/hweb/CMakeLists.txt` — production binary would fail to link
- `HttpHeaders` struct fields `statusCode` and `timestamp` were uninitialized (now default to `0`)
- `HeadersStats` struct fields had no default initializers (could expose garbage values)
- `importHeadersFromFile` returned `-1` for valid but empty files; now returns `0` (empty is success)
- `getHeadersFileStats` could read uninitialized `uniqueUrls`/`uniqueDomains` on exception path
- Unused `headerNames` variable removed (was causing compiler warning)
- Whitespace-only token in `matchesFilter` comma-split caused unsigned `npos+1` overflow — replaced with safe trim + `continue`
- `ExportToReadOnlyPath` test was brittle when running as root (Docker CI) — now calls `GTEST_SKIP()`
- `.gitignore` pattern `hweb` was matching the `src/hweb/` source directory — changed to `/hweb` (root-anchored)
- README and docs: `--wait` was described as taking `<milliseconds>` but actually takes a CSS `<selector>`
- README: removed `--wait-selector` (listed but never implemented in Config.cpp)
- README: added `--assert-count`, `--assert-js`, `--verbose`, `--start`, `--allow-data-uri` which were implemented but undocumented
- Removed stale "629/629 tests passing" claims from docs/README.md

### Changed
- `Assertion::Manager::executeAssertion` gained a session-aware overload: `executeAssertion(Browser&, Session&, Command&)` (old overload unchanged — backward compatible)
- `CommandExecutor::execute_assertions` gained a session-aware overload (old overload unchanged)
- `main.cpp` now calls the session-aware `execute_assertions` overload to enable header assertions
- README assertions section updated with all new assertion types and operator prefix documentation
- Session persistence documentation updated to reflect HTTP header storage

### Closes
- [#1 Save Request and Response Headers](https://github.com/bigattichouse/headlessweb/issues/1)

---

## [1.0.0] - Initial Release

- Headless WebKit browser automation via CLI (`hweb`)
- Session-based state persistence (cookies, URL, scroll position, form values)
- DOM assertions: `--assert-text`, `--assert-visible`, `--assert-attribute`, `--assert-count`, `--assert-js`
- Browser commands: `--click`, `--type`, `--wait`, `--screenshot`, `--js`
- File operations: `--upload`, `--download`
- Test suite management with `--begin-suite` / `--end-suite`
- JSON output mode (`--json`) and silent mode (`--silent`)
- Session management: `--session`, `--start`, `--end-session`, `--list-sessions`
- Operator-prefixed expected values: `contains:`, `!=`, `~=`, `>`, `<`, `>=`, `<=`
