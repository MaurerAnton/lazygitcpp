# lazygitcpp — Git Terminal UI (C++ port of lazygit)

A zero-dependency C++ port of [lazygit](https://github.com/jesseduffield/lazygit) — a keyboard-driven terminal UI for everyday git operations.

## Why lazygitcpp?

The original [lazygit](https://github.com/jesseduffield/lazygit) requires Go plus dozens of modules. lazygitcpp compiles with a single `make` using only C++17 and pthreads.

## Quick Start

```bash
make
./lazygitcpp
```

## Features

- **Status panel** — staged/unstaged files with color coding
- **Branches panel** — branch list sorted by last commit date
- **Log panel** — `git log --oneline --graph` (30 entries)
- **Stash panel** — stash list

## Keyboard Controls

| Key | Action |
|-----|--------|
| 1-4 | Switch panels (Status/Branches/Log/Stash) |
| j/k | Move selection down/up |
| Space | Stage/unstage file (Status) or checkout branch (Branches) |
| c | Commit with editor prompt |
| p | Push |
| P | Pull |
| q | Quit |

## Limitations

- No interactive rebase, cherry-pick, or diff viewer
- No horizontal split for diff display
- No submodule support

## Build

```bash
make
```
Requires: GCC 10+ or Clang 12+, GNU Make, `git` in PATH
