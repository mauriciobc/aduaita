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
