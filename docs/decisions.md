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
