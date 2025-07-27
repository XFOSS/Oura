#!/usr/bin/env bash
set -euo pipefail

ZIG_VERSION="0.14.1"
ZIG_DIR="zig-x86_64-linux-${ZIG_VERSION}"
ARCHIVE="zig-linux-x86_64-${ZIG_VERSION}.tar.xz"
DOWNLOAD_URL="https://ziglang.org/builds/${ARCHIVE}"

if command -v zig >/dev/null 2>&1; then
    INSTALLED=$(zig version)
    if [ "$INSTALLED" = "$ZIG_VERSION" ]; then
        echo "Zig ${ZIG_VERSION} already installed"
        exit 0
    fi
fi

rm -f "$ARCHIVE"

if [ ! -d "$ZIG_DIR" ]; then
    echo "Downloading Zig ${ZIG_VERSION}..."
    curl -L -o "$ARCHIVE" "$DOWNLOAD_URL"
    mkdir -p "$ZIG_DIR"
    tar -xf "$ARCHIVE" -C "$ZIG_DIR" --strip-components=1
fi

cat <<SH > zig
#!/usr/bin/env bash
"$(pwd)/${ZIG_DIR}/zig" "$@"
SH
chmod +x zig

echo "Zig ${ZIG_VERSION} installed in $(pwd)/${ZIG_DIR}"
