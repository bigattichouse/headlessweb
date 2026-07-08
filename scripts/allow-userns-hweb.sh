#!/usr/bin/env bash
# allow-userns-hweb.sh — grant unprivileged user-namespace access to WebKitGTK's mandatory
# sandbox for HWEB ONLY, on an AppArmor-hardened host, WITHOUT opening it for other bwrap users.
#
# WebKitGTK 6.0 hardcodes /usr/bin/bwrap and cannot disable its sandbox. A blanket bwrap
# profile (see allow-userns-bwrap.sh) would grant userns to EVERY bwrap caller. This script
# instead attaches an AppArmor profile to the hweb executable and transitions its sandbox
# subtree (bwrap / xdg-dbus-proxy / WebKit*Process) into a userns-granting CHILD profile.
# Result: only bwrap launched inside hweb's process tree may create a userns; a standalone
# `bwrap` run by anyone else still hits the host restriction. See docs/sandbox-and-userns.md.
#
# Because the exact process tree is only observable at runtime, use the tuning flow:
#
#   sudo ./scripts/allow-userns-hweb.sh --complain     # 1. install, log-only (won't break)
#   DISPLAY=:99 hweb --url about:blank --assert-js true --silent   # 2. run it (as your user)
#   sudo ./scripts/allow-userns-hweb.sh --logs         # 3. see what it actually touched
#   sudo ./scripts/allow-userns-hweb.sh --enforce      # 4. lock it down
#   DISPLAY=:99 hweb --url about:blank --assert-js true --silent   # 5. re-verify under enforce
#
#   sudo ./scripts/allow-userns-hweb.sh --revert       # remove, restore hardening
#
# The hweb binary path is auto-detected next to this script; override with HWEB_BIN=/path.

set -euo pipefail

PROFILE_PATH=/etc/apparmor.d/hweb
die() { echo "error: $*" >&2; exit 1; }

# Informational sanity check: the profile is path-independent (a blanket cx -> sandbox covers
# whatever hweb execs), so this only warns; it never fails the install. Overridable via env.
detect_paths() {
    BWRAP_BIN="${BWRAP_BIN:-$(command -v bwrap || true)}"
    DBUS_PROXY_BIN="${DBUS_PROXY_BIN:-$(command -v xdg-dbus-proxy || true)}"
    if [ -z "${WEBKIT_LIBEXEC:-}" ]; then
        for d in /usr/lib/*/webkitgtk-6.0 /usr/libexec/webkitgtk-6.0 /usr/lib/webkitgtk-6.0 /usr/lib64/webkitgtk-6.0; do
            [ -d "$d" ] && { WEBKIT_LIBEXEC="$d"; break; }
        done
    fi
    [ -n "${BWRAP_BIN:-}" ]   || echo "  note: bwrap not on PATH — WebKit hardcodes /usr/bin/bwrap; make sure it exists"
    [ -n "${WEBKIT_LIBEXEC:-}" ] || echo "  note: webkitgtk-6.0 libexec dir not found — is WebKitGTK 6.0 installed?"
}

[ "$(id -u)" -eq 0 ] || die "must run as root (use: sudo $0 $*)"
command -v apparmor_parser >/dev/null 2>&1 || die "apparmor_parser not found — is AppArmor installed?"

# Resolve the hweb binary the profile will attach to (AppArmor keys on the absolute path).
# Prefer an INSTALLED hweb (stable path from `make install`, e.g. /usr/local/bin/hweb) so the
# profile attaches to the same binary you actually run; fall back to the build tree.
if [ -z "${HWEB_BIN:-}" ]; then
    HWEB_BIN="$(command -v hweb 2>/dev/null || true)"
    [ -n "$HWEB_BIN" ] || HWEB_BIN="$(dirname "$0")/../hweb"
fi
HWEB_BIN="$(realpath -m "$HWEB_BIN")"

action="${1:-install}"

# ── --revert ─────────────────────────────────────────────────────────────────────────────
if [ "$action" = "--revert" ] || [ "$action" = "--uninstall" ]; then
    if [ -f "$PROFILE_PATH" ]; then
        apparmor_parser -R "$PROFILE_PATH" 2>/dev/null || true
        rm -f "$PROFILE_PATH"
        echo "removed $PROFILE_PATH and unloaded it."
    else
        echo "$PROFILE_PATH not present — nothing to revert."
    fi
    exit 0
fi

# ── --logs: recent AppArmor denials relevant to this profile ───────────────────────────────
if [ "$action" = "--logs" ]; then
    echo "recent AppArmor messages for hweb / bwrap / webkit (last 200 kernel lines):"
    { journalctl -k -n 200 --no-pager 2>/dev/null || dmesg 2>/dev/null | tail -200; } \
        | grep -iE "apparmor|bwrap|webkit|userns|DENIED" | grep -iE "hweb|bwrap|webkit|userns" || echo "  (no matching messages)"
    exit 0
fi

# ── --enforce: reload the existing profile in enforce mode ─────────────────────────────────
if [ "$action" = "--enforce" ]; then
    [ -f "$PROFILE_PATH" ] || die "no profile at $PROFILE_PATH — run install/--complain first"
    apparmor_parser -r "$PROFILE_PATH"
    echo "enforced $PROFILE_PATH"
    aa-status 2>/dev/null | grep -E "hweb" || true
    exit 0
fi

# ── install (default) / --complain: write the profile, then load ───────────────────────────
mode_complain=0
[ "$action" = "--complain" ] && mode_complain=1
[ "$action" = "install" ] || [ "$action" = "--complain" ] || die "unknown action '$action' (use: install | --complain | --enforce | --logs | --revert)"

[ -f /etc/apparmor.d/abi/4.0 ] || die "AppArmor abi/4.0 unavailable — kernel/apparmor too old for a scoped 'userns' rule"
[ -x "$HWEB_BIN" ] || die "hweb binary not found/executable at '$HWEB_BIN' (set HWEB_BIN=/path/to/hweb)"
detect_paths

echo "attaching profile to: $HWEB_BIN"
echo "  bwrap=${BWRAP_BIN:-(not found)}  dbus-proxy=${DBUS_PROXY_BIN:-(not found)}  webkit-libexec=${WEBKIT_LIBEXEC:-(not found)}"
cat > "$PROFILE_PATH" <<EOF
# Managed by headlessweb/scripts/allow-userns-hweb.sh
# Grants unprivileged user namespaces to WebKitGTK's sandbox for HWEB ONLY. Both profiles ALLOW
# everything (hweb is not otherwise confined — Tier 1); the only thing enforced is that userns
# lives inside hweb's WebKit subtree and NOT in a standalone bwrap. See docs/sandbox-and-userns.md.
#
# NOTE: these are mediating (allow-all) profiles, NOT flags=(unconfined). flags=(unconfined)
# grants userns to the SAME binary but does not fire cx exec-transitions onto children — and
# WebKit's namespace is created by a SEPARATE binary (bwrap), so bwrap would fall through to the
# kernel's restrictive 'unprivileged_userns' profile (caps stripped → uid_map write denied).
abi <abi/4.0>,
include <tunables/global>

profile hweb $HWEB_BIN flags=(attach_disconnected) {
  capability,
  network,
  signal,
  ptrace,
  dbus,
  unix,
  mqueue,
  mount, umount, pivot_root,
  userns,
  file rwlkm /{,**},

  # Everything hweb execs (bwrap, xdg-dbus-proxy, the WebKit*Process helpers, and whatever they
  # launch) transitions into the 'sandbox' child, which also grants userns + caps. So bwrap runs
  # confined by hweb//sandbox instead of falling through to the kernel's restrictive
  # 'unprivileged_userns' profile — and a standalone bwrap (not launched by hweb) still does.
  /{,**} cx -> sandbox,

  profile sandbox flags=(attach_disconnected) {
    capability,
    network,
    signal,
    ptrace,
    dbus,
    unix,
    mqueue,
    mount, umount, pivot_root,
    userns,
    file rwlkm /{,**},
    /{,**} ix,                       # bwrap re-execs / sandboxed content stay userns-capable
    include if exists <local/hweb-sandbox>
  }

  include if exists <local/hweb>
}
EOF

if [ "$mode_complain" -eq 1 ]; then
    echo "loading in COMPLAIN mode (log-only; nothing is blocked)"
    apparmor_parser -r -C "$PROFILE_PATH"
else
    echo "loading in ENFORCE mode"
    apparmor_parser -r "$PROFILE_PATH"
fi

echo "loaded profile 'hweb':"
aa-status 2>/dev/null | grep -E "hweb" || echo "  (aa-status unavailable; profile written to $PROFILE_PATH)"

# Confirm we did NOT leak the grant: a standalone bwrap by the invoking user must still fail.
if [ -n "${SUDO_USER:-}" ]; then
    if sudo -u "$SUDO_USER" bwrap --unshare-user --uid 0 -- /bin/true 2>/dev/null; then
        echo "WARNING: standalone bwrap (as $SUDO_USER) now works — the pinhole is wider than intended."
        echo "         check for a global sysctl=0 or an /etc/apparmor.d/bwrap profile."
    else
        echo "good: standalone bwrap (as $SUDO_USER) is still blocked — grant is scoped to hweb's tree."
    fi
fi

cat <<EOF

next:
  1. run hweb as your normal user (needs a display):
       DISPLAY=:99 XDG_RUNTIME_DIR=/tmp/headless_runtime \\
         $HWEB_BIN --url about:blank --assert-js "true" --silent; echo exit=\$?
     exit 0 = the web-only pinhole works.
  2. see what it touched:   sudo $0 --logs
$( [ "$mode_complain" -eq 1 ] && echo "  3. lock it down:          sudo $0 --enforce   (then re-run step 1)" )
  undo anytime:             sudo $0 --revert
EOF
