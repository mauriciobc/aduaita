#!/usr/bin/env bash
# Shared helpers for the adwaita-overlay tools. Sourced, never executed.

# extract_sheet <libadwaita.so> — print the compiled stylesheet to stdout.
# Handles both resource layouts:
#   1.9+    a single /org/gnome/Adwaita/styles/gtk.css
#   <= 1.6  four files (base / defaults-light / defaults-dark / base-hc),
#           concatenated — cascade order is irrelevant for the presence
#           checks the guard runs.
extract_sheet() {
  local so="$1" p
  local paths
  paths="$(gresource list "$so" 2>/dev/null | grep '^/org/gnome/Adwaita/styles/.*\.css$' || true)"
  if [[ -z "$paths" ]]; then
    echo "error: no stylesheets found in $so" >&2
    return 1
  fi
  if grep -qx '/org/gnome/Adwaita/styles/gtk.css' <<<"$paths"; then
    gresource extract "$so" /org/gnome/Adwaita/styles/gtk.css
    return 0
  fi
  for p in base.css defaults-light.css defaults-dark.css base-hc.css; do
    grep -qx "/org/gnome/Adwaita/styles/$p" <<<"$paths" || continue
    gresource extract "$so" "/org/gnome/Adwaita/styles/$p"
  done
}

# extract_selectors <gtk.css> — print the sorted set of selector atoms.
# Atom-level: "button, entry" yields two entries. @media preludes are
# stripped so nested rules are counted; @define-color lines dropped.
# It is a heuristic, but the same heuristic everywhere, which is all a
# diff needs.
extract_selectors() {
  sed 's/@media[^{]*{//g' "$1" \
    | grep -v '@define-color' \
    | tr '{' '\n' | grep -v '}' \
    | tr ',' '\n' | sed 's/^ *//;s/ *$//' \
    | grep -v '^$' | sort -u
}

# extract_variables <gtk.css> — print the sorted set of custom properties
# that are *defined* in the sheet (not merely referenced via var()).
extract_variables() {
  grep -oE -- '--[a-z0-9][a-z0-9-]*:' "$1" | tr -d ':' | sort -u
}
