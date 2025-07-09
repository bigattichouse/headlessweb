# Installation

This document records the packages that have been installed to build and run this project.

## System Packages

*   `libwebkitgtk-6.0-dev` (for WebKitGTK+ 6.0 development)
*   `libjsoncpp-dev` (for JSON serialization)
*   `bubblewrap` (for WebKitGTK+ sandboxing)
*   `xdg-utils` (for D-Bus proxying)

## Python Packages (via pip)

*   `cmake` (for building the project)

## Known Limitations and Troubleshooting

*   **JavaScript Result Retrieval:** Due to complexities and API changes in WebKitGTK+ 6.0, this Proof of Concept (POC) does not currently support direct retrieval of JavaScript execution results. The `executeJavascript` function will run the provided script, but its return value will not be captured or printed to stdout.

*   **`bwrap: setting up uid map: Permission denied` / `Failed to fully launch dbus-proxy`:** This error indicates an issue with WebKitGTK+'s sandboxing mechanism, often related to user namespaces or AppArmor/SELinux policies.
    *   **Ensure User Namespaces are Enabled:** Check `sysctl kernel.unprivileged_userns_clone`. If it's `0`, enable it by adding `kernel.unprivileged_userns_clone=1` to `/etc/sysctl.d/10-userns.conf` and running `sudo sysctl -p /etc/sysctl.d/10-userns.conf`. **Reboot required.** (Caution: Security implications apply).
    *   **AppArmor Configuration:** If user namespaces are enabled and the issue persists, AppArmor might be restricting `bwrap`. Try the following steps:
        1.  Install AppArmor profiles: `sudo apt install apparmor-profiles` (if not sufficient `sudo apt install apparmor-profiles-extra`)
        2.  Link the `bwrap` profile: `sudo ln -s /usr/share/apparmor/extra-profiles/bwrap-userns-restrict /etc/apparmor.d/bwrap`
        3.  Load the profile: `sudo apparmor_parser /etc/apparmor.d/bwrap`

