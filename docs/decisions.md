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
