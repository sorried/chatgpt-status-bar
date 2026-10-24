#!/bin/sh
set -eu

if [ "$(uname -s)" != Linux ] || [ "$(uname -m)" != x86_64 ]; then
    printf '%s\n' 'this installer requires Linux x64.' >&2
    exit 1
fi
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    printf '%s\n' 'usage: ./install-linux.sh archive.tar.gz [prefix, default ~/.local]' >&2
    exit 1
fi
archive=$(realpath "$1")
checksum="$archive.sha256"
if [ ! -f "$checksum" ]; then
    printf '%s\n' "checksum file missing: $checksum" >&2
    exit 1
fi
expected=$(awk 'NR == 1 { print $1 }' "$checksum")
actual=$(sha256sum "$archive" | cut -d ' ' -f 1)
if [ "${#expected}" != 64 ] || [ "$expected" != "$actual" ]; then
    printf '%s\n' 'archive checksum mismatch.' >&2
    exit 1
fi
prefix=${2:-"$HOME/.local"}
mkdir -p "$prefix"
prefix=$(realpath "$prefix")
root="$prefix/opt/gpt-status-bar"
launcher="$prefix/bin/gpt-status-bar"
if [ -e "$launcher" ] && [ ! -L "$launcher" ]; then
    printf '%s\n' "refusing to overwrite an existing file: $launcher" >&2
    exit 1
fi
if [ -e "$root/current" ] && [ ! -L "$root/current" ]; then
    printf '%s\n' "refusing to overwrite an existing directory: $root/current" >&2
    exit 1
fi
mkdir -p "$root" "$prefix/bin" "$prefix/share/applications"
stage=$(mktemp -d "$root/.stage.XXXXXX")
trap 'rm -rf "$stage"' EXIT HUP INT TERM
tar --extract --gzip --file "$archive" --directory "$stage" --no-same-owner --no-same-permissions
version=$(cat "$stage/share/gpt-status-bar/version")
if ! printf '%s\n' "$version" | grep -Eq '^[0-9]+[.][0-9]+[.][0-9]+$'; then
    printf '%s\n' 'invalid application version.' >&2
    exit 1
fi
if [ ! -x "$stage/bin/gpt-status-bar" ]; then
    printf '%s\n' 'application executable missing from archive.' >&2
    exit 1
fi
target="$root/$version"
if [ -e "$target" ]; then
    if [ ! -f "$target/.archive-sha256" ] || [ "$(cat "$target/.archive-sha256")" != "$actual" ]; then
        printf '%s\n' "a different build of $version is already installed." >&2
        exit 1
    fi
else
    printf '%s\n' "$actual" > "$stage/.archive-sha256"
    mv "$stage" "$target"
fi
ln -sfn "$target" "$root/current"
ln -sfn "$root/current/bin/gpt-status-bar" "$launcher"
escaped=$(printf '%s' "$root/current/bin/gpt-status-bar" | sed 's/[$`"\\]/\\&/g; s/%/%%/g')
cat > "$prefix/share/applications/io.github.sorried.gpt-status-bar.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=GPT status bar
Comment=Desktop status companion for OpenAI Codex
Exec="$escaped"
Icon=$root/current/share/icons/hicolor/1024x1024/apps/io.github.sorried.gpt-status-bar.png
Categories=Development;
Terminal=false
StartupNotify=true
StartupWMClass=gpt-status-bar
DESKTOP
printf '%s\n' "installed $version: $launcher"
