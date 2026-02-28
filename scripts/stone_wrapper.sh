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

# Hack: first we do some cleanup
echo "INFO: Cleaning up any existing mounts on $TARGET_DEV..."
swapoff ${TARGET_DEV}1 2>/dev/null || true
swapoff ${TARGET_DEV}2 2>/dev/null || true
umount ${TARGET_DEV}2 2>/dev/null || true
umount ${TARGET_DEV}1 2>/dev/null || true
systemctl stop udisks2 2>/dev/null || true

echo "INFO: Wiping existing partition table..."
wipefs --all ${TARGET_DEV}
dd if=/dev/zero of=${TARGET_DEV} bs=512 count=2048
partprobe ${TARGET_DEV}
sleep 1



# --- Stub out all GUI functions so stone runs non-interactively ---
# Tchese replace the interactive dialog/text prompts with automatic answers

gui_menu() {
    local id="$1"
    shift 2
    while [ $# -ge 2 ]; do
        label="$1"
        action="$2"
        shift 2
        case "$id" in
            part_mkfs)
                # Auto-select ext4
                case "$label" in
                    *ext4*) eval "$action"; return 0 ;;
                esac ;;
            *)
                case "$label" in
                    *"Start Package Manager"*) eval "$action"; return 0 ;;
                    *"Minimal base"*)          eval "$action"; return 0 ;;
                    *"Install the system"*)    eval "$action"; return 0 ;;
                    *"Erasing all data"*)      eval "$action"; return 0 ;;
                esac ;;
        esac
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

# --- Stub out setup functions with defaults ---
USERNAME="user"
PASSWORD="password"

set_passwd() {
    echo "$1:$PASSWORD" | chpasswd
}

create_user() {
    useradd -m -G audio,input,users,video "$USERNAME" 2>/dev/null || true
    echo "$USERNAME:$PASSWORD" | chpasswd
}

export -f set_passwd create_user 2>/dev/null || true

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

# Hack: shouldnt stone do this??
mkdir -p /mnt/dev /mnt/proc /mnt/sys /mnt/tmp

echo "INFO: Installing packages..."
# stone packages is called inside main() normally, we call it directly
#"$STONE_DIR/stone.sh" packages

mkdir -p /media/cdrom
mount -o ro /dev/sr0 /media/cdrom 2>/dev/null || true

SDECFG_SHORTID=$(grep '^export SDECFG_SHORTID=' /etc/SDE-CONFIG/config 2>/dev/null | cut -f2- -d= | tr -d "'")

if [ -f "/media/cdrom/live.squash" ]; then
    echo "INFO: Live image detected, copying filesystem..."
    mkdir -p /media/live
    mount -o loop /media/cdrom/live.squash /media/live
    rsync -aAX --exclude=/proc --exclude=/sys --exclude=/dev --exclude=/tmp \
        /media/live/ /mnt/
    echo "INFO: Enabling display manager..."
    chroot /mnt systemctl enable plasmalogin
    echo "INFO: Installing bootloader..."
    mount --bind /dev /mnt/dev
    mount --bind /proc /mnt/proc
    mount --bind /sys /mnt/sys
    mount --bind /run /mnt/run
    mount --bind /sys/firmware/efi/efivars /mnt/sys/firmware/efi/efivars 2>/dev/null || true


    # Strip partition number to get the disk
    DISK=$(echo "$TARGET_DEV" | sed 's/[0-9]*$//')

	cat > /mnt/etc/default/grub << 'EOF'
GRUB_DEFAULT=0
GRUB_TIMEOUT=5
GRUB_DISTRIBUTOR="T2 Linux"
GRUB_CMDLINE_LINUX_DEFAULT="quiet"
GRUB_CMDLINE_LINUX=""
EOF

    chroot /mnt mkdir -p /boot/grub2 /boot/grub
    chroot /mnt grub2-install "$DISK"
    chroot /mnt grub-mkconfig -o /boot/grub/grub.cfg
    ln -s /boot/grub/grub.cfg /mnt/boot/grub2/grub.cfg 2>/dev/null || true

    umount /mnt/dev /mnt/proc /mnt/sys /mnt/run /mnt/sys/firmware/efi/efivar 2>/dev/null || true
    umount /media/live
elif [ -d "/media/cdrom/${SDECFG_SHORTID}/pkgs" ]; then
    echo "INFO: Package-based install detected..."
    export PATH="$PATH:$(realpath ../deps/mine-0.23)"
    chmod +x ../deps/mine-0.23/gasgui ../deps/mine-0.23/mine
    startgas() {
        [ -z "$(cd $dir; ls)" ] && mount -o ro $dev $dir
        gasgui -F -c "$SDECFG_SHORTID" -t "/mnt" -d "/dev/sr0" -s "/media/cdrom"
    }
    . "$SETUPD/stone_mod_packages.sh"
    startgas
else
    echo "ERROR: No packages or live.squash found on install media"
    exit 1
fi

echo "INFO: Setting up chroot environment..."
mount -v --bind /dev /mnt/dev

cat > /mnt/tmp/stone_postinst.sh << 'EOF'
#!/bin/bash
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
