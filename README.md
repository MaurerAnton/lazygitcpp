# lazygitcpp — Terminal UI for Git (C++ port of lazygit)

A zero-dependency C++ port of [lazygit](https://github.com/jesseduffield/lazygit) — a simple terminal UI for git commands with keyboard-driven workflows.

## Why lazygitcpp?

The original [lazygit](https://github.com/jesseduffield/lazygit) requires the Go toolchain plus dozens of modules. lazygitcpp compiles with a single `make` using only C++17 and standard Linux headers.

## Quick Start

```bash
make
./lazygitcpp
```

## Features

- Stage/unstage files individually or in hunks
- Interactive rebase with visual commit list
- Branch management (checkout, merge, delete)
- Stash browser
- Cherry-pick commits
- Diff viewer
- Commit and push workflow

## Build

```bash
make
```
Requires: GCC 10+ or Clang 12+, GNU Make
