# Pluck-GTK

A minimal, keyboard-driven **file-search overlay** for Wayland desktops.
Pluck pops up a floating search bar, pipes your query through
[`fd`](https://github.com/sharkdp/fd) and [`fzf`](https://github.com/junegunn/fzf),
and opens the selected file's containing folder in your system file manager.

Built with **GTK 4** and **gtk4-layer-shell** so it floats above every other
window — bind it to a hotkey and it feels like a native launcher.

<img width="1366" height="768" alt="image" src="https://github.com/user-attachments/assets/26d037f0-a2d4-4108-be84-fb47e899d23b" />

---

## Features

- Fuzzy file search powered by `fd` + `fzf`
- Matching characters highlighted in results (exact-run first, fuzzy fallback)
- Results truncated in the middle so long paths stay readable
- Press **Enter** or click a result to open its folder in the file manager
- Press **Escape** to dismiss
- Layer-shell namespace `pluck-gtk` — easy to target in compositor rules

---

## Requirements

### Runtime

| Dependency | Why |
|---|---|
| Wayland compositor with [wlr-layer-shell](https://wayland.app/protocols/wlr-layer-shell-unstable-v1) support (e.g. Sway, Hyprland, river, niri) | Required for the overlay window |
| [`fd`](https://github.com/sharkdp/fd) | Fast file enumeration |
| [`fzf`](https://github.com/junegunn/fzf) | Fuzzy ranking of results |
| GTK 4 runtime (`libgtk-4-1`) | UI toolkit |
| gtk4-layer-shell runtime (`libgtk4-layer-shell`) | Layer-shell protocol support |

Install runtime deps on Ubuntu/Debian:

```bash
sudo apt-get install fd-find fzf libgtk-4-1
# gtk4-layer-shell runtime is installed alongside the dev library (see below)
```

> **Note:** On Debian/Ubuntu `fd` installs as `fdfind`. Either create a symlink
> (`sudo ln -s $(which fdfind) /usr/local/bin/fd`) or adjust the `fd` call in
> `src/ui.c` to `fdfind`.

### Build

The table below lists every package required to compile Pluck-GTK from source,
including what was verified to work in the agent build environment
(Ubuntu 24.04 LTS, GCC 13.3, GTK 4.14.5).

| Package | Ubuntu apt name | Purpose |
|---|---|---|
| GCC | `gcc` | C compiler |
| Make | `make` | Build orchestration |
| pkg-config | `pkg-config` | Compile/link flag discovery |
| GTK 4 dev headers | `libgtk-4-dev` | GTK 4 headers & pkg-config |
| Meson | `meson` | Build system for gtk4-layer-shell |
| Ninja | `ninja-build` | Backend for Meson |
| Wayland client dev | `libwayland-dev` | Wayland protocol headers |
| Wayland protocols | `wayland-protocols` | Protocol XML files |
| GObject introspection | `libgirepository1.0-dev` | Required by gtk4-layer-shell's Meson build |
| Vala compiler | `valac` | Provides `vapigen`, required by gtk4-layer-shell's Meson build |

```bash
sudo apt-get install -y \
    gcc make pkg-config \
    libgtk-4-dev \
    meson ninja-build \
    libwayland-dev wayland-protocols \
    libgirepository1.0-dev valac
```

**gtk4-layer-shell** is not yet available in the Ubuntu apt repositories and
must be built from source:

```bash
git clone --depth 1 https://github.com/wmww/gtk4-layer-shell.git /tmp/gtk4-layer-shell
meson setup /tmp/gtk4-layer-shell/build /tmp/gtk4-layer-shell \
    --prefix=/usr \
    -Dtests=false -Dexamples=false -Ddocs=false
ninja -C /tmp/gtk4-layer-shell/build
sudo ninja -C /tmp/gtk4-layer-shell/build install
```

This installs the library to `/usr/lib/x86_64-linux-gnu/` and its
pkg-config file as `gtk4-layer-shell-0` — which is what the Makefile uses.

---

## Installation

### From a release (recommended)

1. Download `pluck-gtk.x86_64.tar.gz` from the
   [Releases page](https://github.com/SteffenBlake/Pluck-GTK/releases/latest).
2. Extract to your prefix of choice:

```bash
# Install to /usr/local/bin (default, no sudo needed if you own /usr/local)
tar -xzf pluck-gtk.x86_64.tar.gz -C /usr/local --strip-components=2

# Or to /usr/bin (system-wide, requires sudo)
sudo tar -xzf pluck-gtk.x86_64.tar.gz -C /usr --strip-components=2
```

The tarball's internal layout is `usr/bin/pluck-gtk`, so
`--strip-components=2` drops the `usr/bin/` prefix and places the binary
directly under whichever `bin/` you target.

### From source

```bash
# 1. Clone
git clone https://github.com/SteffenBlake/Pluck-GTK.git
cd Pluck-GTK

# 2. Install build deps (see Requirements → Build above)

# 3. Build
make

# 4. Install the binary
sudo make install          # installs to /usr/local/bin/pluck-gtk by default
# or:
make install PREFIX=/usr   # installs to /usr/bin/pluck-gtk
```

The compiled binary is placed in `lib/pluck-gtk` by `make` before
`make install` copies it to `$(PREFIX)/bin`.

---

## Usage

```
pluck-gtk [search-root]
```

| Argument | Default | Description |
|---|---|---|
| `search-root` | `.` (current directory) | Root directory `fd` will scan recursively |

### Examples

```bash
# Search everywhere under your home directory
pluck-gtk ~

# Search a specific project
pluck-gtk ~/projects/my-app

# No argument → search from wherever you launch it
pluck-gtk
```

### Keyboard shortcuts

| Key | Action |
|---|---|
| Type anything | Filter results in real time |
| `↑` / `↓` | Move selection through results |
| `Enter` | Open the selected file's folder in file manager |
| `Escape` | Close Pluck |

---

## Hotkey binding

Bind `pluck-gtk` to a key in your compositor config so it appears instantly.

**Sway** (`~/.config/sway/config`):
```
bindsym $mod+Space exec pluck-gtk ~
```

**Hyprland** (`~/.config/hypr/hyprland.conf`):
```
bind = $mainMod, Space, exec, pluck-gtk ~
```

---

## Building the project (Makefile reference)

```
make              # compile → lib/pluck-gtk
make install      # copy binary to /usr/local/bin/pluck-gtk
make install PREFIX=/usr   # copy to /usr/bin/pluck-gtk
make clean        # remove src/*.o and lib/pluck-gtk
```

Override `CC` to use a different compiler:

```bash
make CC=clang
```

---

## Project structure

```
Pluck-GTK/
├── src/
│   ├── main.c      Entry point; parses argv, creates GtkApplication
│   ├── ui.c/h      Window construction, GTK signal handlers, CSS
│   ├── search.c/h  Fuzzy-match highlight markup generation
│   ├── files.c/h   GtkFileLauncher wrapper (open containing folder)
│   └── config.h    Shared globals (search_root)
├── lib/            Compiled binary output (git-ignored)
├── Makefile
└── .github/
    └── workflows/
        └── release.yml   Tag-triggered release CI
```

---

## License

[MIT](LICENSE)
