/* tools/render-widget.c — render one widget with one CSS file, offscreen.
 *
 *   render-widget <css-file> <out.tiff> <button|headerbar> <width> <height>
 *
 * Same mechanism as the overlay: a CssProvider on the default display at
 * GTK_STYLE_PROVIDER_PRIORITY_USER (800). Paints the widget via
 * GtkWidgetPaintable + GskCairoRenderer and saves a TIFF for pixel
 * analysis (PIL reads it).
 *
 * Note: briefly presents a window (~400 ms) — GtkWidgetPaintable only
 * renders mapped widgets. gtk_widget_paint() would avoid that, but it is
 * not exposed in the public headers.
 *
 * Build:
 *   gcc -O1 -o build/render-widget tools/render-widget.c \
 *       $(pkg-config --cflags --libs gtk4)
 *
 * Born in Evening 0 (docs/decisions.md); reused by the X5 test card.
 */
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
  if (argc != 6) {
    g_printerr("usage: %s css out widget w h\n", argv[0]);
    return 2;
  }
  gtk_init();

  GdkDisplay *display = gdk_display_get_default();
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(provider, argv[1]);
  gtk_style_context_add_provider_for_display(
      display, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref(provider);

  GtkWidget *widget;
  if (strcmp(argv[3], "headerbar") == 0)
    widget = gtk_header_bar_new();
  else
    widget = gtk_button_new_with_label("Test");

  GtkWidget *win = gtk_window_new();
  gtk_window_set_child(GTK_WINDOW(win), widget);

  int w = atoi(argv[4]), h = atoi(argv[5]);
  gtk_widget_measure(widget, GTK_ORIENTATION_HORIZONTAL, -1, NULL, NULL, NULL, NULL);
  gtk_widget_allocate(widget, w, h, -1, NULL);
  gtk_widget_realize(win);
  gtk_widget_realize(widget);
  gtk_window_present(GTK_WINDOW(win));
  gint64 end = g_get_monotonic_time() + 400000;  /* 400 ms of mapping */
  while (g_get_monotonic_time() < end)
    g_main_context_iteration(NULL, FALSE);

  GdkPaintable *paintable = gtk_widget_paintable_new(widget);
  GtkSnapshot *snapshot = gtk_snapshot_new();
  gdk_paintable_snapshot(paintable, snapshot, w, h);
  GskRenderNode *node = gtk_snapshot_to_node(snapshot);

  if (node == NULL) {
    g_printerr("empty render node\n");
    return 1;
  }

  GskRenderer *renderer = gsk_cairo_renderer_new();
  gsk_renderer_realize(renderer, NULL, NULL);
  GdkTexture *texture = gsk_renderer_render_texture(renderer, node, NULL);
  gdk_texture_save_to_tiff(texture, argv[2]);

  graphene_rect_t bounds;
  gsk_render_node_get_bounds(node, &bounds);
  g_print("rendered %s (%g x %g)\n", argv[2],
          bounds.size.width, bounds.size.height);
  return 0;
}
