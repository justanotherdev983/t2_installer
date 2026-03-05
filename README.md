# t2_installer

A graphical installer for [T2 SDE Linux](https://t2linux.com), built with Qt6. Replaces the interactive `stone` TUI with a super simple install. 

## Dependencies

- Qt6
- CMake ≥ 3.16
- C++17

External deps bundled under `deps/`:
- `deps/stone/`: t2 legacy tui installer
- `deps/mine-0.23/`: `gasgui` + `mine` legacy package manager
- `deps/README.md`: provides more information for the dependencies

## Building

```bash
$ cmake -B build
```

## Usage

Boot the T2 live ISO, then:

```bash
$ cd build
$ make
$ chmod +x scripts/stone_wrapper.sh deps/stone/*.sh
# ./t2_installer
```

Select your target drive/device and click **Install**. 

The installer calls `scripts/stone_wrapper.sh <device>` under the hood.

User credentials default to `user`/`password` and can be overridden via environment:

```bash
# USERNAME=<your_name> PASSWORD=<your_new_password> ./t2_installer
```

### Install sources

The wrapper auto-detects the install source from `/media/cdrom`

There are 2 ways we can install to the target drive/device:
- rsync'ing from live.squash squashfs to target (DEFAULT)
- gasgui package install method from STONE to target (WIP)

## Contributing

PRs and contributions in any way/shape/form are always welcome and highly appreciated :). 

## Closing

t2-installer is made
