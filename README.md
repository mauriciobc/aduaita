# adwaita-overlay

Bevels, texture and depth for libadwaita — an additive CSS overlay loaded at
`GTK_STYLE_PROVIDER_PRIORITY_USER` (800) over libadwaita's stylesheet at
`PRIORITY_THEME` (200). Personal use only: never redistributed, nothing
proposed upstream, libadwaita never patched or forked.

Read [docs/proposal.md](docs/proposal.md) first (the design document, with
the accepted review amendments), then [BACKLOG.md](BACKLOG.md) (the working
backlog). [docs/evening-0.css](docs/evening-0.css) holds the mechanism
experiments that gate the design — run them before writing any L1 code.

## Layout

```
src/            L0 tokens, L1 primitives, L2 surfaces (SCSS, built by sassc)
assets/         SVG texture tiles — only if Evening 0 keeps them alive
upstream/       pinned-version, selector + variable contracts, cache/ (gitignored)
tools/          fetch-upstream, check-selectors, build, _lib.sh
hooks/          pacman PostTransaction hook
docs/           proposal, evening-0 experiments, decisions
build/          sassc output (gitignored)
```

## Toolchain

All present except **sassc** — install it before the first build:

```
sudo pacman -S --needed sassc
```

## Commands

```
tools/fetch-upstream             # fill upstream/cache/<pinned>/gtk.css from the Arch archive
tools/fetch-upstream 1:1.6.5-1   # any archived version (handles the pre-1.9 four-file layout)
tools/check-selectors            # contract vs the *installed* sheet — what the pacman hook runs
tools/check-selectors 1:1.9.4-1  # contract vs an archived version (network)
tools/build                      # sassc + symlink ~/.config/gtk-4.0/gtk.css + restart daemons
```

## The contracts

- `upstream/selectors.txt` — every selector atom L2 targets. Hand-written on
  purpose: adding a line is a deliberate act of taking on a dependency.
- `upstream/variables.txt` — every upstream variable L0 derives from. Guards
  the load-bearing layer; a renamed upstream variable is caught here, not by
  the selector contract.

`tools/check-selectors` exits 1 on any miss. The pacman hook runs it against
the installed sheet after every libadwaita upgrade — no network, nothing
written into the repo, safe as root:

```
sudo cp hooks/adwaita-overlay.hook /etc/pacman.d/hooks/
```

Fix the `Exec` path in the hook if the repo ever moves.

## Note

`~/.config/gtk-4.0/gtk.css` currently exists as a real file (window and
pane translucency tweaks). `tools/build` refuses to replace a real file;
BACKLOG.md P1.5 migrates those rules into the overlay first.
