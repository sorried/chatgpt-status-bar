#!/bin/sh
set -eu
installer=$1
base=$2
mkdir -p "$base"
workspace=$(mktemp -d "$base/case.XXXXXX")
trap 'rm -rf "$workspace"' EXIT HUP INT TERM
bundle="$workspace/bundle"
mkdir -p "$bundle/bin" "$bundle/share/gpt-status-bar"
printf '#!/bin/sh\nexit 0\n' > "$bundle/bin/gpt-status-bar"
chmod +x "$bundle/bin/gpt-status-bar"
printf '1.2.3\n' > "$bundle/share/gpt-status-bar/version"
archive="$workspace/gpt-status-bar-linux-x64.tar.gz"
tar -czf "$archive" -C "$bundle" .
sha256sum "$archive" > "$archive.sha256"
prefix="$workspace/installed app"
sh "$installer" "$archive" "$prefix"
test -x "$prefix/bin/gpt-status-bar"
test -L "$prefix/opt/gpt-status-bar/current"
test -f "$prefix/share/applications/io.github.sorried.gpt-status-bar.desktop"
sh "$installer" "$archive" "$prefix"
if command -v desktop-file-validate >/dev/null 2>&1; then
    desktop-file-validate "$prefix/share/applications/io.github.sorried.gpt-status-bar.desktop"
fi
printf '%064d  archive\n' 0 > "$archive.sha256"
if sh "$installer" "$archive" "$workspace/rejected"; then
    printf '%s\n' 'installer accepted an invalid checksum.' >&2
    exit 1
fi
test ! -e "$workspace/rejected"
sha256sum "$archive" > "$archive.sha256"
mkdir -p "$workspace/conflict/bin"
printf 'keep me\n' > "$workspace/conflict/bin/gpt-status-bar"
if sh "$installer" "$archive" "$workspace/conflict"; then
    printf '%s\n' 'installer overwrote an existing launcher.' >&2
    exit 1
fi
test "$(cat "$workspace/conflict/bin/gpt-status-bar")" = 'keep me'
printf '..\n' > "$bundle/share/gpt-status-bar/version"
tar -czf "$archive" -C "$bundle" .
sha256sum "$archive" > "$archive.sha256"
if sh "$installer" "$archive" "$workspace/invalid-version"; then
    printf '%s\n' 'installer accepted an invalid version.' >&2
    exit 1
fi
