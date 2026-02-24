#!/bin/bash
# T2 SDE Installer - Stone Wrapper
# Called by the Qt installer with the target device as argument
# Usage: stone_wrapper.sh /dev/<drive_name>

set -xe

export platform2="pc"

STONE_DIR="$(dirname "$0")/../deps/stone"
SETUPD="$STONE_DIR"
export SETUPD

# --- Validate arguments ---
if [ -z "$1" ]; then
    echo "ERROR: No target device specified"
    echo "Usage: $0 /dev/<drive_name>"
    exit 1
fi

TARGET_DEV="$1"

if [ ! -b "$TARGET_DEV" ]; then
    echo "ERROR: $TARGET_DEV is not a valid block device"
    exit 1
fi

echo "INFO: Target device: $TARGET_DEV"

# --- Stub out all GUI functions so stone runs non-interactively ---
# Tchese replace the interactive dialog/text prompts with automatic answers

gui_menu() {
    shift 2
    while [ $# -ge 2 ]; do
        label="$1"
        action="$2"
        shift 2
        if [ -n "$action" ]; then
            eval "$action"
            return 0
        fi
    done
}

gui_yesno() {
    echo "AUTO-YES: $1"
    return 0
}

gui_input() {
    # args: prompt default varname
    local prompt="$1"
    local default="$2"
    local varname="$3"
    echo "AUTO-INPUT: $prompt -> $default"
    eval "$varname='$default'"
}

gui_message() {
    echo "INFO: $1"
}

gui_cmd() {
    echo "RUNNING: $1"
    shift
    "$@"
}

gui_edit() {
    echo "SKIP: editing $1 (non-interactive)"
}

export -f gui_menu gui_yesno gui_input gui_message gui_cmd gui_edit 2>/dev/null || true

# --- Load stone install module ---
echo "INFO: Loading stone install module..."
echo "INFO: uname -m = $(uname -m)"
echo "INFO: efi = $([ -e /sys/firmware/efi ] && echo yes || echo no)"
grep '\(platform\|type\)' /proc/cpuinfo | head -5
. "$SETUPD/stone_mod_install.sh"

# --- Run automatic partitioning and install ---
echo "INFO: Starting automatic partition of $TARGET_DEV..."
disk_partition "$TARGET_DEV"

echo "INFO: Checking mounts..."
if ! grep -q " /mnt" /proc/mounts; then
    echo "ERROR: Nothing mounted at /mnt after partitioning"
    exit 1
fi

echo "INFO: Installing packages..."
# stone packages is called inside main() normally, we call it directly
"$STONE_DIR/stone.sh" packages

echo "INFO: Setting up chroot environment..."
mount -v --bind /dev /mnt/dev

cat > /mnt/tmp/stone_postinst.sh << 'EOF'
#!/bin/sh
mount -v /proc
mount -v /sys
. /etc/profile
stone setup
umount -v /dev
umount -v /proc
umount -v /sys
EOF

chmod +x /mnt/tmp/stone_postinst.sh

# Generate /mnt/etc/mtab from current mounts
rm -f /mnt/etc/mtab
sed -n '/ \/mnt[/ ]/s,/mnt/\?,/,p' /proc/mounts > /mnt/etc/mtab

# Copy keymap if set
cp -f /etc/default.keymap /mnt/etc/ 2>/dev/null || true

echo "INFO: Running post-install setup in chroot..."
chroot /mnt /tmp/stone_postinst.sh
rm -f /mnt/tmp/stone_postinst.sh

echo "INFO: Installation complete. You may now reboot."
exit 0
