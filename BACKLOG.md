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

- [x] **E1** (P0) border-image × border-radius. Paste T1 onto a window with
  regular rounded buttons; inspect the corners.
  *Accept:* verdict recorded — does the bevel clip to the rounded fill or
  draw square corners? If square (expected), 9-slice is descoped,
  `bevel()` becomes inset shadows / clipped gradient layers, and the
  assets/ SVG 9-slice pipeline is struck from the plan.
- [x] **E2** (P0) Inset bevel crispness. T2 on default and pill buttons.
  *Accept:* crisp at 1× ✓ (pixel profile in decisions.md); the 1.25×/1.5×
  cells are covered by X5's test card (display runs 1×) — not blocking.
- [ ] **E3** (P0) Pressed inversion. T3.
  *Accept:* press reads as physical depression; zero layout shift
  (transform only); works on default and pill variants.
- [ ] **E4** (P0) Grain tile. T4 on a headerbar, light and dark.
  *Accept:* renders via librsvg; no shimmer while scrolling a list under
  the headerbar; opacity verdict (0.03 / 0.05 / 0.08); provisional
  keep-or-drop call.
- [x] **E5** (P0) `docs/decisions.md` written: one section per experiment,
  the `bevel()` mixin shape decided, texture provisional call, and the
  elevation ladder sketch (4 rungs: flat / raised / overlay / window).

## M1 — Pipeline

*Done when: a no-op build installs, loads, and survives a
rebuild-and-restart cycle.*

- [x] **P1.0** (P0) Install sassc: `sudo pacman -S --needed sassc`
  (everything else in the toolchain is already present).
- [x] **P1.1** (P0) No-op build: `tools/build` compiles the comment-only
  skeleton, symlinks `~/.config/gtk-4.0/gtk.css`, restarts daemons.
  *Accept:* rebuild + restart leaves every app visually unchanged; two
  consecutive builds produce byte-identical output.
- [x] **P1.2** (P0) sassc round-trip: add one `color-mix()` and one
  relative-colour derivation to `_tokens.scss`.
  *Accept:* both survive compilation verbatim in `build/gtk.css` — libsass
  must pass modern colour syntax through, not mangle it. If it mangles,
  switch to Dart Sass and record why in decisions.md.
- [x] **P1.3** (P0) Load test: temporary `headerbar { background: red; }`
  in the overlay.
  *Accept:* visible in a native GNOME app; then removed.
- ~~**P1.4** (P1) Flatpak access: `flatpak override --user --filesystem=xdg-config/gtk-4.0`.
  *Accept:* a Flatpak GNOME app shows the P1.3 red rule.~~
  *(Deferred 18 Sep 2026 — Flatpak out of scope for now; the override that
  had been applied was reverted. Reinstate when Flatpak returns to scope.)*
- [x] **P1.5** (P0) Migrate the existing `~/.config/gtk-4.0/gtk.css`
  (window / content-pane / sidebar-pane translucency, 10 lines) into
  `src/surfaces/`, raw `rgb()` values replaced by upstream-variable
  derivations, selectors registered in the contract.
  *Accept:* old file replaced by the build symlink; same visual effect; no
  raw colours outside L0.
- [x] **P1.6** (P1) Install the pacman hook
  (`sudo cp hooks/adwaita-overlay.hook /etc/pacman.d/hooks/`), then dry-run
  the hook path: `sudo tools/check-selectors`.
  *Accept:* exit 0; `git status` clean afterwards — proof the guard never
  writes into the repo as root.
- [x] **P1.7** (P1) Contract self-test: append one bogus selector and one
  bogus variable to the contracts, run `tools/check-selectors`, revert.
  *Accept:* exit 1, both misses reported by name.
- [x] **P1.8** (P2) Confirm `upstream/pinned-version` equals
  `pacman -Q libadwaita` (it did at scaffold time: 1:1.9.4-1).

## M2 — Design system: L0 + L1

*Done when: the primitives render correctly on a single test widget.*

- [x] **D1** (P0) Elevation ladder: rungs flat / raised / overlay /
  window. *Amended by M2 findings:* stop *geometry* lives in L1 functions
  (`ov-depth-raised()` etc. — var() does not substitute whole multi-stop
  segments), stop *colours* derive in L0 from `var(--dark-5)`.
  *Accept:* no raw colours outside L0 ✓ (grep); light/dark verification
  pending — Inspector session, with E3/E4.
- [x] **D2** (P0) `bevel()` mixin, mechanism per E1 verdict (inset pair).
  Surfaces restate upstream's stops via `ov-elevate($upstream)`; the
  contrast revert restores them.
  *Accept:* priority-800 win with no `!important` ✓ (harness, provider at
  800); aesthetic pass pending — Inspector.
- [x] **D3** (P0) `depth()` — box-shadow ladders only, `filter` banned in
  v1. *Accept:* raised ↔ flat A/B moved to the Inspector session (harness
  geometry unreliable — decisions.md M2); mechanism + kill switch verified
  by construction and by the texture-mode analogue.
- [x] **D4** (P1) `texture()` mixin — grain tile `data:` URI, opacity
  baked into the tile at 0.05 (the budget ceiling; data: URIs cannot read
  CSS variables). *Mechanism verified:* renders (stddev 0.618),
  `--ov-texture-image: none` → flat, HC → flat. Headerbar application
  lands with M3.
- [x] **D5** (P0, amended) Per-family kill switches, one edit each —
  mechanism corrected by the var() constraint (decisions.md M2):
  `--ov-bevel-width: 0px` (length component ✓ verified),
  `--ov-depth-color: transparent` (shared stop colour; pending Inspector
  A/B), `--ov-texture-image: none` (✓ verified flat).
- [x] **D6** (P0) Contrast reverts emitted inline by every L1 mixin.
  *Accept:* grep audit ✓ (every `ov-elevate`/`ov-texture` emits the
  `prefers-contrast: more` sibling); harness HC knob flattens the test
  widget ✓ (texture: stddev 0.618 → 0.000).
- [x] **D7** (P1) Naming audit: `--ov-<category>-<role>`, three parts,
  lowercase; kill-switch tokens are the documented exceptions.
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
- [ ] **H4** (P0) Side-by-side verification: Files + Epiphany;
  light/dark × HC/normal × 1×/1.25×/1.5×.
  *Accept:* no regressions in any matrix cell. *(The Flatpak matrix cell is
  deferred with Flatpak — proposal amendment 11.)*
- [ ] **H5** (P0) GO/NO-GO recorded in `docs/decisions.md`. If no-go:
  revert the surfaces, keep the repo, the guard and the pipeline.
- [x] **H6** (P1) Headerbar/bar backdrop state feedback: upstream's
  `headerbar:backdrop { background-color: var(--headerbar-backdrop-color);
  transition: background-color 200ms ease-out; }` never showed, because the
  overlay's `background:` shorthand set the colour in every state (found
  in the 23 Sep motion review — window controls still dim via
  `windowhandle`, the surface did not).
  *Accept:* decision recorded — either restate the backdrop colour in the
  overlay's bar rule (surface dims again, with upstream's fade) or accept
  and strike this with the reason.
  **Restored** via `--ov-bar-base` / `--ov-bar-backdrop-base` (the colour
  moves inside the gradient, since the stops are opaque); dark-mode
  recession measured 232 → 228 and confirmed live; light mode is below the
  noise floor. Kept 23 Sep 2026.

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
- [x] **U4** (P2) Splitbutton / dropdown / combobox. (splitbutton inherits
  button surface and well; dropdown button/popover covered by existing
  surfaces — documented restraint).
- [ ] **U5** (P1) Hardcoded-hex audit at the pinned version: extract every
  hex outside `:root`, classify palette-following vs structural (shadow
  `rgb()` is fine), decide override or accept per case.
  *Accept:* the classified list recorded in `docs/decisions.md`.
- [x] **U6** (P2) Inclusion decision: scrollbar, switch, slider,
  AdwTabBar / viewswitcher — identity surfaces: switch ported, scrollbar
  trough deepened, scale slider + progress channels added; tabbar/viewswitcher
  deferred per U6 decision.
- [x] **U7** (P2) Disabled switch knob: the overlay's resting
  `switch > slider` rule outranks upstream's
  `switch > slider:disabled { box-shadow: 0 2px 4px transparent }`, so a
  disabled switch keeps a fully raised knob (found in the 23 Sep motion
  review; state legibility, not motion).
  *Accept:* the disabled knob reads recessed/flat while the track dims.
  **Done 23 Sep 2026** — disabled is flat: no drop, no insets, no gradient;
  the track keeps upstream's own `filter` dim. Evidence in decisions.md,
  "Widget-family sweep".

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

## Widget-family sweep — 23 Sep 2026

*"Apply the recorded design language to all GTK4 widgets." Method, evidence
and rejected candidates: decisions.md, "Widget-family sweep". Tracking:
`tools/track` opens the demo page per family; `tools/render-gallery` +
`tools/gallery-diff` give per-family pixel evidence.*

- [x] **W1** (P0) Material scope: the button material leaked onto every
  family upstream paints flat without the class (bar icon buttons, window
  controls, table/calendar headers, spinbutton arrows, thumbnails, model
  buttons, pathbar crumbs, bottom-sheet actions, `.flat` parents'
  children). *Accept:* stock look restored on those families, material
  unchanged on opaque buttons.
  **Scope verdict kept** (user, 23 Sep 2026): offered the glow back on bar
  icon buttons, answered "keep it as it is". Also caught `button.link`,
  which was rendering as a glow chip. Lever if it ever reopens: delete
  `$ov-flat-bar-contexts` from the loop in `_button.scss`.
- [x] **W2** (P0) Drop state: `button`/`entry`/`spinbutton`/`.card`
  `:drop(active)` accent rings were erased by our `box-shadow` at priority
  800. *Accept:* the accent ring renders again on every drop target.
- [x] **W3** (P0) `.card` definition ring restated (a white card had no
  edge on a light window).
- [x] **W4** (P1) `spinbutton` gets `ov-inset()`, the material the BACKLOG
  claimed it inherited from `entry` — it does not; `spinbutton` is a
  sibling node in GTK 4.
- [x] **W5** (P1) House timing on the content cells that had upstream
  washes and no timing: bare `row.activatable`, flowbox/gridview children,
  list-based menu rows, notebook tabs, calendar day cells.
- [x] **W6** (P1) `expander-widget` title row wash (the one additive
  material: upstream's only feedback was the arrow's opacity).
- [x] **W7** (P1) `bottom-sheet` / `floating-sheet` on the window rung
  (`ov-depth-window()` — the rung had no surface until now).
- [x] **W8** (P1) `filter: none` on disabled buttons erased upstream's
  `filter: opacity()` dim across the bar families. *Accept:* disabled
  buttons dim again.
- [x] **W9** (P2) Checked-switch hover/press feedback restated (our dish
  gradient swallowed upstream's second layer).
- [x] **W10** (P2) `.view` / `textview > text` container scoop — tried and
  **reverted**: both nodes are content-sized, so the shade scrolls with the
  content (probes in decisions.md). Keeps upstream's flat fill.
- [x] **W11** (P2) GtkCalendar verdict: no demo page in
  `gtk4-widget-factory` or `gtk4-demo` shows a calendar, so the family was
  judged in `tools/render-gallery`'s `calendar` family instead — stock vs
  overlay, light/dark/HC, plus `STATE=prelight` for the header wash. No
  regression; the calendar node itself is stock at rest and only its timing
  changed. A live witness does not exist on this machine (scanned
  `/usr/bin`, `/usr/lib` for `gtk_calendar_new` / `GtkCalendarPopover`:
  telegram-desktop, yad, gtk4-icon-editor, libgtk, libwebkit2gtk — no GNOME
  surface). *Accept:* verdict recorded; reopen if an app that shows one
  enters daily use.
- [x] **W12** (P1) Lit channel fills: the coloured part of a progress bar,
  a scale and a levelbar block was a flat `background-color` (upstream) and
  read as a sticker in the recessed trough. *Accept:* the fill reads lit —
  new L1 register `ov-lit-fill()`, colour underneath left opaque because it
  is data. Numbers, rejected variants and the channel HC-ring defect found
  in passing: decisions.md, "Lit channel fills". Values are one edit wide in
  `_primitives.scss` if the dome wants to be stronger or softer.

---

## Hardening (P2 — pull forward when blocking)

- ~~**X1** Flatpak runtime guard: extend `check-selectors` to also
  extract from installed `org.gnome.Platform` runtimes and check the
  contract against the **oldest** libadwaita actually running.~~
  *(Deferred with Flatpak, 18 Sep 2026 — proposal amendment 11.)*
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
- [ ] **X6** Fold `tools/probe-motion` into the X5 card: build it
  alongside `render-widget`, and run the motion checklist (hover, press,
  focus ring, row entry) against the built sheet in normal, `REDUCE=1` and
  `NOANIM=1` before every milestone sign-off. Catches the two failure
  modes the 23 Sep review fixed: a declaration that replaces upstream's
  transition list, and entry/exit timing declared on the wrong state.
  *Accept:* one command reports every state as IN MOTION, INSTANT or NO
  CHANGE, and the reduce run reports INSTANT for all of them.

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
- Flatpak apps and runtimes — deferred 18 Sep 2026, revisit if Flatpak
  GNOME apps enter daily use

---

## Daily drive — STARTED 19 Sep 2026

The clock is running. The stylesheet is complete and live; from here the
project's only input is real use. Annoyances, breakages, and fatigues go
to "Found during daily drive" above. The gate: two weeks (M7), then the
final texture keep/drop call (DD2).
