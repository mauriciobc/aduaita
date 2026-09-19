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
