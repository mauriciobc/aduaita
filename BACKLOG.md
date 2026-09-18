# BACKLOG — adwaita-overlay

How to read this file:

- One checkbox = one task. A task is **done when its acceptance criterion
  passes**, not when code exists.
- IDs are stable. Never renumber; drop a task by striking it through with a
  reason, never by deleting it.
- Priorities: **P0** blocks the current milestone, **P1** is
  current-milestone work, **P2** is hardening or deferred (pull a P2
  forward when it starts blocking).
- Milestone numbers follow docs/proposal.md, including the new Milestone 0
  (Evening 0).

---

## M0 — Evening 0: mechanism experiments

*Gate for everything downstream. Run the four tests in
[docs/evening-0.css](docs/evening-0.css) in GTK Inspector
(`GTK_DEBUG=interactive <app>`), one at a time. Record every verdict in
`docs/decisions.md`.*

- [ ] **E1** (P0) border-image × border-radius. Paste T1 onto a window with
  regular rounded buttons; inspect the corners.
  *Accept:* verdict recorded — does the bevel clip to the rounded fill or
  draw square corners? If square (expected), 9-slice is descoped,
  `bevel()` becomes inset shadows / clipped gradient layers, and the
  assets/ SVG 9-slice pipeline is struck from the plan.
- [ ] **E2** (P0) Inset bevel crispness. T2 on default and pill buttons.
  *Accept:* crisp at 1×; verdict recorded at 1.25× and 1.5× fractional
  scale (where 1px insets can smear).
- [ ] **E3** (P0) Pressed inversion. T3.
  *Accept:* press reads as physical depression; zero layout shift
  (transform only); works on default and pill variants.
- [ ] **E4** (P0) Grain tile. T4 on a headerbar, light and dark.
  *Accept:* renders via librsvg; no shimmer while scrolling a list under
  the headerbar; opacity verdict (0.03 / 0.05 / 0.08); provisional
  keep-or-drop call.
- [ ] **E5** (P0) `docs/decisions.md` written: one section per experiment,
  the `bevel()` mixin shape decided, texture provisional call, and the
  elevation ladder sketch (4 rungs: flat / raised / overlay / window).

## M1 — Pipeline

*Done when: a no-op build installs, loads, and survives a
rebuild-and-restart cycle.*

- [ ] **P1.0** (P0) Install sassc: `sudo pacman -S --needed sassc`
  (everything else in the toolchain is already present).
- [ ] **P1.1** (P0) No-op build: `tools/build` compiles the comment-only
  skeleton, symlinks `~/.config/gtk-4.0/gtk.css`, restarts daemons.
  *Accept:* rebuild + restart leaves every app visually unchanged; two
  consecutive builds produce byte-identical output.
- [ ] **P1.2** (P0) sassc round-trip: add one `color-mix()` and one
  relative-colour derivation to `_tokens.scss`.
  *Accept:* both survive compilation verbatim in `build/gtk.css` — libsass
  must pass modern colour syntax through, not mangle it. If it mangles,
  switch to Dart Sass and record why in decisions.md.
- [ ] **P1.3** (P0) Load test: temporary `headerbar { background: red; }`
  in the overlay.
  *Accept:* visible in a native GNOME app; then removed.
- [ ] **P1.4** (P1) Flatpak access:
  `flatpak override --user --filesystem=xdg-config/gtk-4.0`.
  *Accept:* a Flatpak GNOME app shows the P1.3 red rule.
- [ ] **P1.5** (P0) Migrate the existing `~/.config/gtk-4.0/gtk.css`
  (window / content-pane / sidebar-pane translucency, 10 lines) into
  `src/surfaces/`, raw `rgb()` values replaced by upstream-variable
  derivations, selectors registered in the contract.
  *Accept:* old file replaced by the build symlink; same visual effect; no
  raw colours outside L0.
- [ ] **P1.6** (P1) Install the pacman hook
  (`sudo cp hooks/adwaita-overlay.hook /etc/pacman.d/hooks/`), then dry-run
  the hook path: `sudo tools/check-selectors`.
  *Accept:* exit 0; `git status` clean afterwards — proof the guard never
  writes into the repo as root.
- [ ] **P1.7** (P1) Contract self-test: append one bogus selector and one
  bogus variable to the contracts, run `tools/check-selectors`, revert.
  *Accept:* exit 1, both misses reported by name.
- [ ] **P1.8** (P2) Confirm `upstream/pinned-version` equals
  `pacman -Q libadwaita` (it did at scaffold time: 1:1.9.4-1).

## M2 — Design system: L0 + L1

*Done when: the primitives render correctly on a single test widget.*

- [ ] **D1** (P0) Elevation ladder in `_tokens.scss`: rungs flat / raised /
  overlay / window, each a `--ov-depth-*` token holding a 2–3 stop
  low-alpha shadow ladder derived from upstream variables. One light
  source: light from the top, all shadows fall downward.
  *Accept:* no raw hex outside `:root`; ladders verified in light and dark.
- [ ] **D2** (P0) `bevel()` mixin, mechanism per E1 verdict. Every
  declaration restates upstream's existing `box-shadow` /
  `background-image` stops alongside the overlay's — we override, not
  append; the pin makes restating safe.
  *Accept:* bevelled test button wins at priority 800 with no `!important`.
- [ ] **D3** (P0) `depth()` mixin — box-shadow ladders only, `filter`
  banned in v1.
  *Accept:* raised ↔ flat A/B on one widget via the kill switch.
- [ ] **D4** (P1) `texture()` mixin — grain tile `data:` URI + opacity
  token ≤ 5%; `-gtk-recolor()` only if a tile must track the palette.
  *Accept:* applies to the headerbar only; independently toggleable;
  skipped entirely if E4 said drop.
- [ ] **D5** (P0) Per-family kill switches `--ov-bevel` / `--ov-texture` /
  `--ov-depth`: every primitive emits its full value through a token; the
  switches redefine those tokens.
  *Accept:* toggling each in Inspector removes exactly that family and
  nothing else.
- [ ] **D6** (P0) Contrast reverts emitted inline by every L1 mixin.
  *Accept:* grep audit — no material declaration without a sibling
  `prefers-contrast: more` revert; Inspector HC toggle flattens the test
  widget.
- [ ] **D7** (P1) Naming audit: `--ov-<category>-<role>`, three parts,
  lowercase; the kill switches are the only documented exceptions.
- [ ] **D8** (P2) Perf baseline: `gtk4-demo` animated page with each family
  on/off. *Accept:* no visible frame drops; formal measurement deferred to
  M4 where node counts are highest.

## M3 — Headerbar & toolbar — GO/NO-GO

*Done when: the largest visual mass carries the new material and the
verdict is recorded. Stop the project if it is a no-go.*

- [ ] **H1** (P0) Headerbar cluster, incl. `.flat`, `.boxed`, window
  controls area. Every selector registered in `upstream/selectors.txt` as
  it lands; every rule commented with the upstream source file.
- [ ] **H2** (P0) Window surface: headerbar↔content hairline separator
  (alpha, never solid); window rung of the ladder.
- [ ] **H3** (P1) Toolbar / searchbar / actionbar.
- [ ] **H4** (P0) Side-by-side verification: Files + Epiphany + one Flatpak
  app; light/dark × HC/normal × 1×/1.25×/1.5×.
  *Accept:* no native↔Flatpak divergence beyond accent colour. Any
  divergence means a runtime-version mismatch → **X1 becomes P0**.
- [ ] **H5** (P0) GO/NO-GO recorded in `docs/decisions.md`. If no-go:
  revert the surfaces, keep the repo, the guard and the pipeline.

## M4 — Lists & cards — density/perf gate

- [ ] **C1** (P0) `list.boxed-list`, `.boxed-list-separate`, `.card`;
  hairline row separators. No shadows on rows — shadows belong to elevation
  transitions only.
- [ ] **C2** (P0) Performance measurement on the Intel iGPU: Files grid +
  list view, heavy scrolling, each family toggled.
  *Accept:* no perceptible jank; `--ov-depth: none` documented in README
  as the escape hatch.

## M5 — Controls

- [ ] **U1** (P0) Button matrix: default / flat / pill / suggested /
  destructive / osd / opaque; states hover, `:active` (inverted bevel per
  E3), `:checked`, `:disabled`, `:focus-visible`. Ladders on default
  variants only — flat stays flat.
- [ ] **U2** (P1) Entry + search + password-reveal.
- [ ] **U3** (P1) Popover + menu — overlay rung of the ladder.
- [ ] **U4** (P2) Splitbutton / dropdown / combobox.
- [ ] **U5** (P1) Hardcoded-hex audit at the pinned version: extract every
  hex outside `:root`, classify palette-following vs structural (shadow
  `rgb()` is fine), decide override or accept per case.
  *Accept:* the classified list recorded in `docs/decisions.md`.
- [ ] **U6** (P2) Inclusion decision: scrollbar, switch, slider,
  AdwTabBar / viewswitcher — identity surfaces; only if the M3 verdict
  says the aesthetic wants them.

## M6 — Contrast & accessibility

- [ ] **A1** (P0) Full HC pass over every surface via the Inspector
  contrast toggle.
  *Accept:* every material effect flat; text contrast untouched.
- [ ] **A2** (P0) Focus visibility audit on bevelled surfaces.
  *Accept:* focus rings clearly visible over bevel + texture in both
  schemes.

## M7 — Daily driving

- [ ] **DD1** (P0) Two weeks of normal use before adding anything further.
  Annoyances are appended to the section below.
- [ ] **DD2** (P1) After two weeks: final texture keep/drop; only then
  consider `filter`, and only for a surface that demonstrably needs blur.

---

## Hardening (P2 — pull forward when blocking)

- [ ] **X1** Flatpak runtime guard: extend `check-selectors` to also
  extract from installed `org.gnome.Platform` runtimes and check the
  contract against the **oldest** libadwaita actually running. *(Becomes
  P0 if H4 shows divergence.)*
- [ ] **X2** `tools/selftest`: automate P1.7 (bogus contract entries →
  exit 1, both axes reported) so the guard is testable in one command.
- [ ] **X3** Regenerate the proposal's metrics from the pinned version —
  lines, `:root` variable count, hardcoded-hex count, `filter` /
  `background-image` tallies, selector survival — and replace the pre-pin
  numbers in docs/proposal.md.
- [ ] **X4** Bisect rehearsal: deliberately break one visual rule, `git
  bisect` to it, fix it. Verifies one-visual-change-per-commit actually
  pays.
- [ ] **X5** Fractional-scale test card: fixed checklist (headerbar,
  button, list, popover at 1×/1.25×/1.5×) run before every milestone
  sign-off.

## Found during daily drive

*(append entries here as they appear; nothing yet)*

---

## Out of scope — explicit, do not creep

- The GNOME Shell stylesheet (a separate project, only if the desktop
  reads inconsistent once apps are done)
- Upstream contributions, issues, MRs — libadwaita does not accept
  AI-assisted contributions; nothing from this tree is ever filed
- Redistribution of any kind
- Patching, forking or replacing libadwaita
- Widget internals, layout, metrics, padding, adaptive breakpoints, window
  controls
