#!/usr/bin/env bash
# allow-userns-bwrap.sh — let WebKitGTK's mandatory bubblewrap sandbox run on an
# AppArmor-hardened host, WITHOUT globally disabling the unprivileged-userns restriction.
#
# It installs a scoped AppArmor profile for /usr/bin/bwrap that grants only the `userns`
# permission. This re-enables unprivileged user namespaces for bwrap-mediated sandboxes
# (WebKit, Flatpak) only — it does NOT touch kernel.apparmor_restrict_unprivileged_userns,
# so arbitrary binaries stay restricted. See docs/sandbox-and-userns.md for the trade-offs.
#
# Usage:
#   sudo ./scripts/allow-userns-bwrap.sh            # install + load the profile
#   sudo ./scripts/allow-userns-bwrap.sh --revert   # remove the profile, restore hardening
#
# For a TIGHTER scope (userns for hweb only, not all bwrap users) see Option B in the docs.

set -euo pipefail

PROFILE_PATH=/etc/apparmor.d/bwrap
BWRAP_BIN="${BWRAP_BIN:-$(command -v bwrap || true)}"; [ -n "$BWRAP_BIN" ] || BWRAP_BIN=/usr/bin/bwrap

die() { echo "error: $*" >&2; exit 1; }

[ "$(id -u)" -eq 0 ] || die "must run as root (use: sudo $0 $*)"
command -v apparmor_parser >/dev/null 2>&1 || die "apparmor_parser not found — is AppArmor installed?"
[ -x "$BWRAP_BIN" ] || die "bwrap not found (looked for '$BWRAP_BIN'; set BWRAP_BIN=/path)"

# ── --revert: remove the profile and restore the default (restricted) posture ────────────
if [ "${1:-}" = "--revert" ] || [ "${1:-}" = "--uninstall" ]; then
    if [ -f "$PROFILE_PATH" ]; then
        apparmor_parser -R "$PROFILE_PATH" 2>/dev/null || true   # unload from the kernel
        rm -f "$PROFILE_PATH"
        echo "removed $PROFILE_PATH and unloaded it."
    else
        echo "$PROFILE_PATH not present — nothing to revert."
    fi
    if bwrap --unshare-user --uid 0 -- /bin/true 2>/dev/null; then
        echo "note: bwrap userns still WORKS — the global sysctl may be 0, or another profile grants it."
        echo "      check: sysctl kernel.apparmor_restrict_unprivileged_userns"
    else
        echo "verified: bwrap userns is blocked again (host hardened)."
    fi
    exit 0
fi

# ── install ──────────────────────────────────────────────────────────────────────────────
# The `userns` rule requires AppArmor abi 4.0. Fall back with a clear error if it is absent.
ABI=4.0
[ -f /etc/apparmor.d/abi/4.0 ] || die "AppArmor abi/4.0 not available — this kernel/apparmor is too old for a scoped 'userns' rule; see Option C in docs/sandbox-and-userns.md"

echo "installing scoped userns profile → $PROFILE_PATH"
cat > "$PROFILE_PATH" <<EOF
# Managed by headlessweb/scripts/allow-userns-bwrap.sh
# Grants /usr/bin/bwrap the 'userns' permission so WebKitGTK's mandatory sandbox can start
# on hosts with kernel.apparmor_restrict_unprivileged_userns=1. Scoped to bwrap only.
abi <abi/$ABI>,
include <tunables/global>

profile bwrap $BWRAP_BIN flags=(unconfined) {
  userns,
  include if exists <local/bwrap>
}
EOF

echo "loading profile"
apparmor_parser -r "$PROFILE_PATH"

echo "verifying"
if bwrap --unshare-user --uid 0 -- /bin/true 2>/dev/null; then
    echo "OK — bwrap can now create an unprivileged user namespace."
    echo "     WebKitGTK/hweb should launch. End-to-end check (as your normal user):"
    echo "       DISPLAY=:99 hweb --url about:blank --assert-js \"true\" --silent; echo exit=\$?"
    echo "     To undo: sudo $0 --revert"
else
    die "profile loaded but bwrap still cannot create a userns — check 'journalctl -k | grep apparmor' for denials, or see docs/sandbox-and-userns.md Option B/C."
fi
