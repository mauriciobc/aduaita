# Decisions — adwaita-overlay

Running decision record. Each entry names the evidence that produced it.
Experiments reference `docs/evening-0.css` (T1–T4) and BACKLOG.md items.

## Evening 0 — 18 Sep 2026

**Method.** `tools/render-widget.c` (promoted from this session's scratch
harness): renders one widget with one CSS file offscreen via
GtkWidgetPaintable + GskCairoRenderer, using the same mechanism as the
overlay itself — a CssProvider on the default display at
GTK_STYLE_PROVIDER_PRIORITY_USER (800). Verdicts come from pixel analysis
of the TIFF output; contact sheets for human confirmation are scratch
(`/tmp`). The harness briefly presents a window (~400 ms) because
GtkWidgetPaintable only renders mapped widgets.

### E1 — border-image × border-radius: NOT clipped → 9-slice dead

- Control: plain 4px border on a 12px-radius button — 0 of 4 sampled
  pixels outside the corner radius opaque (normal borders are clipped).
- border-image: 3 of 4 opaque (only the exact corner subpixel survives),
  border band renders normally (mid-top pixel 252,0,2).
- Conclusion: GTK follows web semantics — border-image draws **square
  corners over a rounded fill**.

**Decision:** `bevel()` is inset box-shadows (primary mechanism) and
clipped gradient background layers (for bevels wider than ~2px). The
assets/ SVG 9-slice pipeline is descoped; assets/ remains for texture
tiles only, with `data:` URIs preferred. Every bevel declaration must
restate upstream's existing `box-shadow` stops (we override, not append).

### E2 — inset bevel pair: renders, crisp at 1×

Column profile through the button (x=10, left of the label):

- sharp jump into the highlight band at the top edge (202 → 250 in one
  row), flat background through the middle, a dark line at the bottom
  edge (177 at y=60).
- Blur-0 shadows stay blur-0: no smearing.

**Decision:** the inset pair is the bevel mechanism. The 1.25×/1.5×
fractional cells are not blocking (display runs 1×): they are covered by
the X5 test card before milestone sign-offs.

### E3 — pressed inversion: pending (human verdict)

Press-feel cannot be measured. Paste T3 from docs/evening-0.css into
Inspector and judge: physical depression, zero layout shift, default and
pill variants.

### E4 — grain tile: renders

- feTurbulence data: URI tile renders via librsvg: luminance stddev
  0.562 over a 32×32 crop at 5% opacity (flat background would be ~0).
- Shimmer-on-scroll verdict: pending (human).

**Provisional call:** keep texture as a headerbar-only garnish; final
keep-or-drop at DD2, and it stays independently killable via
`--ov-texture: none` regardless.

### E5 — bevel() mixin shape + elevation ladder sketch

From E1/E2:

```scss
// sketch — exact token names and restated upstream stops land with D2
@mixin ov-bevel($width: 1px) {
  box-shadow: inset 0 $width 0 var(--ov-bevel-highlight),
              inset 0 -$width 0 var(--ov-bevel-shadow);
  // plus the restated upstream box-shadow stops for this surface
  @media (prefers-contrast: more) { /* revert to upstream's stops */ }
}
```

Elevation ladder (4 rungs; exact values derive in D1 from upstream
variables — this fixes only rung count and usage map):

| Rung | Used by | Character |
| --- | --- | --- |
| flat (0) | list rows, inline content | no shadow |
| raised | buttons, entries, cards | 2 stops, shallow |
| overlay | popovers, menus | 3 stops, deeper |
| window | headerbar cluster, windows | deepest, widest |

Light source: top, everywhere. Highlights on top edges, shade on bottom
edges, all shadows fall downward.

---

**Method.** Harness renders (tools/render-widget.c) + node-tree dump
(nodes.c, scratch). The harness's texture-mode verdicts are sound; its
button/box geometry is unreliable: the widget's real allocation never
equals the requested size (win 270×217 / widget 222×34 vs requested
200×100), and GtkWidgetPaintable rescales — so absolute pixel geometry
from button/box renders must not be trusted. (Earlier Evening-0 verdicts
E1/E2/E4 are unaffected: they judged clipping/crispness/rendered-ness of
the rendered surface itself, not absolute geometry.)

### Discovered constraint: var() substitutes single value tokens only

- `box-shadow: inset 0 var(--ov-bevel-width) 0 var(--ov-bevel-highlight)` — works.
- `box-shadow: … , var(--ov-depth-raised)` where the token holds a whole
  multi-stop segment — **does not render** (single-stop segment fails too).
- GTK docs are silent on substitution semantics ("no direct replacement"
  for non-colour types); upstream's sheet follows the constraint: per-
  component vars only (`--shade-color`, `--border-opacity` inside
  `color-mix`), never a whole segment from one var.

**Decision:** geometry lives in L1's mixin (per-rung functions with literal
stop geometry); colours flow through L0 tokens. Consequences:

- Depth kill switch is `--ov-depth-color: transparent` (one token, shared
  base colour of every ladder stop — was `--ov-depth-raised: …`).
- The M0-era ladder tokens `--ov-depth-raised/overlay/window` are removed
  from L0 — the rungs are now `ov-depth-raised()` etc. functions in L1.
- D5's acceptance is reworded in BACKLOG.md to match the mechanism.

### Verified by render (texture mode + colour-mode knob, reliable)

- texture renders (stddev 0.618 @ 5% opacity)
- `--ov-texture-image: none` → flat (0.000) — kill switch works
- `CONTRAST=more` + `@media (prefers-contrast: more)` revert → flat
  (0.000) — the inline L3 revert works

### NOT verified by harness (geometry-unreliable)

- bevel pair presence/crispness *in the M2 mixins* (E2 earlier validated
  the mechanism from hand-written CSS)
- ladder rungs' outer shadow rendering

**These move to the Inspector session (with E3/E4):** paste the compiled
`d-raised.css` / `d-kill.css` on a real surface and confirm by eye — the
compiled declaration matches upstream's proven `box-shadow` pattern, so
rendering is expected; what needs a human is the aesthetic anyway.

## Pressed-state decision — 19 Sep 2026 (Inspector session, live)

User verdict after judging the refined variants: **N4 — the pressed well.**

- Resting state: the tuned 0.5px hairline pair (white@30% / black@15%),
  scoped to opaque buttons (`.raised` + default; `.flat`/`.osd` excluded).
- Pressed state: the bevel + ladder are REPLACED by an inverted well —
  shadow moves to the inside:
  ```css
  box-shadow: inset 0 2px 4px color-mix(in srgb, black 10%, transparent),
              inset 0 -1px 1px color-mix(in srgb, white 30%, transparent);
  filter: brightness(0.96);
  ```
- No outward shadows during press; no translate. The surface sinks.
- Plus the 90ms ease-out transition on press AND release (box-shadow,
  filter) — user confirmed the animation was the missing piece in the
  earlier snap version.

Implementation note: the pressed well REPLACES the resting pair and rung
(they cannot coexist — two competing inset stories). ov-elevate's pressed
companion emits exactly the well; restated upstream stops still apply in
both states.

**REVISED same day:** the user struck the resting bevel entirely —
**no resting styling at all.** Idle buttons stay stock Adwaita; N4's well
is pressed-only. The 0.5px hairline values remain on record as the tuned
bevel recipe; `--ov-bevel-width: 0px` is now effectively the standing
state (no surface emits the resting pair by default).

## Bevel tuning — 19 Sep 2026 (Inspector session, live)

The resting bevel was tuned on real buttons by the user:

- **Geometry:** 0.5px insets — sub-pixel hairlines (GSK antialiases them;
  graceful at fractional scale, unlike 1px+ pairs).
- **Strength:** highlight white@30%, shadow black@15% — "whisper" register
  (R2 direction, weaker than shipped R1's 55/30).
- **Scope:** NOT all buttons — opaque buttons only. Flat buttons (headerbar
  buttons, sidebars) stay flat; L2 will scope via `button:not(.flat)`-
  style selectors when surfaces land (M3). Upstream already uses `:not()`
  extensively, so the selector pattern is proven in-tree.
- These values are now the shipped L0 tokens (`--ov-bevel-width: 0.5px`,
  `--ov-bevel-highlight` @30%, `--ov-bevel-shadow` @15%).

## Neumorphism probe — 19 Sep 2026

N1–N3 (permanent extrude variants) rejected by the user after live judging:
carpet-bomb `button {}` selectors turned the whole window to mud — scoped
probe (block 1f, headerbar `.raised` controls only) was needed to judge
anything. **N4 — the pressed well — won:** depth exists only during
interaction; the resting state stays the 0.5px hairline whisper. Permanent
outward soft-shadows (classic neumorphism) are rejected as the resting
material.

Scoping lesson recorded: `.raised` (+ default, non-flat) buttons are the
material carriers; `.flat`/`.osd` never wear it. This is upstream's own
flat/raised split (1.4+), so the selector pattern is native.

## Neumorphism probe — 19 Sep 2026

N1–N3 (permanent extrude variants) rejected by the user after live judging:
carpet-bomb `button {}` selectors turned the whole window to mud — scoped
probe (block 1f, headerbar `.raised` controls only) was needed to judge
anything. **N4 — the pressed well — won, and was then simplified further:**
no resting styling AT ALL. Idle buttons stay stock Adwaita; the well is
pressed-only, with the 90 ms ease.

Final pressed spec (the only button material):
  button:active (and :keyboard-activating) {
    box-shadow: inset 0 2px 4px color-mix(in srgb, black 10%, transparent),
                inset 0 -1px 1px color-mix(in srgb, white 30%, transparent);
    filter: brightness(0.96);
    transition: box-shadow 90ms ease-out, filter 90ms ease-out;  /* on base */
  }
Resting bevel: none. Ladder: none on buttons. Flat/osd: never.

## E4 delivery finding — 19 Sep 2026 (Inspector session)

This Nautilus build paints its top bar with a plain **GtkBox** — no
`headerbar` node exists in the window (upstream's `.top-bar > headerbar`
selectors target other apps). Consequences, all observed live:

- `background-color` pastes reach the box (boxes paint their own bg) —
  the yellow probes worked on the sidebar AND top bar.
- `background-image` / `background: url(...)` pastes are **silently
  dropped** — plain GtkBox widgets do not paint CSS image layers.
- The feTurbulence data-URI failure earlier was never independently
  confirmed dead; the delivery failure masked it.

**E4 resolution:** the grain experiment cannot be judged in this window —
the surface it targets doesn't exist there. Options for closing E4
properly: (a) judge the grain on a真 headerbar app (gnome-calculator,
Epiphany), (b) accept texture as dormant until M3 surfaces exist and
verify with the harness+Inspector there. Record as "E4 deferred with
reason", not failed.

**Architecture note for M3:** Nautilus's headerbar-less top bar means
L2's headerbar surface rules must target the *container pattern*
(toolbarview > .top-bar children) AND real headerbars — two selector
families, both registered in the contract.

## E4 verdict — 19 Sep 2026 (live, real provider)

Grain judged on a real headerbar via the overlay file (the only working
delivery — Inspector CSS tab drops url() layers entirely; Nautilus's top
bar is a GtkBox with no image layers at all):

- **Shimmer:** none while scrolling ✓
- **Dark mode:** 5% dark-speck tile is far too noisy on dark surfaces ✗
- **Strength:** 5% on light "seems nice" (user)

**Resolution:**
- Shipped: scheme-split tiles — `assets/grain-light.png` (dark specks,
  5%, light scheme) and `assets/grain-dark.png` (light specks, 3%, dark
  scheme — dark surfaces show noise more), selected via
  `prefers-color-scheme` in the texture token pair.
- Delivery shape (proven): `background:` shorthand + restated fill, never
  bare background-image.
- Texture stays a headerbar-only garnish (M3 wires it); HC reverts it.

Findings ledger (session total):
1. feTurbulence data-URIs render empty in app processes (librsvg).
2. Plain GtkBox surfaces drop background-image layers (Nautilus top bar).
3. The Inspector CSS tab drops url() layers — only the real user
   provider delivers images. All three now documented.

## Entry surface spec — 19 Sep 2026 (live-tuned, user-approved: "AWESOME")

Search/text entries get a permanent recessed material (the first container
depth), with a deeper well on focus — the user's model: "everything that
has depth keeps it in all states; interaction only modulates depth."

```css
entry,
entry:focus-within {
  background-image: linear-gradient(to bottom,
    color-mix(in srgb, black 5%, transparent),
    color-mix(in srgb, black 0%, transparent) 40%);   /* the scoop */
  box-shadow: inset 0 1px 3px color-mix(in srgb, black 9%, transparent),
              inset 0 -1px 0 color-mix(in srgb, white 20%, transparent);
}
entry:focus-within {
  box-shadow: inset 0 2px 5px color-mix(in srgb, black 13%, transparent),
              inset 0 -1px 0 color-mix(in srgb, white 20%, transparent);
}
```

Why v1 failed perceptually: 1px black@7% over a white fill is a ~76-point
one-row band — invisible on small fields. v2 adds the interior scoop
gradient (the eye needs the FIELD shaded, not a line) + deeper focus
(13%/5px vs 9%/3px) so focus-modulation is actually perceptible.

Landing plan (M2/M3): `ov-inset()` primitive in L1 with these values;
entry/textview surfaces in L2; upstream's focus outline coexists (it is
an outline, not box-shadow — proven compatible).

## Headerbar surface spec — 19 Sep 2026 (live-tuned, user-approved)

Final recipe (user iterated the gradient live and approved "looks nice" at
the full values):

```css
headerbar {
  background:
    url("assets/grain-light.png"),                /* grain @5%, repeat */
    linear-gradient(to bottom,
      color-mix(in srgb, white 14%, var(--headerbar-bg-color)),
      color-mix(in srgb, black 10%, var(--headerbar-bg-color))),
    var(--headerbar-bg-color);
  background-repeat: repeat, no-repeat, no-repeat;
}
```

Notes:
- First gradient attempt (4%/3%) was subliminal — a ~47px bar cannot show
  a 7-point swing; user iterated to 14%/10% ("statement headerbar").
- This is M3's headerbar surface rule, pending: HC sweep verdict, dark
  mode pass, and the widget-selector split (real headerbars + Nautilus's
  GtkBox top bar need separate delivery — the gradient shorthand shape
  works for both).

## L1 landing — 19 Sep 2026

All live-tuned recipes landed as L1 primitives (src/_primitives.scss):

- ov-bevel() — tuned hairline pair, DORMANT by default
- ov-depth-*() — container ladders, unused until a surface asks
- ov-well() + ov-press() — THE button material (N4 + 90ms ease,
  :active + :keyboard-activating, $upstream restating, HC revert)
- ov-inset() — THE entry material (scoop + recess, deeper on focus,
  HC revert)
- ov-texture() — scheme-split grain (tiles land with M3 wiring)

New governing principle recorded: "everything that has depth keeps it in
all states; interaction only MODULATES depth" (entries). Buttons are the
explicit exception (no resting material; depth appears only on press).

## Hover glow — 19 Sep 2026 (user-locked, v4)

The button state machine is complete — four states, all user-tuned live:

  rest    — stock Adwaita (nothing)
  hover   — ov-glow(): five-layer soft rim, accent edge-pooling
            (16/14/11/11% rim washes + 5% center bloom), label 45% accent
  pressed — ov-well(): amplified inset well (3px/6px, black@16%) + lip,
            brightness 0.96
  held    — ov-well-held(): light inset (1px/3px, black@8%)

Geometry odyssey recorded: flat tint (rejected, "transparent overlay") →
center-bloom radial (rejected, "inverted") → hard rim (close) → v4
layered soft rim (locked). Lesson: "glow" meant edge-pooling with smooth
multi-stage falloff, not a hotspot.

Scope: .flat/.osd excluded. HC: all four states revert flat. Contract
clean against 1:1.9.4-1 after registering button:hover.

## House motion spec — 19 Sep 2026 (user-approved)

One curve, one duration for state, one press exception:

- curve: cubic-bezier(0.25, 0.46, 0.45, 0.94) — upstream's own standard
- state changes (hover glow, entry focus deepening, checked color): 200ms
- press well: keeps the snappier feel via the same curve at 90-200ms —
  user judged the unified 200ms "elegant" in the live probe; press
  inherits it rather than keeping the 90ms exception.
- switch accent wash: 180ms ease-out (matches native knob slide).

Landed: _button.scss, ov-press(), entry transition (90ms -> 200ms pending
match), switch kept at 180ms ease-out.

## C2 performance gate — 19 Sep 2026

Measured (harness: 200-row GtkListBox, 700x500, full snapshot+render via
GskCairoRenderer, 200 iterations):

  stock Adwaita:   3.19 ms/frame
  with overlay:    2.80 ms/frame

Both an order of magnitude inside the 16.7 ms budget, and the overlay
measures slightly FASTER than stock (within noise; the row washes
replace upstream's hover work rather than adding to it at rest). The
gradient+grain on chrome surfaces and row micro-washes cost nothing
measurable. No family needs a perf-based restriction. Verdict: C2 PASSED.

## Tabbar/viewswitcher decision — 19 Sep 2026 (U6)

DEFERRED, with reasoning: tabbar/viewswitcher tabs are `.flat` buttons
inside a box that upstream itself gives a subtle inset "slot" look
(the whole point of the Adwaita 4.9 tab redesign). Our material adds
nothing they don't already have — the tabs ARE a groove in upstream's
language. Restyling them would fight upstream's structure for zero
visual gain. Revisit only if the daily drive surfaces a specific tab
that looks broken against the new material.

## HC structural audit — 19 Sep 2026

The build's material declarations (box-shadow / background-image) all sit
inside @media (prefers-contrast: more) blocks or are their reverts —
zero material declarations exist outside HC coverage. 10 HC blocks cover
the full stylesheet. Structural audit PASSED. The user's live sweep
(toggle + eyeball) remains the final gate ritual, but the structure is
proven complete.

## Light/dark matrix audit — 19 Sep 2026

Color audit of the built stylesheet:

- zero raw hex literals anywhere
- `white`/`black` keywords appear ONLY inside color-mix() derivations
  (highlight/shadow pairs, scoop gradients) — they are light-direction
  physics constants, not palette colors; every *palette* color flows
  through upstream vars (--light-1, --dark-5, --window-bg-color,
  --headerbar-bg-color, --accent-*, --view-bg-color...)
- the single rgb(from ...) is the P1.5 content-pane relative-color
  derivation (user-tuned)

Scheme behavior: gradient stops mix over scheme-aware base colors, grain
tiles switch via prefers-color-scheme, well/scoop/glow use black/white
derivations that invert meaningfully in dark. Structural dark-verification
PASSED; the eyeball pass rides with daily driving (any dark-mode artifact
lands in the daily-drive section).

## Full stylesheet complete — 19 Sep 2026

The overlay covers: buttons (4-state machine + variant dials), entries,
switches, headerbars, toolbar family, lists/cards, popovers, scrollbars,
window/panes. 238-line build from 12 source files, 30+ contract entries,
house motion spec, C2 passed, HC structurally proven, zero raw colors.

The stylesheet is COMPLETE for daily driving. Remaining gates: the user's
live HC eyeball ritual, and the two-week daily-drive (M7).

## List hover softening — 19 Sep 2026

User: hover animation on list rows should be softer without jank.

The jank trap identified first: the current implementation transitions
`background-image` (a 3-stop gradient). Gradient-to-gradient
interpolation in GSK is a re-rasterization per frame — at 200ms on a
dense list that's real work, and it's why softness has felt risky here.

Fix: transition `background-color` (GPU-trivial, GSK lerps it natively)
and make the hover state a FLAT color wash, not a gradient. Softness now
comes from three honest dials: lower peak alpha, longer duration
(280ms), and the same house curve — not from a gradient shape that the
renderer struggles with.

## System-wide micro-interaction rollout — 19 Sep 2026

User: extend micro-interaction improvements across all system components;
strictly zero visual design or material alterations.

1. Asymmetric mechanical timing applied across interactive components:
   - Hover approach: gentle 280ms on the house curve `cubic-bezier(0.25, 0.46, 0.45, 0.94)`.
   - Release / departure: clean 200ms spring-back (no sluggish lingering).
   - Active press acknowledge: 120ms immediate mechanical tactile response
     on buttons (`button:not(.flat):not(.osd):active`, `ov-press()`, suggested/destructive),
     popover menu buttons, and all list rows (`boxed-list`, `content`,
     `boxed-list-separate`, expander row headers).

2. Menu / popover jank elimination:
   - Struck the legacy 2-stop `background-image` gradient on hover (a leftover from
     before M4 list row softening) in favor of the GPU-trivial flat color wash
     (`var(--accent-color) 5%` hover, `9%` active), fully aligning menu rows
     with the list row interaction spec.

3. Zero design drift:
   - No new decorative properties, shadows, or colors added to controls.
   - Flat buttons, switches, scrollbars, and entries keep their locked designs.

## Controls surface extension (scale, progress, check, radio, toast) — 19 Sep 2026

User approved Option A across remaining candidate controls:

1. Sliders & Progress (`scale`, `progressbar`, `levelbar`):
   - Troughs (`scale > trough`, `progressbar:not(.osd) > trough`, `levelbar > trough > block.empty`):
     recessed mechanical channel with top-shade hairline (`inset 0 1px 1px -1px black@40%`)
     and bottom reflection lip (`0 1px 1px -1px white@70%`), matching the `switch` track.
   - Slider knob (`scale > trough > slider`): floating disc with top-light gradient
     and soft drop shadow (`0 2px 4px @20%`), paired with `120ms` mechanical depression
     on active drag (`0 1px 2px @25%` + inverted inset well). Disabled collapses shadow.

2. Checkboxes & Radios (`check`, `radio`):
   - Tactile microcavity (`inset 0 1px 2px black@10%`) within the restated 2px ring.
   - Gentle `280ms` hover wash (`var(--accent-color) 5%`).
   - Dry, crisp `120ms` mechanical depression (`inset 0 2px 4px black@16%`) on active click.
   - Checked state active compression (`inset 0 1px 2px black@18%`).

3. Toasts (`toast`):
   - Restated at the overlay elevation rung with 3-stop diffuse shadow
     matching suspended popovers/menus.

4. Contract & Invariants:
   - All selectors registered in `upstream/selectors.txt` and verified via `tools/check-selectors`.
   - Full HC reverts under `prefers-contrast: more`.
   - Zero raw hex; zero layout shifts.

## AdwToolbarView top-bar unstyled gap fix — 19 Sep 2026

1. The Bug:
   - In apps utilizing `AdwToolbarView` with multiple top widgets (e.g. HeaderBar + SearchBar in `gnome-extensions-app`, HeaderBar + TabBar in `nautilus`), Libadwaita assigns `.collapse-spacing` to the internal vertical `GtkBox`:
     `toolbarview > .top-bar .collapse-spacing { padding-top: 3px; padding-bottom: 3px; }`
   - Because `headerbar` was styled directly, its background started at y=3. The top 3px belonged to the parent `GtkBox`, which was unstyled, leaking the underlying window / content-pane background (especially prominent with translucent / blurred window surfaces such as Blur my Shell).

2. The Solution:
   - Follow Libadwaita's architecture by applying the surface gradient and grain to `toolbarview > .top-bar` as well as standalone `headerbar`.
   - Set `background: none` on nested `toolbarview > .top-bar headerbar` to prevent double-painting.
   - Upstream contract updated with `toolbarview > .top-bar headerbar` and `toolbarview > .top-bar.raised`.


## Sidebar surface styling (split-view & navigation-sidebar) — 19 Sep 2026

1. The Surface & Scope:
   - Split-view container pane (`.sidebar-pane`) and navigation list widgets (`.navigation-sidebar` across `row`, `child` list items, and `flowboxchild`).
   - Includes legacy tree rows, `sidebar .navigation-sidebar > row`, `placessidebar .navigation-sidebar > row`, and inline item actions (`button.sidebar-button`).
   - Dedicated surface extracted to `src/surfaces/_sidebar.scss`.

2. Material & Micro-interaction Decisions:
   - Asymmetric hover entry & exit microinteractions:
     * Entry (mouse enters item): gentle `280ms` ease on the house curve (`cubic-bezier(0.25, 0.46, 0.45, 0.94)`) declared on `:hover`. Eliminates stock Adwaita's abrupt, unsmoothed flashes when hovering.
     * Exit (mouse leaves item): clean `200ms` spring-back declared on the base item (`> row`, `> child`, `> flowboxchild`, etc.), preventing sluggish trails when skimming down navigation trees.
     * Active press acknowledgment: crisp `120ms` mechanical tactile response.
   - Jank prevention: flat `background-color` washes (6% hover, 10% active, 9% selected, 13% selected hover, 16% selected active) via `currentColor` derivations. Avoids GSK gradient re-rasterization overhead on dense navigation trees.
   - `has-open-popup`: pins the hover wash when context menus are open.
   - Micro-actions: `button.sidebar-button` (unmount, eject, add bookmark) receives matching 280ms entry, 200ms exit, and 120ms active transition washes.
   - Separators: subtle hairline division via `color-mix(in srgb, currentColor 8%, transparent)`.

3. Contracts & Invariants:
   - 31 selector atoms registered in `upstream/selectors.txt` and verified via `tools/check-selectors`.
   - Clean high-contrast reversion under `prefers-contrast: more` (transparency restores upstream's 1px HC outline).
   - Zero raw hex; dark mode and accent tracking automatic.
## Motion review — 23 Sep 2026

**Scope.** Every state transition in the stylesheet against the house
motion spec (19 Sep) — plus what the spec never asked: what GTK 4.22 can
animate at all, whether the sheet was animating what it said it was, and
what the platform's accessibility axis leaves to us.

**Method.** Two scratch probes, both kept: `tools/probe-motion.c` loads
one CSS file at priority 800 (the overlay's own mechanism), walks a real
`button` and a real `.boxed-list` row through `:hover`/`:active`/
`:focus-visible` by setting state flags, and reports the widget's
mean-RGBA at rest, at two samples into the state change and once settled
(two samples because a state change lands on the frame clock, so a slow
start must not read as "no change"). A second probe (scratch, `/tmp`)
tested keyframe replay on a real popover. Rest-state verification stays
on `tools/render-widget.c`.

### E6 — what GTK 4.22 animates (measured)

- **`var()` resolves inside the `transition` shorthand.** Both
  `transition: background-color var(--d) linear` and
  `transition: background-color var(--d) var(--e)` animated with the
  substituted values (probe: 9/255 vs 18/255 mid-flight, the second
  matching the house curve's front-load). The M2 whole-segment limit does
  not apply to single-value tokens inside a list — motion is tokenisable.
- **`@media (prefers-reduced-motion: reduce)` matches** (GTK 4.22
  `GtkCssProvider:prefers-reduced-motion` /
  `GtkSettings:gtk-interface-reduced-motion`, a separate axis from
  `gtk-enable-animations`), and a `:root` custom-property override inside
  the query propagates into `transition`. A transition declared *only*
  inside the query applies only then.
- **`gtk-enable-animations=false` already zeroes every transition** at the
  toolkit level. So the reduced-motion axis is ours to deliver.
- **No popover entry animation is possible.** A keyframe animation on
  `popover > contents` fires on the *first* map only — sampled mid-flight
  100 ms after the first `popup()` (green 27) and already at target 100 ms
  after the second (255). GTK binds animations to style computation, not
  to mapping, and the popover node survives popdown. GTK also has no exit
  animation mechanism (`@starting-style` does not exist here). Verdict:
  popovers keep popping instantly; a one-shot animation is worse than none.

### Defects found and fixed

1. **The overlay was silencing upstream's motion.** `transition` replaces
   a list, it never extends it — so every overlay rule that declared one
   dropped upstream's list for that node. Measured on a button:
   `:focus-visible` was already *landed* at the first 80 ms sample on the
   19 Sep sheet, while the fixed sheet reads rest 238 → 240,236,233 at
   80/160 ms → 240,235,232 settled (the 200 ms fade, still climbing at
   160 ms). Upstream animates the focus-ring trio
   (`outline-color`/`width`/`offset`, 200 ms, the same house curve) on
   buttons, rows, entries, switch, scale slider and sidebar buttons — the
   bare `button` atom carries it in 1:1.9.4-1. `ov-motion()` (L1)
   restates it on every declaration.
2. **`list.boxed-list` and popover menu rows had entry/exit inverted.**
   The resting rule carried 280 ms and `:hover` 200 ms, so the *entry* was
   the fast one — the opposite of the house spec, of its own comment, and
   of the later `_sidebar.scss`/`_button.scss`. Measured: the row wash
   landed by the 160 ms sample before (22 → 26), and is still climbing
   there after (21 → 24 → 26) — the 280 ms approach.
3. **The switch track's dish gradient snapped** at the toggle while the
   accent fill cross-faded over 180 ms. `background-image` is now in the
   switch list, at the switch duration.
4. **No reduced-motion path existed at all** — GTK's own animations
   toggle was the only escape.

### Decisions

- **Motion is tokenised in L0**: `--ov-motion-ease`,
  `-enter` (280 ms), `-exit` (200 ms), `-press` (120 ms),
  `-switch` (180 ms), `-ring` (upstream's own 200 ms). Values unchanged;
  what changed is that they live in one place and every declaration emits
  through `ov-motion()`, which is what makes the reduced-motion override
  structural instead of a rule per surface (the D5/D6 pattern).
- **Reduced motion ⇒ 0 ms, not a gentler duration.** Same state language,
  no animation: nothing teleports, nothing is hidden, no effect is
  removed — only the time spent getting there. This sheet animates no
  movement at all (every effect is a colour or shadow change), so
  duration is the only axis, and GTK's own `gtk-enable-animations=false`
  answers the same preference with exactly 0 ms. A third behaviour
  (shorter fades) would be indistinguishable from the platform one while
  being harder to reason about.
- **The restated ring follows the same override**: under reduce, nothing
  in this sheet animates, upstream's motion included. Tokenised as
  `--ov-motion-ring` precisely so it can be silenced with everything else
  instead of staying the one exception.
- **Not added: a transition on `switch > slider`.** The overlay paints the
  slider identically in every state (upstream's hover/active white and its
  disabled shadow are both overridden), so a transition there would
  animate nothing. Recorded because "the knob should fade too" is the
  obvious next thought.

### Verification

- Rest rendering **byte-identical** to the 19 Sep build for `button`,
  `box` (centred button) and `headerbar` — the change is timing, not paint.
- `tools/probe-motion build/gtk.css` (80 ms samples): hover 129 → 44 → 11,
  press 196 → 194, row entry 21 → 24 → 26, focus ring 238 → 240,236,233 →
  settled — every probe IN MOTION.
- Same probes with `REDUCE=1`: every state lands instantly at both
  samples; the 19 Sep sheet under the same flag still animates — the
  before/after proof for the reduce delivery. `NOANIM=1` lands everything
  instantly too.
- `tools/check-selectors`: contract OK against 1:1.9.4-1 (no selector
  changed).
- **Live pass (user, 23 Sep):** hover approach, list skim and the
  reduced-motion toggle all feel right — the eyeball half of the review,
  which no pixel probe can rule on.

### Found while reviewing (not motion, deliberately untouched)

- **Headerbar backdrop dimming is dead.** Upstream's
  `headerbar:backdrop { background-color: var(--headerbar-backdrop-color);
  transition: background-color 200ms ease-out; }` can never show, because
  the overlay's `background:` shorthand (priority 800) sets the colour in
  every state. The controls still dim (`windowhandle` filter is
  untouched); the surface does not. → BACKLOG H6.
- **`switch > slider:disabled`** keeps the overlay's full knob shadow:
  the overlay's resting rule outranks upstream's
  `switch > slider:disabled { box-shadow: 0 2px 4px transparent }`. →
  BACKLOG U7.
- **`ov-press()` (L1) is unused** by every surface — the button surface
  writes the same mechanism inline. Pre-existing; left alone.

## H6 — chrome backdrop recession — 23 Sep 2026

**Finding (measured, from the motion review).** Upstream's unfocused-window
signal was dead on every chrome surface the overlay paints:
`headerbar:backdrop { background-color: var(--headerbar-backdrop-color) }`
(and the searchbar/actionbar equivalents) can never show, because our rule
paints every state. Probe, headerbar mean-RGBA with the upstream sheet
loaded at 200: rest 233 → settled 233 (light), 232 → 232 (dark) — the
backdrop state changed nothing at all.

**Why the obvious fix is not a fix.** Re-stating `background-color` under
`:backdrop` would still show nothing: the bar material's second layer is an
*opaque* `color-mix()` gradient, so whatever is painted in the
`background-color` slot underneath it is invisible. The colour has to move
*inside* the gradient's stops.

**Implementation.** The gradient moved into `ov-bar-surface()` (L1) and its
base colour into `--ov-bar-base` (L0). A backdrop rule is then one
declaration:

    headerbar:backdrop, toolbarview > .top-bar:backdrop { --ov-bar-base: var(--ov-bar-backdrop-base); }

`--ov-bar-backdrop-base: var(--headerbar-backdrop-color)`, which upstream
defines as `@window_bg_color` — the chrome recedes into the content, on the
house transition (`background`, 200 ms, reduce-aware). Side effects, all
intended: the dark grain URI now comes from `--ov-texture-image` instead of
being respelled (the texture kill switch now works in dark mode too), and
the duplicated gradient stack in `_headerbar.scss`/`_toolbar.scss` collapsed
into one definition.

**Measured effect** (probe, upstream loaded, forced `:backdrop`):

| scheme | resting base | backdrop base | probe mean (rest → settled) |
| --- | --- | --- | --- |
| light | `#ffffff` | `#fafafb` | 233 → 232 — **below the noise floor** |
| dark | `#2e2e32` | `#222226` | 232 → 230 → 228, progressive (animating) |

So the trade is a whisper in light mode and a real (stock-Adwaita) dim in
dark mode, where the chrome stops being the brightest thing on an unfocused
window.

**Kill switch** (one edit, D5 pattern): set
`--ov-bar-backdrop-base: var(--headerbar-bg-color)` in `_tokens.scss` and
the bar stays lit in every state.

**Contract.** `--headerbar-backdrop-color` added to `variables.txt`;
`headerbar:backdrop`, `searchbar > revealer > box:backdrop`,
`actionbar > revealer > box:backdrop` and `toolbarview > .top-bar.raised:backdrop`
added to `selectors.txt`; guard passes against 1:1.9.4-1. Rest-state means
are unchanged (233/232 identical to the pre-change sheet).

**Verdict: KEPT** (user, live look 23 Sep 2026) — the dark-mode recession
reads right; light mode is unchanged in practice. BACKLOG H6 closed.

**Out of scope, same family.** `.sidebar-pane:backdrop` is also dead (our
`.sidebar-pane` is deliberately `transparent`), but restoring it would mean
an opaque colour where the translucency is a user decision from P1.5 —
a different trade, not taken here.

## Widget-family sweep — 23 Sep 2026

**Scope.** "Apply the recorded design language to all GTK4 widgets" — every
family the first pass (M0–M5) left outside the material system, judged on
real widgets through `gtk4-widget-factory` and `gtk4-demo`, tracked per
family by the new `tools/track`.

**Method.**
1. **Inventory from the sheet, not from widget names.** The pinned 1:1.9.4-1
   stylesheet was parsed rule by rule and every declaration of material
   (`box-shadow`, `background-image`, `background`, `background-color`,
   `filter`) enumerated per node family, in base, dark and HC. Coverage
   decisions are therefore stated in terms of upstream's own declarations.
2. **Fixes are written in the existing vocabulary** — L0 tokens, the L1
   functions (`ov-bevel`, `ov-well`, `ov-glow`, `ov-inset`, `ov-depth-*`,
   `ov-bar-surface`), house motion through `ov-motion()`. No new token was
   needed; `ov-depth-window()` (recorded in Evening 0, unused until now)
   finally has a surface: sheets.
3. **Contract.** Every new selector atom registered in
   `upstream/selectors.txt` (86 → 164 entries).
   `tools/check-selectors` passes against the installed 1:1.9.4-1.

### Defects this sweep found — all of them our own, all fixed

**1. The button material leaked onto upstream's flat families.** The guard
was `button:not(.flat):not(.osd)`, but upstream paints whole families flat
*without* the class: bar icon buttons (`headerbar`/`searchbar`/`actionbar`/
`.toolbar` + `.image-button`/`.arrow-button`/`.image-text-button`), the
wrapper-guarded `menubutton`/`splitbutton` children, `windowcontrols`,
`tabthumbnail`, `notebook` arrows, `columnview`/`treeview` headers,
`calendar` header, `infobar .close`, popover model buttons, spinbutton
arrows, pathbar crumbs, bottom-sheet actions. Those buttons were getting
the bevel, the accent glow and the pressed well — including `windowcontrols`,
which the proposal lists as out of scope.
*Fix:* the flat families are neutralised by property (`box-shadow`,
`background-image`, `filter`) in `_button.scss`, generated from two selector
lists. The reset selectors carry ≥8 classes, which is what makes one
non-state rule outrank the richest material rule (7 classes) at equal
priority — no `!important`, no per-state explosion. Upstream's own
transition list is restated on the same rule, so their washes still fade.
*Found later the same day, while rendering the hover register:* `button.link`
belongs on that list too. It is not flat by a `background: none` rule — it is
flat because upstream never gives it a surface at all (`button.link { color:
accent; text-decoration: underline }`), so a body-scan for "background:
none" could not find it. With the material on, a link rendered as a
full-width glow chip; `STATE=prelight build/render-gallery build/gtk.css
… buttons` shows the before/after, and `button.link` is now registered in the
contract.

**Verdict on the scope — KEPT (user, 23 Sep 2026).** The accent hover glow
was demonstrably *not* lost on opaque buttons (`hover` mean 228 → 19 under
the overlay, `Suggested`/`Destructive` bloom intact), but the earlier pass
took it off the families upstream paints flat. Asked explicitly whether to
restore it on bar icon buttons — glow only, or bevel+glow+well — the answer
was **keep it as it is**: the 19 Sep scope stands, bar icons and the other
flat families stay flat, and the neon register lives on opaque buttons and
coloured CTAs. The two alternatives stay rendered for reference
(`/tmp` scratch; regenerate with `STATE=prelight build/render-gallery …`).
Reopening it means deleting `$ov-flat-bar-contexts` from the loop in
`_button.scss` — the lever is named here so the decision is one edit wide.

**2. `button:drop(active)` was erased.** Upstream marks a drop target with
`box-shadow: inset 0 0 0 2px var(--accent-bg-color)`; the overlay's resting
rule owns `box-shadow` at priority 800, and priority decides before
specificity, so the accent ring never rendered on any non-flat button. Same
for `entry:drop(active)`, `spinbutton:drop(active)` and the generic
`:not(window):drop(active)` that `.card` relies on.
*Fix:* every material selector carries `:not(:drop(active))`, so the state
is handed back to upstream's own rule untouched. No restatement needed —
that is the point.

**3. `.card` lost its definition ring.** Our card ladder replaced the whole
`box-shadow` list, dropping upstream's first stop, `0 0 0 1px RGB(0 0 6/3%)`
— the 1px ring outside the card's edge.
*Fix:* the ring is restated ahead of the ladder and deliberately does **not**
ride `--ov-depth-color` (it is definition, not depth, so the depth kill
switch must not remove the edge).
*Measured* (gallery `lists` family, vertical profile through the card's top
edge at x=120, grey value of the boundary pixel, page 255): stock 225,
pre-fix card rule 206, fixed 193 — so the ring was really gone and is really
back, but the practical damage was smaller than it looked on paper: the
overlay's own translucent window surface (`--ov-surface-window`, 84% then —
96% since the retune of 23 Sep 2026) already separates a white card from the
page. Recorded as a real defect with a modest user impact, not as the
near-invisible card the source alone suggested.

**4. `switch > slider:disabled` kept a raised knob** (BACKLOG U7): our
resting slider rule outranked upstream's
`switch > slider:disabled { box-shadow: 0 2px 4px transparent }`, so a
disabled switch read as interactive.
*Fix:* disabled is flat — no drop, no insets, no gradient; the state stays
legible because the track keeps upstream's own dim.

**5. A checked switch lost its hover and press feedback.** Upstream
modulates `switch:checked` with a second `image()` layer on `:hover` and
`:active`; our dish gradient is declared for every state, so it swallowed
that layer entirely.
*Fix:* both states restated with upstream's own layer underneath our dish.

**6. `filter: none` on disabled buttons erased upstream's dim.** Several
families (every bar button family, `label`, `scale`, `switch`) dim with
`filter: opacity(...)`; a `filter: none` at priority 800 wiped it, so a
disabled icon button read as enabled.
*Fix:* the disabled rule now neutralises only `box-shadow` and
`background-image`. Nothing else of ours may declare `filter` outside the
press state.

**7. `spinbutton` never received the entry material.** The BACKLOG recorded
"the spinbutton's text area IS an entry node" as the reason U2 needed no
work. It is not: in GTK 4 a spinbutton's node is `spinbutton` with
`spinbutton > text`, a sibling of `entry`. Entries were recessed; every
spinbutton (GtkSpinButton, AdwSpinRow) stayed stock.
*Fix:* `_entry.scss` applies `ov-inset()` to both, because upstream gives
them identical bodies (`widgets/_entry.scss` vs `widgets/_spin-button.scss`).

### The high-contrast audit was passing for the wrong reason

Applying the language to the new families meant auditing their reverts, so
the 19 Sep "structural audit PASSED" claim was re-run **properly** this
time: parse the built sheet, take every rule that declares material
(`box-shadow` / `background-image` with a value other than `none`) outside
a `prefers-contrast` block, and require an HC rule whose selector is at
least as specific. The 19 Sep audit had only checked that HC blocks
existed, so it never noticed that a revert with *fewer* classes than the
rule it reverts loses: HC is the same provider at the same priority, and
priority ties are broken by specificity, then by source order.

Eight material declarations were surviving HC as a result:

| Declaration | Why the revert lost |
| --- | --- |
| `button:hover` glow | HC listed `…:hover` without the `:not(:active):not(:checked):not(:disabled)` tail (4 classes vs 7) |
| `button:active` well, `.keyboard-activating` | the press variant was not listed at all |
| `button.suggested-action`/`destructive-action` hover, active, checked | one revert sat at the end of the rule, covering only the base selector; nested states carry their own selectors |
| `entry:focus-within` deep scoop | `ov-inset()`'s revert covered the resting selector only |
| `scale:active > trough > slider` well | HC listed the resting knob only |
| `check`/`radio` microcavity | HC listed `check`/`radio` bare (0 classes) against a 2-class guard |
| `list.boxed-list` container scoop | no revert existed at all |
| `switch:checked:hover/:active` (added in this pass) | same trap, caught before shipping |

**Fix.** Every material rule now carries a revert whose selector matches it
character for character, and each revert restates what the material took:
the button HC list was rewritten per state, `ov-inset()` grew a
`:focus-within` sibling revert, the accent glass gained `ov-accent-solid()`
repeated through each state, and the boxed-list scoop and the
scale/check cases got their own. **Audit result: 0 material
declarations without a matching revert** (was 8).

Two further HC defects surfaced only once the reverts were *measured*
rather than read, both from reverting with `none` where a restatement was
owed:

- **`background-image: none` on the resting button erased a fill that the
  sheet underneath delivers as a gradient.** Measured with
  `tools/probe-motion` against the system GTK theme (a plain-GTK app:
  `gtk4-demo`, `gtk4-widget-factory`, and every non-libadwaita app):
  probed button mean `rgb(238,239,240)/α252` at rest → `rgb(15,16,15)/α40`
  under HC. libadwaita's own buttons carry **no** `background-image` at
  all — rest/hover/active are `background-color: color-mix(currentColor
  10% / 15% / 30%, transparent)` (verified in the pinned sheet) — so a
  libadwaita app never saw this. The revert now names only what the
  material declares: background-image is reverted on the hover state,
  which is the only state that sets one.
- **`box-shadow: none` erased upstream's own HC ring.** Under HC the ring
  `inset 0 0 0 1px color-mix(currentColor var(--border-opacity))` is the
  button's boundary; the revert now restates that expression, so the ring
  keeps following the palette.

### The harnesses were measuring the wrong baseline (found 23 Sep)

`tools/render-widget`, `tools/probe-motion` and the new
`tools/render-gallery` all load one CSS file at priority 800 — but GTK
*also* loads `$XDG_CONFIG_HOME/gtk-4.0/gtk.css` for every process, and on
a machine that has installed this overlay that path is a symlink to the
sheet under test. Every "stock" run therefore already carried the overlay:
proved by `md5sum` on two gallery runs, `none` versus `build/gtk.css`,
which were byte-identical (`672472ad907272f21cc92b090e29981f` for the
headerbar family). The A/B was measuring one file twice.

All three harnesses now point `XDG_CONFIG_HOME` at a private empty
directory before `gtk_init()`, so the CSS arguments are the only
stylesheets in the process; `KEEP_CONFIG=1` restores the user environment.
After the fix the same pair reports 15/15 families changed
(`tools/gallery-diff`, light scheme). libadwaita's own stylesheet is
unaffected — it arrives from the theme search path, not from the user
config.

### The language applied to the families the first pass missed

| Family | Change | File |
| --- | --- | --- |
| Content cells — bare `row.activatable`, `flowbox > flowboxchild`, `gridview > child.activatable`, `popover.menu list > row` / `listview > row` | House timing only (280 in / 200 out / 120 press). Upstream's alphas and the accent selection pair are untouched: the recorded system-wide rollout was explicitly "timing only, zero material alterations". | `_cells.scss` |
| Notebook tabs | House timing only, on upstream's own wash. The U6 deferral stands: no material added. | `_notebook.scss` |
| `expander-widget` titles | The row wash (5% / 9%) **added**, with house timing — the one place material was added rather than re-timed, because upstream's only feedback is the arrow's opacity and the title is an activatable row. | `_expander.scss` |
| `calendar > grid > label` | House timing on the pointer wash (`:checked`). Selection semantics (`:selected` accent) untouched. | `_calendar.scss` |
| `bottom-sheet > sheet`, `floating-sheet > sheet` | The window rung of the ladder (`ov-depth-window()`) — the first surface on the rung recorded in Evening 0 for "windows". Follows `--ov-depth-color`, so the depth kill switch reaches sheets. Upstream's `outline` hairline stays. | `_sheet.scss` |

### Reviewed and deliberately NOT changed (evidence first)

- **`.view` and `textview > text` — the container scoop was tried and
  reverted.** The only node carrying upstream's view fill is content-sized,
  so a `background-image` there scrolls with the content instead of sitting
  in the viewport. Probed (scratch `probe-view.c` / `probe-textview.c`,
  red→blue gradient on the node, 300×200 viewport, two scroll positions):
  textview content 7218 px — scroll 0 renders the top band `252,0,0` and
  scroll 1 `6,0,245`; treeview `.view` content 8400 px — the middle band
  moves `51,0,201 → 170,0,79` between the two positions. A shade that moves
  while scrolling is the exact failure the E4 texture verdict rejects
  ("no shimmer while scrolling"), so the file was deleted rather than
  shipped. GtkTextView and view bodies keep upstream's flat fill.
- **Tooltip.** Upstream sets `tooltip { box-shadow: none }` deliberately —
  tooltips are flat dark bubbles. Putting them on the overlay rung would
  add depth upstream removed on purpose.
- **Keycaps** (`shortcut > .keycap`, `shortcut-label .keycap`). Already the
  house language: `inset 0 -2px var(--card-shade-color)`, i.e. shade on the
  bottom edge from an upstream token.
- **Paned separators and scroll undershoots.** Already hairlines and
  alpha-shade gradients in the same idiom as ours
  (`color-mix(currentColor var(--border-opacity))`,
  `color-mix(var(--shade-color) 75%)`). Restating them would be churn.
- **AdwTabBar / AdwViewSwitcher / tabthumbnails.** The U6 deferral stands.
  The only rule of ours that reaches them is the flat reset, which *removes*
  our material from `tabthumbnail button`.
- **GtkCalendar — harness verdict taken, live verdict impossible here.**
  No demo page shows one (`gtk4-demo --list`, 112 examples; the widget
  factory's own set — neither carries a calendar), so the family was judged
  in `tools/render-gallery`'s `calendar` family instead: stock vs overlay at
  rest 75785/129600 changed pixels (6.65 mean, all of it window and bar
  surface — the calendar node itself is untouched at rest), and
  `STATE=prelight` shows the header arrows taking upstream's own hover wash
  with our timing on it. Light, dark and HC renders all read clean, day
  numerics, the `:selected` accent and the `today` underline included.
  A live witness does not exist on this machine: `/usr/bin` and `/usr/lib`
  were scanned for `gtk_calendar_new` / `GtkCalendarPopover` and the only
  carriers are `telegram-desktop`, `yad`, `gtk4-icon-editor`, libgtk and
  libwebkit2gtk — no GNOME surface. Verdict: **no regression, timing
  accepted on harness evidence**; reopen if an app that shows one enters
  daily use (BACKLOG W11 closed with this reason).
- **`upstream/selectors.txt` header.** The 12 comment lines of its header were
  in a shuffled order (pre-existing — the same order is in HEAD, so it came
  in with an early commit): the contract's own prose read backwards and in
  fragments. **Fixed 23 Sep 2026:** the fragments were reordered into the
  intended reading order and checked word-for-word against `docs/proposal.md`
  ("Rules" section, hard rule 2), so no text was added, removed or reworded —
  `sorted(header) == sorted(HEAD header)` and `tools/check-selectors` still
  passes. The one ambiguous fragment ("silently") was placed per the
  proposal's own sentence: "An unregistered dependency is invisible to the
  upgrade guard and will break silently."

## Gallery — 15 families, stock vs overlay

`tools/render-gallery` (new, 23 Sep) renders one window per family offscreen
with the same provider mechanism as the rest of the toolchain and writes one
TIFF per family; `tools/gallery-diff` reports `changed_px/total_px`,
`mean_abs_delta` and `max_delta`. Light scheme, `build/gtk.css` vs no
provider at all (both runs hermetic — see the baseline note above):

| Family | changed/total | mean Δ | max Δ |
| --- | --- | --- | --- |
| adw (toolbarview, banner, tabbar, status page) | 340409/396800 | 9.45 | 40 |
| buttons | 297510/298080 | 14.67 | 255 |
| calendar | 75785/129600 | 6.65 | 40 |
| cells (flowbox, gridview) | 100227/316960 | 3.63 | 40 |
| columns (columnview + headers) | 112681/210800 | 6.12 | 40 |
| controls (switch, scale, progress, level, scrollbar) | 305877/312000 | 10.86 | 62 |
| dnd (drop-active button + entry) | 174815/176800 | 11.36 | 57 |
| entries | 278633/279360 | 12.45 | 63 |
| expander | 153173/153600 | 11.14 | 40 |
| headerbar | 111415/112000 | 11.74 | 40 |
| lists (listbox, boxed-list, card) | 193842/291200 | 7.30 | 40 |
| notebook | 219860/228800 | 10.82 | 40 |
| popover | 198143/218400 | 10.48 | 40 |
| spinbutton | 146786/147200 | 12.55 | 63 |
| textview | 158782/239200 | 7.41 | 40 |

15/15 families changed in every scheme (light, dark, HC). Two readings to
keep honest: the delta proves the overlay *reaches* a family, not that the
result is right — that is what the image review and the live pass are for;
and under HC the remaining delta is the *surface* set (translucent window
and panes), which is deliberately not reverted, because a surface tint is
not a material effect.

### Verification

- `tools/check-selectors`: contract OK against the installed 1:1.9.4-1 —
  169 entries, both axes (86 → 169; every new atom taken from the pinned
  sheet, not invented, `button.link` and the four channel-fill atoms last).
- `tools/build`: compiles, 861 lines, `!important` count 0, raw hex
  count 0.
- Structural HC audit (script above): 0 material declarations without a
  matching revert (53 material selectors in the final sheet).
- Motion matrix (`tools/probe-motion`, 80 ms samples, upstream sheet
  loaded, hermetic baseline): normal, `SCHEME=dark`, `CONTRAST=more`,
  `REDUCE=1` and `NOANIM=1` all report the expected verdicts — the press
  well, the row wash and the focus ring are IN MOTION in the animated
  runs, and every probed state lands instantly under `REDUCE=1` and
  `NOANIM=1` (the reduce/no-animation runs were repeated *with* the
  upstream sheet at the end of the session: without it the headerbar row
  is a no-op, because `--headerbar-bg-color` is undefined and the whole
  background declaration is invalid at computed-value time — the README
  warns about exactly this, and the first pass of this matrix skipped it).
  The hover glow lands inside the first sample in both schemes, which is
  the state the 23 Sep motion review recorded (a background-image swap,
  not a fade).
- Gallery (`tools/render-gallery`, 15 families, stock vs overlay, diffed
  by `tools/gallery-diff`) re-run on the final sheet: 15/15 families
  changed in light (largest: adw, 340409/396800), dark (338378) and HC
  (319485) — numbers below.
- Contrast of every pair this pass touches, computed from the palette the
  pinned sheet defines (translucent colours composited over the surface
  they render on, not estimated):

  | Pair | Light | Dark |
  | --- | --- | --- |
  | body text on window | 12.22:1 | 15.85:1 |
  | text on a row wash at hover (5%) | 11.20:1 | 13.71:1 |
  | text on a row wash at press (9%) | 10.42:1 | 12.06:1 |
  | text on the check/radio cavity at press (30%) | 6.87:1 | 5.87:1 |
  | destructive label on the HC solid fill | 4.83:1 | 6.11:1 |

  Every pair clears 4.5:1; the tightest is the destructive label on its
  solid fill, which is upstream's own pair (`--destructive-bg-color` /
  `--accent-fg-color`) and unchanged by this pass. The row/expander washes
  move the text contrast by at most 1.5 points, so the new expander wash
  costs nothing legible.
- Visual: the `headerbar`, `columns`, `lists` and `spinbutton` families
  were read as images in both runs. The bar's icon buttons and the column
  headers are indistinguishable from stock (the flat-family reset), the
  card edge is present in both, and the spinbutton carries the new inset
  while its up/down arrows stay flat.
- Live pass: `tools/track <family>` opens the demo page for each family;
  `tools/track <family> -i` opens the same page under GTK Inspector. The
  user's eyeball verdict rides with daily driving (M7).

## Lit channel fills — 23 Sep 2026

**Request.** "Progress bar is still looking rather flat (the coloured bits)."

**Diagnosis.** Correct, and by construction: upstream paints every filled
channel — `progressbar > trough > progress`, `scale > trough > highlight`
(one shared rule upstream, so one shared material here) and
`levelbar > trough > block` — with a flat `background-color` (accent,
`--warning-bg-color`, `--success-bg-color`) and nothing else. Ours only
recessed the trough around it, so the fill sat in a groove as a sticker.

**Decision.** A new L1 register, `ov-lit-fill()`, and **the colour stays
opaque underneath it**. Two reasons: a progress bar's colour is data, and a
levelbar's low/high/full distinction is meaning — diluting either to make
glass would trade information for shine. So the lighting is layered on top
(one light source, top): a 3-stop top-light gradient plus a 1px lit lip and
shadowed foot, in white/black physics constants only, exactly like
`ov-raised()` and `ov-glow()`. No kill switch of its own (the glow has
none either); the HC revert is the escape hatch, and it flattens the fill to
upstream's solid colour.

Values are the CTA glass's own curve (white 28% → 6% at 42% → black 10%),
compressed for a 4-12px bar and tuned by measurement: the first pass
(white 22% / black 10%) moved the top row from `rgb(53,132,228)` to
`rgb(146,189,241)` — one lit pixel out of four, still flat to the eye at 1×.
The shipped pair (30%, 8% at 45%, black 14%; insets white 55% / black 22%):

| node | before (every row) | after (top → foot) |
| --- | --- | --- |
| `scale > trough > highlight` | `rgb(53,132,228)` | `rgb(186,213,246)` → `rgb(41,95,161)` |
| `progressbar > trough > progress` | `rgb(53,132,228)` | `rgb(189,214,246)` → `rgb(39,92,158)` |
| `levelbar > trough > block` | `rgb(53,132,228)` | `rgb(189,214,246)` → `rgb(39,92,158)` |

(measured in the gallery `controls` family; each selector was first proved to
hit a painted node by painting it a flat probe colour and locating the
pixels, the same technique the textview question used.)

**Scope.** `progressbar.osd` and an empty trough are excluded, because
upstream unsets the fill there (`progressbar > trough.empty > progress { all:
unset }`). Motion: upstream animates `background` and `box-shadow` on these
nodes; both are restated through `ov-motion()`, plus `background-image`
(ours), so the colour fade keeps the house exit timing.

**Defect found in passing.** The channel's HC revert set `box-shadow: none`,
which erased upstream's own 1px HC ring on `scale > trough`,
`progressbar > trough` and `levelbar > trough > block.empty` — the third
instance of the same trap (buttons, card, now channels). The ring is
restated instead. Only the ring: we never touch the channel's
`background-color`, so upstream's HC value applies untouched.

**Contract.** +4 atoms (`scale > trough > highlight`,
`progressbar > trough > progress`, `progressbar > trough.empty > progress`,
`levelbar > trough > block:not(.empty)`), 165 → 169; guard passes.

**Verdict: KEPT** (user, 23 Sep 2026) — "nice and subtle, approved". The
register ships as measured above; the tuning lever stays `ov-lit-fill()` in
`_primitives.scss`, one edit wide, with the variant numbers on record if it
ever wants to be stronger (the bevel-only and 42%-top variants were rejected
on measurement, not on taste).

**Verification.** Gallery `controls` in light and dark (identical output —
the accent is scheme-independent, so the dome is too), HC flat at
`rgb(53,132,228)` on every row, contract OK, sheet parses, 861 lines, 0
`!important`, 0 raw hex. Live verdict rides with the user's eye:
`tools/track factory` (progress bar) and `tools/track style-classes`.
Rejected on measurement: the bevel-only variant (no gradient, hairlines
only) — crisp but it loses the dome — and a 42%-top gradient variant, which
dipped *below* the base colour through the middle.

## Button review — glass retired, ring thinned — 23 Sep 2026

Trigger (user): *"I'm not sold on the buttons… use the appropriate design
skills to review their implementation"*, then *"the 'glass' effect on the
buttons is not really very polished"*, and *"use thinner borders on
outlined buttons"* — clarified to mean the keyboard focus ring.

**Method.** Design-skill review (better-interface, routed through better-ui /
better-colors / better-accessibility) run against the *rendered* surface, not
the source: gallery `buttons` family, stock vs overlay, five states ×
light/dark/HC, the CTA block at 2×, plus a numeric pass over each label pair,
the focus-ring pixel profile, and the pixels *between* two adjacent CTAs.

### 1. The accent glass is retired (was "Accent luminous glass", 19 Sep)

Label foreground against the fill it actually renders on. 4.5:1 is the
requirement for a 13px bold label; 3:1 is the large-text floor.

| state | suggested stock | suggested glass | destructive stock | destructive glass |
| --- | --- | --- | --- | --- |
| rest  | 3.77 | **2.00** | 4.60 | **2.58** |
| hover | 3.27 | 2.27     | 4.15 | 3.12     |
| press | 5.46 | 3.12     | 3.06 | 4.20     |

One root cause — one lighting recipe painted over two different upstream
materials — produced three defects:

- **The 55% translucent fill is a function of the backdrop.** On the light
  scheme's near-white window it leaves a pale ghost (2.00:1, *below* even the
  3:1 floor); the same 55% over the dark window measures 5.44:1. A material
  whose label contrast swings with whatever sits behind it cannot be called
  polished.
- **`.destructive-action` is not a filled variant upstream.** It remaps
  `--accent-*` and paints a **15% `currentColor` container** with its own hue
  as the label, plus 20/35/45% washes on hover/active/checked (gtk.css
  L318-328). The glass's blanket `color: --accent-fg-color` turned that label
  white on a saturated red slab: an emphasis inversion (the destructive CTA
  out-shouting the accent one) *and* a 2.58:1 pair.
- **The hover/press outer bloom** (`0 4px 14px -2px` / `0 12px 28px -6px`)
  breaks the same-surface law stated in `_primitives.scss` ("buttons never
  do"), and painted into the 10px gap between two CTAs: one pixel between
  them, both prelight, moved 249,249,250 → 200,211,236 in light and
  33,33,37 → 44,57,85 in dark. It had never been decided — no entry in this
  file, only incidental mentions.

**Decision.** `.suggested-action` wears the house lit fill instead:
upstream's own **opaque** `--accent-bg-color` under `ov-lit-curve()` at button
scale, with the mid stop at **zero alpha** so the band the label sits in
keeps the base colour (the channel curve's white-8% mid cost 0.6 of contrast,
3.77 → 3.14). Upstream's own hover lift (`image(currentColor 10%)`), press
darkening (`image(RGB(0 0 6/20%))`) and held darkening
(`image(RGB(0 0 6/15%))`) are kept as the top layer, because our resting
declaration replaces their `background-image`; press and held then sink into
the same `ov-well()` / `ov-well-held()` every other button wears.

`.destructive-action` gets **no CTA material at all** and falls through to the
generic button material. That restores upstream's container, and re-hues the
house hover glow for free — upstream remaps `--accent-color` to the
destructive hue on that node (L318) and `ov-glow()` reads exactly that
variable.

| light | stock | glass | now |
| --- | --- | --- | --- |
| suggested rest / hover / press / held | 3.77 / 3.27 / 5.46 / 4.99 | 2.00 / 2.27 / 3.12 / 3.12 | 3.95 / 3.42 / 5.58 / 5.18 |
| destructive rest / hover / press / held | 4.60 / 4.15 / 3.06 / 3.06 | 2.58 / 3.12 / 4.20 / 4.20 | 4.35 / 3.46 / 2.93 / 2.78 |

Suggested is now at or *above* upstream on every state. Destructive's
hover/press/held sit 0.2-0.7 below stock because the house glow and well are
*added* to a container whose own upstream pairs are already 3.06; that trade
— the house hover language on every button versus 0.7 of label contrast on
one state of one variant — is recorded as an open item (BACKLOG B3), not
hidden. HC: `ov-lit-fill()`'s own revert hands the CTA back to upstream's flat
fill (suggested rest = exactly stock, 3.77).

### 2. Focus ring: 2px → 1.5px

User decision. Upstream's ring is 2px of `accent 50%` (gtk.css L240 — the
button focus rule whose selector list ends in the bare `button` atom — plus
per-family re-sets in the bar / CTA / flat rules). **One** rule at priority
800 (`src/surfaces/_button.scss`) sets
`outline-width: var(--ov-focus-ring-width)` for every button family at once,
flat families included, so a bar icon button and its opaque sibling cannot
drift apart. New kill-switch token `--ov-focus-ring-width` (`1.5px`; `2px`
restores upstream). Colour, per-family offset and upstream's own outline
transition are untouched, so the ring still fades in at `--ov-motion-ring`.

Rendered profile at 1×: 2px covers two device rows solid (117,163,210 twice,
light scheme); 1.5px covers one solid row plus one half-intensity outer row
(170,193,218 then 106,156,206) — the ring keeps its edge anchoring and loses a
quarter of its ink. HC restores 2px (verified: every width variant renders
92,141,192 + 106,156,206 under `CONTRAST=more`).

Known trade, stated rather than buried: the focus-appearance floor is a 2px
perimeter, and stock's 2px at 50% alpha is already only ~1px of solid ink;
1.5px keeps ~0.75px-equivalent. The same one token is the lever for a crisper
hairline at fractional scale (`1px`) and for the floor (`2px`).

**Contract.** +1 atom (`button:focus:focus-visible`), 169 → 170; guard passes
against the installed sheet and the pinned archive.

### Verification

- **Blast radius.** Gallery, old sheet vs new sheet, eight families:
  `buttons` changed (30271 px — the intended restyle), `controls`,
  `notebook`, `lists`, `popover`, `adw`, `headerbar`, `textview`
  **pixel-identical** (the `ov-lit-curve()` refactor is neutral).
- **Gallery `buttons`.** stock vs overlay × rest / hover / press / held /
  focus-visible × light / dark × normal / HC; every label pair measured as
  tabulated above; the gap pixel between the two CTAs is identical at rest
  and at prelight in both schemes (bloom gone).
- `tools/check-selectors`: OK against installed `1:1.9.4-1` and against the
  pinned archive.
- **Not verified:** a checked *or* destructive CTA in a live app (no gallery
  member wears both flags), and the ring at 1.25×/1.5× scale (X5 card).

### Review findings not acted on

Recorded in BACKLOG under "Button review" (B3-B7): destructive hover/press
contrast cost, the 280ms hover entry on a high-frequency control, the
`:disabled` `background-image` reset on plain-GTK buttons, the asymmetric
bevel pair in the light scheme, and checked-toggle press feedback.

## Window translucency: 84% → 96% — 23 Sep 2026

**Report.** A window over another window (the Extensions app over a browser):
the page's own body text read through the window's content, in the space
where the app paints nothing (list gaps, page padding — most of a plain
window). Not a new rule: the migrated P1.5 alpha.

**Cause.** `--ov-surface-window` (L0) mixes the window colour 84% with
transparent, i.e. 16% of whatever is behind the window reaches the screen.
On a light page over a dark text run that is ~38/255 of contrast — the eye
resolves that as text, not as tint.

**Measured** (gallery `headerbar`, the family that renders a real window,
bare-surface pixels). Surface alpha 215/255 = 0.843 — the token exactly; the
header bar over it stays opaque (255). Composited over #0f1419 text on a
#ffffff page: the ghost reads 213/255 against a 250/255 surface, 37/255 of
contrast. Same in dark (rgb(33,33,37) body).

**Decision.** 96% — one number in L0. Bleed drops to 4% and ghost contrast
to 9/255 (~3.5%, under the legibility floor), while the surface still takes
the backdrop's cast: the translucency survives as a tint. `dialog` rides
the same token; `.content-pane` already derives opaque and `.sidebar-pane`
is transparent, so both follow the window and neither needed an edit. No
contrast variant is added: HC must not remove surface definition, and this
direction is toward opaque anyway.

**Verification.** render-gallery `headerbar`, light and dark: surface alpha
245/255 in both, bar unchanged at 255 (`/tmp/ov-before`, `/tmp/ov-after`,
`/tmp/ov-after-dark`). `tools/gallery-diff` old→new: `headerbar`
86188/112000 changed, mean 6.11, max 30 (the alpha channel is compared).
`tools/build` reinstalled the sheet and restarted the two service daemons,
so windows opened from here get it. **Not verified:** a live window over
another window after the rebuild — the compositor path is the one the
report itself exercised at 84%, and the render-node alpha is the only
thing that changed.

## Foreign apps: Helium came up black — 25 Sep 2026

**Report.** Helium 0.18.1.1 (Chromium 154, `imputnet/helium`) with its default
`extensions.theme.system_theme = 1` (`ui::SystemTheme::kGtk`, i.e. "follow the
GTK theme") opened with a black tab strip, a black toolbar and a black
viewport: a completely dark browser window in a light session. Helium is a
plain GTK4 app — it never loads libadwaita's stylesheet, so none of our
`--ov-*` tokens derived from libadwaita custom properties resolve in it.

**Cause.** A libadwaita custom property exists only when libadwaita's sheet is
loaded. In an app that never calls `adw_init()` the reference is undefined, and
GTK does **not** fall back to the theme's own colour for that property: the
declaration computes to nothing and the node paints no background at all. So
`window { background-color: var(--ov-surface-window) }` and the bar material
(which removes upstream's gradient and repaints from `var(--headerbar-bg-color)`)
both rendered as nothing.

Chromium turns "nothing" into black. It builds its Linux palette out of
*rendered* GTK nodes — `ui/gtk/gtk_util.cc` `GetBgColor()` renders the node
into a 24x24 cairo surface and averages it, `ui/gtk/gtk_color_mixers.cc`
consumes the result — and forces the frame opaque:

```c
frame_color = SkColorSetA(GetBgColor("headerbar.header-bar.titlebar"),
                          SK_AlphaOPAQUE);
```

An empty render averages to `0x00000000` (`a == 0` in `GetAveragePixelValue`),
`SkColorSetA(..., 255)` makes it `#ff000000`, and `kColorPrimaryBackground`
(averaged `window.background`) stays transparent: black frame, black toolbar,
black viewport.

**Measured — `tools/probe-foreign` (new, offline, exits 1 on any input that
paints nothing), same selectors and averaging as `gtk_color_mixers.cc`:**

| input | pre-fix sheet | fixed sheet | stock GTK |
| --- | --- | --- | --- |
| `GetBgColor("")` (primary bg) | `#00000000` | `#f5f6f5f4` | `#fff6f5f4` |
| `opaque(bg(headerbar))` (frame) | `#ff000000` | `#ffe4e3e2` | `#ffdddad6` |
| `bg("") over frame` (toolbar) | `#ff000000` | `#fff5f4f3` | `#fff6f5f4` |
| inputs that paint nothing | 9 of 31 | 0 | 0 |

**Live A/B in the running session** (second Helium instance, `--gtk-version=4`,
`system_theme=1`, fresh profile, same 900x600 window, captured per window;
sheet swapped by `XDG_CONFIG_HOME`): window pixels classified as pure black —
stock 36367, pre-fix 79598, fixed 36386; the top chrome rows (y=10-19) read
`#eeedeb` / `#000000` / `#ededec`, i.e. the fix is stock-identical, not merely
"less black".

**Decision.** L0 gains an explicit upstream-alias layer: **one `--ov-up-*`
token per libadwaita custom property the sheet reads**, each carrying the
fallback that keeps the sheet painting when libadwaita is absent — GTK's own
built-in named colours (`@theme_bg_color`, `@theme_base_color`,
`@theme_selected_bg_color`, `@accent_color`), which are the same palette
libadwaita's variables alias and track the scheme for free (Default-light /
Default-dark define the same names — dark reads `#353535` window / `#3e3e3e`
bar). `black` and `white` appear only as the physics constants for `--dark-5`
and `--light-1`; `--ov-up-border-opacity` keeps the `100%` fallback the sheet
already used, and `--ov-up-slider-border` keeps `currentColor`. Surfaces and
primitives now reference the aliases and nothing else, so the sheet can no
longer compute a colour to nothing in *any* GTK4 app, and
`upstream/variables.txt` finally lists all ten variables it depends on
(`--headerbar-bg-color`, `--accent-color`, `--accent-bg-color`,
`--view-bg-color`, `--border-opacity` and `---slider-border-color` were
missing from the contract).

**Consequence, deliberate.** A foreign app now wears the overlay's material in
the stock palette rather than stock GTK (frame `#ffe4e3e2` vs stock
`#ffdddad6`); CSS cannot ask whether libadwaita is loaded, and painting our
material with sane colours is the only alternative to painting nothing.
Helium's own appearance setting (Classic) sidesteps GTK colours entirely for
anyone who wants the browser untouched.

**Verification.** `tools/probe-foreign`: 9 → 0 inputs painting nothing, light
and dark, plus the live-session A/B above. `tools/gallery-diff` on
`render-gallery` output, old sheet vs new, **light and dark: 15/15 families
pixel-identical, 0 changed** (the alias layer is a pure rename inside
libadwaita apps). `tools/check-selectors`: contract OK against installed
`1:1.9.4-1`. **Not verified:** a dark-scheme live Helium window (only the
probe covers dark), and the browser after the sheet was already loaded — GTK
reads user CSS at process start, so the running Helium instance needs a
restart to pick this up.

## Flat register — 25 Sep 2026

**Ask.** "Our current 'flat' variation button is ALL FLAT" (user): the flat rung
should read as ours without becoming a chip. Run as a `variant` pass — three
candidates on one axis, *what carries the register's presence at rest* —
judged in the gallery, so the decision came from pixels rather than taste.

**What the sheet did before.** Nothing at all to `.flat`: the material's guard
is `button:not(.flat)` and the reset's guard is `:not(.flat)` too, so a `.flat`
button was upstream's own look (`background: transparent; box-shadow: none`,
gtk.css L338) plus upstream's washes and rings — while the families upstream
paints flat *without* the class were hard-zeroed by the 23 Sep reset. Two
registers sharing one name.

**Candidates.** `A` contour (0.5px `currentColor` ring), `B` sheen (lit top →
shade foot, no edge), `C` bevel-lite (the house pair at reduced amplitude over
a whisper sheen). Peaks against the sheet before them, light/dark: **A 7/7,
B 9/16, C 12/16**, where the opaque rest bevel measures **+1/−10 (light)** and
**+17/−3 (dark)** on the same page.

**A rejected.** Scheme-symmetric and clean, but a closed contour has no light
direction — it breaks the one-light-source rule and reads as a drawn outline
(a bordered field), not as material.

**The light-scheme finding.** The physics constants are not scheme-symmetric:
white over the light scheme's near-white page moves it by **+1/255** (measured
on the opaque button's own top hairline), black over the dark bar by about
**−2/255**. In light only a candidate's *shade* half can act; in dark only its
*highlight* half. That is why B and C measured 9 and 12 in light against 16 in
dark, and why they read "almost invisible in light" (user). Both then gained
per-scheme values, calibrated to **equal measured presence: 17/255 light,
16/255 dark** — matching a fill-less rung to the weight an opaque button gets
from its 19/255 fill plus its 10/255 foot line.

**Verdict (user, 25 Sep 2026): C, with B kept as a valid variation.** C is the
house's own material one notch down in both schemes — light carries it on the
bottom rule (how the opaque buttons themselves read in light, B6), dark on the
top rule — so it adds no second lighting language. At equal peak the two
differ in *distribution*, not amplitude: across the button's lower third B
spreads 11.8/255 of shade against C's 4.1/255, because a rule with no edge has
to ride its whole presence on the gradient, and in light what that ink does is
imitate a shadow under the button (the register the house bans on buttons). B
survives as a *token* variation of the same rule — point
`--ov-flat-edge-{top,bottom}` at `transparent` and raise `--ov-flat-shade` —
rather than as dead code.

**Promoted.** L1 `ov-flat-register()`, called from the `.flat` variation block
at the bottom of `surfaces/_button.scss`; four L0 tokens
(`--ov-flat-edge-top`, `--ov-flat-edge-bottom`, `--ov-flat-hilite`,
`--ov-flat-shade`, the last two re-pointed under `prefers-color-scheme: dark`).
The three `.flat`-parent bridges (`menubutton.flat`, `splitbutton.flat`) moved
out of `$ov-flat-structural` into the variation: same register, and the reset's
8-class selector would otherwise outrank the variation's 7-class one.

**The HC mechanism, deliberately different from every other primitive.** This
one declares its material inside `(prefers-contrast: no-preference)` instead of
emitting a `prefers-contrast: more` revert, because upstream's `button.flat`
owns `box-shadow` in its own states: `none` at rest, a `currentColor` ring on
hover / active / checked, 50% of it under HC. A revert block could only restate
`none` — which erases those rings — so the register declines to participate in
HC at all and hands the node back to upstream in every state. Measured: the
ring does not render on the gallery's flat button even in stock HC + prelight
(`--border-opacity` reaches a `color-mix()` whose node has nothing to mix), so
this is belt and braces rather than a visible repair.

**What is NOT touched.** The implicit flat families keep the 23 Sep reset
(BACKLOG W1): `button.link`, window controls, spinbutton arrows, pathbar
crumbs, popover model buttons, tab thumbnails, notebook arrows, calendar and
column/tree headers, infobar close, bottom-sheet actions. The flat rung's
press / held / checked states keep upstream's washes and rings.

**Verification.** `promoted.py` cells, every one noise-filtered: rest ×
{light, dark} across all 15 families; HC rest, hover and HC hover ×
{light, dark} across the four families that carry `.flat` nodes. Result: the
register moves *only* `.flat` nodes — `buttons` peak 17 (light) / 16 (dark),
`headerbar` 16/13, `adw` 17/12 — and 12 of 15 families are pixel-identical to
the sheet before it; HC is 0 px in all four cells. The matched-node set from a
solid-red probe equals the noise-filtered diff, so the scope claim is
geometric, not statistical. `tools/check-selectors`: contract OK against
installed `1:1.9.4-1`. `tools/probe-motion build/gtk.css`: hover and press
INSTANT (state landed), row hover and focus-visible IN MOTION — unchanged from
before the rule. The register's transition list is byte-for-byte the set
upstream gives those nodes (`outline-*` + `background` + `box-shadow`, 200ms on
the same curve), so it silences no upstream motion.

**Method note, worth keeping.** `render-gallery` is not run-to-run
deterministic: two renders of *one* sheet differ by up to ~19k px of text
antialiasing in `buttons`, `lists`, `columns` and `notebook` (and by 0 in the
shapes-only families). Every number above is measured against that floor — a
per-cell noise mask subtracted from the A/B diff — because without it the noise
alone reads as a 15-20/255 "change" on a label glyph. Any future gallery
comparison should do the same, or report `changed_px` as meaningless below the
floor.

**Not verified.** The live eye over a populated bar in daily apps (the user's
sweep is the gate, as always); fractional scale (X5 card); a disabled `.flat`
button — the guard is `:not(:disabled)` and upstream's `filter: opacity(30%)`
dim is never declared by us, but the gallery carries no disabled `.flat`
button, so that one is structural rather than measured.
