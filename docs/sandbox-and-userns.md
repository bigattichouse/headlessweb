# Running hweb in AppArmor-restricted environments (unprivileged user namespaces)

## Symptom

`hweb` aborts before loading any page, with:

```
bwrap: setting up uid map: Permission denied
** (process:NNNN): ERROR **: Failed to fully launch dbus-proxy: Child process exited with code 1
```

and the process **coredumps** (exit code 133 / SIGTRAP). Every `--assert-*` gate
"fails" even on a trivially-correct page such as `about:blank`.

## Root cause

`hweb` is built on **WebKitGTK 6.0** (`webkitgtk-6.0`, e.g. 2.52.x). In the 6.0 API the
bubblewrap **sandbox is mandatory** — the old `webkit_web_context_set_sandbox_enabled()`
switch was removed, so WebKit *always* launches `bwrap` + `dbus-proxy` for its Network/Web
processes. There is **no env var and no API to disable it** (`WEBKIT_FORCE_SANDBOX=0` only
*forces it on*; it cannot turn it off).

`bwrap` needs to create an **unprivileged user namespace** and write `/proc/self/uid_map`.
On recent Ubuntu (23.10+, kernel with the AppArmor userns patches) this is gated by:

```
kernel.apparmor_restrict_unprivileged_userns = 1
```

When this is `1`, an unconfined process that creates a userns is transitioned into the
restrictive `/etc/apparmor.d/unprivileged_userns` profile, which **strips capabilities**
(`audit deny capability`). `bwrap` then can't finish setting up the namespace → the
`uid map: Permission denied` above. The sysctls that *look* permissive
(`unprivileged_userns_clone=1`, a large `max_user_namespaces`) do **not** help — the block
is the AppArmor layer, not the classic userns sysctl.

## Diagnose

```bash
cat /proc/sys/kernel/apparmor_restrict_unprivileged_userns   # 1 = restricted (the culprit)
bwrap --unshare-user --uid 0 -- /bin/true                    # 'Permission denied' = blocked
ls -l "$(command -v bwrap)"                                  # note: NOT setuid → relies on userns
```

## Fix

Three options, **least exposure first**. Pick one; do **not** stack them.

### Option A — scope a profile to `bwrap` only (recommended)

Give `bwrap` its own AppArmor profile that grants `userns`, so it is no longer forced into
the capability-stripped transition profile. This re-enables unprivileged userns **only for
`bwrap`-mediated sandboxes** (WebKit, Flatpak, …), not for arbitrary binaries.

Use the bundled installer (writes `/etc/apparmor.d/bwrap`, loads it, and verifies):

```bash
sudo ./scripts/allow-userns-bwrap.sh            # install
sudo ./scripts/allow-userns-bwrap.sh --revert   # undo, restore hardening
```

Equivalent manual steps if you prefer — create `/etc/apparmor.d/bwrap`:

```
abi <abi/4.0>,
include <tunables/global>

profile bwrap /usr/bin/bwrap flags=(unconfined) {
  userns,
  include if exists <local/bwrap>
}
```

then load it (it auto-loads on boot from `/etc/apparmor.d/`):

```bash
sudo apparmor_parser -r /etc/apparmor.d/bwrap
```

### Option B — scope to hweb specifically (tightest; recommended if you want a true pinhole)

Attach an AppArmor profile to the `hweb` binary and transition its sandbox subtree
(`bwrap` / `xdg-dbus-proxy` / `WebKit*Process`) into a `userns`-granting **child** profile, so
**only bwrap launched inside hweb's process tree** may create a user namespace. A standalone
`bwrap` run by anyone else still hits the host restriction. WebKitGTK 6.0 hardcodes
`/usr/bin/bwrap` (no override path), so this transition chain — not a private bwrap copy — is
the way to scope it.

First install hweb to a **stable path** so the profile attaches to the same binary you run
(the script defaults to an installed hweb, e.g. `/usr/local/bin/hweb`, over a build tree):

```bash
sudo make install   # from the headlessweb root → /usr/local/bin/hweb
```

Then use the bundled installer with its complain → run → enforce tuning flow (the exact process
tree is only observable at runtime):

```bash
sudo ./scripts/allow-userns-hweb.sh --complain     # install, log-only (won't break anything)
DISPLAY=:99 hweb --url about:blank --assert-js true --silent   # run it as your normal user
sudo ./scripts/allow-userns-hweb.sh --logs         # inspect what the sandbox actually did
sudo ./scripts/allow-userns-hweb.sh --enforce      # lock it down
DISPLAY=:99 hweb --url about:blank --assert-js true --silent   # re-verify under enforce
sudo ./scripts/allow-userns-hweb.sh --revert       # undo, restore hardening
```

The installed profile leaves `hweb` itself **unconfined** — it only enforces *where* userns is
available. If you also want to box hweb's filesystem/network so a WebKit breakout can't reach
the rest of the host, that's a further step (turn the `flags=(unconfined)` profile into a real
confined one via a complain-mode data-collection run + `aa-logprof`).

### Option C — global sysctl (broadest exposure; not recommended)

```bash
sudo sysctl -w kernel.apparmor_restrict_unprivileged_userns=0
echo 'kernel.apparmor_restrict_unprivileged_userns=0' | sudo tee /etc/sysctl.d/60-userns.conf
```

This turns the restriction off **for every process on the system**, re-opening the
unprivileged-userns local-privilege-escalation surface globally. Avoid it — especially on any
host that executes untrusted or model-generated commands. Prefer Option A.

## Verify the fix is actually live

`sysctl -w` in one shell, or a `tee` into `/etc/sysctl.d/`, does **not** guarantee the value
changed — always re-read it. After applying a fix:

```bash
# Option A/B: bwrap should now succeed (exit 0, no 'Permission denied')
bwrap --unshare-user --uid 0 -- /bin/true; echo "exit=$?"

# End to end: hweb should load and assert cleanly
DISPLAY=:99 hweb --url about:blank --assert-js "true" --silent; echo "exit=$?"   # exit 0 = fixed
```

(`hweb` still needs a display — start `Xvfb :99` first if you're headless, and export
`XDG_RUNTIME_DIR` / the `WEBKIT_DISABLE_*` vars as in the harness wrapper.)

## Notes

- WebKitGTK 6.0's sandbox cannot be disabled in code — do not spend time looking for a flag.
- **Preflight (implemented):** before starting WebKit, hweb probes whether an unprivileged
  userns + `uid_map` write succeeds. If not, it prints the fix hint above and exits with code
  **3** (distinct from `0` pass / `1` assertion-fail) instead of coredumping — so a caller/CI
  can tell "sandbox unavailable" from "the page is broken." Set `HWEB_SKIP_SANDBOX_PREFLIGHT=1`
  to bypass the probe. Implemented in `src/hweb/main.cpp` (`sandbox_preflight()`).
