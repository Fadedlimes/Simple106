# S-116 Porting Notes

Practical lessons from porting a dense synth editor (Metaphysical Factory) to the S-116 kit. These are engineering notes, not new API; the design system remains authoritative.

## 1. The knob halo is a tick band, not decoration

`kTickMargin = 11` is not a soft shadow allowance. The kit draws the 11 ticks between radius `r + 4.5` and `r + 11`; at a 64 px component the outer tick sits on the component edge. Do not overlap halos or rely on body-only hit testing to make overlapping controls look acceptable.

Minimum legible rotary is the Small body (42 px), which needs a **64 x 64** square. Below that the body becomes `min(w,h) - 2*halo`; 24 px gives a 2 px body and the draw call returns early at `body < 8`.

## 2. Derive the minimum window from footprints, not from the old layout

Compute each panel's minimum from its contents plus compact padding, then solve for the window:

- Bank A/B: 28 padding + 64 trim row + 202 fader cell = 294.
- Filters: 28 + 80 knob row + 202 fader cell = 310.
- Telemetry: 2 x 128 screens + 6 gap + 28 padding = 290.
- Body = 294 + 10 + 310 = 614; with header 84 and margins 12/16 the raw height is 726. The 48.5% row split needs 729; add 7 px slack -> **736**.
- Width at 1280: 426 + 10 + 342 + 10 + 468 = 1256 content; plus 24 margins = 1280. Do not add horizontal padding on top of that.

This is faster and safer than trying to shrink knobs below the kit minimum.

## 3. Resize-grip margin

JUCE's `ResizableCornerComponent` measured **18 x 18** in this build. A 16 px bottom margin keeps every control clear (the last button ends at `H-20`), though the grip still overlaps the panel background by 2 px. Use an 18 px bottom margin if zero panel-background overlap is required. The right margin can stay 12 px because the last control is horizontally clear.

## 4. Measure fader-cell widths from the real labels

Use the actual Barlow Condensed Bold 13 px + kit kerning, not guesses:

- `DUAL NOISE` = 63.8 px -> 68 px Bank cell.
- `RESO TUNE` = 59.8 px -> 64 px (or wider) Resochord cell.
- `ABSORB` = 42.1 px -> 54 px Effects cell.
- `LP NOISE` = 49.2 px -> fits a 68 px Bank cell.

At 1280, Banks use 68 px cells / 2 px gaps; Filters 54 / 2; Effects 55 / 2 (8 x 55 + 7 x 2 = 454 inside 460); Sampler/Reso 75 / 2. Centre a 64 px trim in the fader cell (`placeBareKnob` with the fader cell width) so trim and fader centre lines match.

## 5. Opt-in properties

- `slider.getProperties().set("s116Fader", true)` enables the vertical kit fader (slot, matte cap, ticks, LED lane). Leave it off for existing linear sliders.
- `button.getProperties().set("s116Compact", true)` enables the 26 px face / 34 px component compact ledge button, minimum 48 px wide.
- v1.5.1 opt-in label type properties: `s116NoShadow`, `s116NoKerning`, `s116Size`, `s116AutoContrast`, `s116OnDark`, `s116IntegerText`. The `--type` preview prints contrast ratios and checks ENV MOD (14 px, no kerning) in 64 px.
- `button.getProperties().set("s116Arrow", true)` enables the 26 px face / 34 px component compact arrow: use a 32 x 34 component (minimum width 32), no LED strip. Compact text buttons SAVE / INIT / CLR / REST are 48 px minimum wide; their 12 px Barlow labels measure 18.7-25.4 px.
- `placeBareKnob(slider, cellX, cellY, cellW, body)` positions a no-text-box knob centred in the cell; pass the fader-cell width to keep rows aligned.

These properties are off by default, which is how Simple116 kept its old pitch/mod wheels and buttons unchanged.

## 6. Metaphysical Black is a fixed theme

`ThemeId::MetaphysicalBlack` is intentionally **outside** `ThemeId::numThemes`, so the four legacy themes keep their existing cycle. Select it explicitly with `skin().setTheme(ThemeId::MetaphysicalBlack)` and set the LED base to `0xffff2a85` if the host wants the full pink identity. The theme's logo accent/pointer are pink; caption text is light enough for 4.5:1 on the panel.

## 7. C1 / C2 / C3 order

1. **C1:** typefaces, fixed theme/LED, `SilverLookAndFeel`, one `layout()`, compact panels, all editor-owned rotaries, captions/accents, header and custom nameplate. Keep the visualizers and motion faders on their existing drawing; list the temporary V4 fallbacks.
2. **C2:** MotionSlider only. Set `s116Fader`, kit caption labels, stacked compact REC/1x buttons, theme wireframe/playhead, recording text marker. Keep attachments, lanes, drag handlers and the 60 Hz timer untouched.
3. **C3:** telemetry only. Remove the local OLED LAF, use 64 px scope dials in 72 px wings, cache the static skin with `SkinnedComponent`, repaint only trace/meters/playhead, and restyle the waveform and SMP badge states.

Doing the steps in this order keeps each screenshot review small and makes fallbacks obvious instead of hiding them in one large restyle.

## Simple106 lessons (v1.5.2, docs only)

These come from porting Simple106 (fixed 920x640 editor -> kit panels). They are
additive guidance; the v1.5.1 code and all existing vendored copies are unchanged.

### Fixed-size knob layouts and window derivation
- Do not scale the kit art with a canvas transform when the editor must be
  resizable at native pixels. Derive the window from real footprints instead.
- Simple106 used Small knobs: 42 px body + 11 px halo = 64x64; a bare cell is
  80 px (64 + 16 caption). Combos are 32 px high (52 px with a field label);
  compact buttons are 34 px; arrows are 32x34; compact panels use 24/4/4.
- Panel minima were derived per group (e.g. DCO 360x196, voice matrix 240x310,
  footer 1008x182) then summed with 10 px gaps and 36 px side margins. The hard
  content height was 822 px; with 12 px slack the minimum became 1080x840. The
  maximum 1280x960 was chosen because fixed 64 px knobs turn extra width/height
  into knob spacing and empty panel area above that. Default = minimum.
- Keep a single `layout()` as the source of truth and audit at minimum, default
  and maximum: child-in-parent, sibling overlap (halo included), label fit,
  knob-centre alignment and panel-title width.

### Multi-instance theme rule
- The kit skin is process-global. Each editor applies its own saved chassis/LED
  at construction and when its window gains focus.
- A timer syncs this editor's combo boxes from raw `guiTheme`/`ledTheme` values,
  but applies them to the skin only when this editor is focused or is the only
  open editor. With two visible editors both show the last-focused editor's
  theme; document that limit.
- `SkinnedComponent` caches rebuild on skin change messages, not on timer ticks.
  Never call `invalidateSkin()` from a 60 Hz timer.

### Gesture-driven header LCD
- Use a `LcdText` (Simple106: 160x46) and update it only from user gestures:
  hover, drag, wheel or mouse-down via the editor's global mouse listener.
  Do not use `Slider::Listener`/`onValueChange`, which also fires on host
  automation and preset loads.
- Hold the edit for 1.5 s (90 ticks at 60 Hz), then return to a static idle
  (`S-106` / `READY`). The idle text must not follow the preset name, or preset
  loads would change it. Keep the six voice readouts separate and persistent.

### Text legibility (variant E)
- Variant E: no shadow, no extra kerning, integer text bounds, 14 px captions
  and V1-V6, auto ink (near-black on light surfaces, white on dark). Field
  labels are 14 px only inside a 16 px-high label box; otherwise use 13 px.
- Apply `s116OnDark` to labels on the dark header (PATCH, CHASSIS, LED COLOUR)
  even in Classic Silver; the default 12 px field style with a black -1 px
  shadow on the dark header measured 1.2:1. The 10 px Simple116 header labels
  have the same latent risk, but are currently not laid out.
- The old 13 px caption + 0.085 kerning + black shadow read V5 as V6 and looked
  soft on Classic Silver's light panels. Variant E at 14 px/no kerning makes
  V5/V6 distinct and removes the light-panel shadow halo.
- Keyboard text: minimum 11 px (OCTAVE 11, OCT -/+ 12, white keys 12, black keys
  11), integer text rects, and ink chosen for the actual surface. The octave
  plate is LED-dark; the black keys are black in every chassis theme. Never use
  `t.heading`/`t.caption` blindly on those surfaces.
- Audit with `GlyphArrangement` against the label rectangle, and contrast every
  label and keyboard group against its real background for all four chassis
  themes (and all seven LED bases for LED surfaces). Require >= 4.5:1.

### Voice-readout pattern
- Six voice knobs use `placeKnob` with a 54x20 readout and V1-V6 captions; keep
  the readout text function **after** the `SliderAttachment`. JUCE's
  `SliderAttachment` installs the parameter's own text function, so setting the
  formatter first silently gives numeric readouts. Pan: `MID` / `L<0-100>` /
  `R<0-100>`; tune: `<n> ct`. Rebind `vPan1-6` / `vTune1-6` on `voiceVarMode`.
  Draw the six live LEDs in the voice panel's `paintDynamic`, driven by
  `isVoiceActive(i)`, not in editor paint over child panels.

### Focus audit
- Every control and kit component must have both
  `setWantsKeyboardFocus(false)` and `setMouseClickGrabsKeyboardFocus(false)`.
  The only allowed focus owners are the editor root (QWERTY), the save dialog's
  TextEditor, and the resize grip.
- JUCE creates/positions internal ComboBox labels and slider text boxes during
  `resized()`. Clear the focus flags after every layout, not only in the
  constructor, or internal labels grab focus again.
- Keep the 60 Hz re-grab and `focusLost` note-offs. Audit by walking the whole
  hierarchy, not just the editor's direct controls.

### Debug audit behind a build flag
- Keep the layout/text/behaviour audit out of shipping editor code. Simple106
  uses `S106_ENABLE_DEBUG_AUDIT` (OFF by default) and compiles the audit under
  `S106_DEBUG_AUDIT`; when enabled it runs at 1080x840, 1200x900 and 1280x960.
  Do not read an environment variable in release builds.
