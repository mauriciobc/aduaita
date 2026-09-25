# adwaita-overlay

Bevels, texture and depth for libadwaita — an additive CSS overlay loaded at
`GTK_STYLE_PROVIDER_PRIORITY_USER` (800) over libadwaita's stylesheet at
`PRIORITY_THEME` (200). Personal use only: never redistributed, nothing
proposed upstream, libadwaita never patched or forked.

Read [docs/proposal.md](docs/proposal.md) first (the design document, with
the accepted review amendments), then [BACKLOG.md](BACKLOG.md) (the working
backlog) and [docs/decisions.md](docs/decisions.md) (the running decision
record).

## Layout

```
src/            L0 tokens, L1 primitives, L2 surfaces (SCSS, built by sassc)
assets/         SVG texture tiles (data: URIs preferred); 9-slice descoped —
                border-image does not follow border-radius (decisions.md E1)
upstream/       pinned-version, selector + variable contracts, cache/ (gitignored)
tools/          fetch-upstream, check-selectors, build, render-widget.c,
                probe-motion.c, probe-foreign.c, render-gallery.c,
                gallery-diff, track
hooks/          pacman PostTransaction hook
docs/           proposal, decisions, evening-0 experiments
build/          sassc output (gitignored)
```

## Toolchain

All present: sassc, glib2 (gresource), bsdtar, git, gcc (for render-widget).

## Commands

```
tools/fetch-upstream             # fill upstream/cache/<pinned>/gtk.css from the Arch archive
tools/fetch-upstream 1:1.6.5-1   # any archived version (handles the pre-1.9 four-file layout)
tools/check-selectors            # contract vs the *installed* sheet — what the pacman hook runs
tools/check-selectors 1:1.9.4-1  # contract vs an archived version (network)
tools/build                      # sassc + symlink ~/.config/gtk-4.0/gtk.css + restart daemons
```

Offscreen render of one widget with one CSS file (pixel-level verdicts,
X5 test card):

```
gcc -O1 -o build/render-widget tools/render-widget.c $(pkg-config --cflags --libs gtk4)
build/render-widget <css-file> <out.tiff> <button|headerbar> <width> <height>
```

Offscreen motion and state verdicts — walks a real button and a real
`.boxed-list` row through hover, press and focus, and a headerbar through
backdrop, sampling at rest / mid-transition / settled:

```
gcc -O1 -o build/probe-motion tools/probe-motion.c $(pkg-config --cflags --libs gtk4)
build/probe-motion build/gtk.css                                   # house motion
build/probe-motion build/gtk.css 80 upstream/cache/1:1.9.4-1/gtk.css  # + upstream vars
REDUCE=1 build/probe-motion build/gtk.css ...                       # prefers-reduced-motion
NOANIM=1 build/probe-motion build/gtk.css ...                       # gtk-enable-animations=false
SCHEME=dark build/probe-motion build/gtk.css ...                    # dark scheme
```

Pass the upstream sheet whenever the surface under test derives from an
upstream variable (`--headerbar-bg-color` and friends): without it the
declaration is invalid at computed-value time and nothing measures.

## Tracking widget coverage

Two halves. The eyeball half is the demo apps; the pixel half is the
gallery. Both are launchers/renderers — neither writes into the repo.

```
tools/track                        # family -> demo page -> surface file
tools/track <family>               # open that page (gtk4-widget-factory or
                                   # gtk4-demo --run=<example>)
tools/track <family> -i            # ...under GTK Inspector

build/render-gallery none       out/stock    # every family, stock Adwaita
build/render-gallery build/gtk.css out/overlay
tools/gallery-diff out/stock out/overlay      # per-family pixel delta
```

Build the gallery once:

```
gcc -O1 -o build/render-gallery tools/render-gallery.c \
    $(pkg-config --cflags --libs gtk4 libadwaita-1)
```

`render-gallery` renders 15 widget families offscreen (same mechanism as
`render-widget`: a CssProvider at priority 800, `GtkWidgetPaintable` →
`GskCairoRenderer` → TIFF), one TIFF per family, and takes the same
`CONTRAST=more` / `SCHEME=dark` knobs. `gallery-diff` reports
`changed_px  mean_delta  max_delta` per family, so a material change is
visible as a number before it is judged by eye. `tools/track` names which
family lives in which surface file, which is what makes coverage
auditable rather than remembered.

`STATE=<prelight|active|checked|focus-visible|drop>` renders that state on
every widget in the family at once — a stress shot, not a per-widget
state. The hover glow, the accent drop ring and the press well are
invisible at rest by construction, so this is the only way to review the
interaction register per family. It is how the bar-button glow and the
`button.link` leak were judged:

```
STATE=prelight build/render-gallery build/gtk.css out/hover buttons
STATE=prelight build/render-gallery none          out/hover-stock buttons
tools/gallery-diff out/hover-stock out/hover
```

## Motion

Motion lives in L0 (`--ov-motion-*`: curve, enter/exit/press/switch/ring)
and every declaration emits through `ov-motion()` in L1 — which also
restates upstream's focus-ring motion, because a `transition` declaration
replaces the list rather than extending it. `prefers-reduced-motion:
reduce` collapses every duration to `0ms` (same state language, no
animation); `gtk-enable-animations=false` already does that globally in
GTK itself. Rationale and measurements: `docs/decisions.md`, "Motion
review — 23 Sep 2026".

## Foreign apps (no libadwaita)

GTK loads this sheet in **every** GTK4 process, including apps that never
call `adw_init()` — Chromium and its forks (Helium), anything GTK4 without
libadwaita. libadwaita's custom properties do not exist there, and a
declaration whose only value is an undefined `var()` computes to *nothing*:
GTK paints no background at all rather than falling back to the theme's own
colour. Chromium builds its whole Linux palette out of rendered GTK nodes
(`ui/gtk/gtk_color_mixers.cc` over `gtk_util.cc`'s `GetBgColor`) and forces
the frame colour opaque, so "paints nothing" became a completely black
browser window (decisions.md, "Foreign apps: Helium came up black",
25 Sep 2026).

The rule that keeps that from happening: every libadwaita variable the sheet
reads is read in exactly one L0 alias (`--ov-up-*`), each carrying a GTK
built-in named-colour fallback; surfaces and primitives reference the
aliases and nothing else.

`probe-foreign` reimplements Chromium's colour mixer against a bare GTK4 app
and prints the inputs it reads plus the derived frame/toolbar colours. It
exits 1 when any painted input comes out fully transparent — the exact
condition Chromium renders as black:

```
gcc -O1 -o build/probe-foreign tools/probe-foreign.c \
    $(pkg-config --cflags --libs gtk4)
build/probe-foreign                    # what this machine reads now
build/probe-foreign none               # stock GTK control
build/probe-foreign build/gtk.css      # ...or any sheet, in isolation
SCHEME=dark build/probe-foreign build/gtk.css
CONTRAST=more build/probe-foreign build/gtk.css
```

## The contracts

- `upstream/selectors.txt` — every selector atom L2 depends on upstream
  having. Hand-written on purpose: adding a line is a deliberate act of
  taking on a dependency.
- `upstream/variables.txt` — every upstream variable L0 reads, all of them
  through the `--ov-up-*` aliases. Guards the load-bearing layer; a renamed
  upstream variable is caught here, not by the selector contract.

`tools/check-selectors` exits 1 on any miss. The pacman hook runs it against
the installed sheet after every libadwaita upgrade — no network, nothing
written into the repo, safe as root:

```
sudo cp hooks/adwaita-overlay.hook /etc/pacman.d/hooks/
```

Fix the `Exec` path in the hook if the repo ever moves.
