#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include <cairo/cairo.h>

#include <clutter/clutter.h>
#include <clutter-cairo/clutter-cairo.h>
#include <tidy/tidy.h>

#include <twitter-glib/twitter-glib.h>

#include "tweet-status-renderer.h"

#define ICON_WIDTH      96
#define ICON_HEIGHT     96

#define V_PADDING       6
#define H_PADDING       12

#define DEFAULT_WIDTH   (ICON_WIDTH + (2 * H_PADDING) + 230)
#define DEFAULT_HEIGHT  (ICON_HEIGHT + (2 * V_PADDING))

#define TEXT_X          (ICON_WIDTH + (2 * H_PADDING))
#define TEXT_Y          (V_PADDING)

#define BG_ROUND_RADIUS 12

G_DEFINE_TYPE (TweetStatusRenderer,
               tweet_status_renderer,
               TIDY_TYPE_CELL_RENDERER);

static ClutterActor *
create_cell (TwitterStatus *status,
             gint           width,
             gint           height,
             const gchar   *font_name)
{
  ClutterActor *base, *bg, *label;
  cairo_t *cr;
  ClutterColor bg_color = { 162, 162, 162, 0xcc };
  ClutterColor label_color = { 0xee, 0xee, 0xee, 255 };

  if (width < 0)
    width = DEFAULT_WIDTH;
  if (height < 0)
    height = DEFAULT_HEIGHT;

  base = clutter_group_new ();

  /* background texture */
  bg = clutter_cairo_new (DEFAULT_WIDTH, DEFAULT_HEIGHT);
  clutter_container_add_actor (CLUTTER_CONTAINER (base), bg);
  clutter_actor_show (bg);

  cr = clutter_cairo_create (CLUTTER_CAIRO (bg));
  g_assert (cr != NULL);

  cairo_move_to (cr, BG_ROUND_RADIUS, 0);
  cairo_line_to (cr, width - BG_ROUND_RADIUS, 0);
  cairo_curve_to (cr, width, 0, width, 0, width, BG_ROUND_RADIUS);
  cairo_line_to (cr, width, height - BG_ROUND_RADIUS);
  cairo_curve_to (cr, width, height, width, height, width - BG_ROUND_RADIUS, height);
  cairo_line_to (cr, BG_ROUND_RADIUS, height);
  cairo_curve_to (cr, 0, height, 0, height, 0, height - BG_ROUND_RADIUS);
  cairo_line_to (cr, 0, BG_ROUND_RADIUS);
  cairo_curve_to (cr, 0, 0, 0, 0, BG_ROUND_RADIUS, 0);

  cairo_close_path (cr);

  clutter_cairo_set_source_color (cr, &bg_color);
  cairo_fill_preserve (cr);

  cairo_destroy (cr);

  g_assert (TWITTER_IS_STATUS (status));

  /* status text */
  label = clutter_label_new ();
  clutter_label_set_color (CLUTTER_LABEL (label), &label_color);
  clutter_label_set_font_name (CLUTTER_LABEL (label), font_name);
  clutter_label_set_line_wrap (CLUTTER_LABEL (label), TRUE);
  clutter_label_set_text (CLUTTER_LABEL (label), twitter_status_get_text (status));
  clutter_container_add_actor (CLUTTER_CONTAINER (base), label);
  clutter_actor_set_position (label, TEXT_X, TEXT_Y);
  clutter_actor_set_width (label, 230);
  clutter_actor_show (label);

  return base;
}

static ClutterActor *
tweet_status_renderer_get_cell_actor (TidyCellRenderer *renderer,
                                      TidyActor        *list_view,
                                      const GValue     *value,
                                      TidyCellState     cell_state,
                                      ClutterGeometry  *size,
                                      gint              row,
                                      gint              column)
{
  ClutterActor *retval = NULL;
  gchar *font_name;
  ClutterColor *background_color, *active_color, *text_color;
  ClutterFixed x_align, y_align;

  tidy_stylable_get (TIDY_STYLABLE (list_view),
                     "font-name", &font_name,
                     "bg-color", &background_color,
                     "active-color", &active_color,
                     "text-color", &text_color,
                     NULL);

  tidy_cell_renderer_get_alignmentx (renderer, &x_align, &y_align);

  if (row == -1 || cell_state == TIDY_CELL_HEADER)
    {
      ClutterActor *label;

      retval = tidy_frame_new ();
      tidy_actor_set_alignmentx (TIDY_ACTOR (retval), x_align, y_align);

      label = g_object_new (CLUTTER_TYPE_LABEL,
                            "font-name", font_name,
                            "text", g_value_get_string (value),
                            "color", text_color,
                            "alignment", PANGO_ALIGN_CENTER,
                            "ellipsize", PANGO_ELLIPSIZE_END,
                            "wrap", FALSE,
                            NULL);
      clutter_container_add_actor (CLUTTER_CONTAINER (retval), label);
      clutter_actor_show (label);

      goto out;
    }

  if (G_VALUE_TYPE (value) != TWITTER_TYPE_STATUS)
    return NULL;
  else
    {
      TwitterStatus *status;
      gint width, height;

      status = g_value_get_object (value);

      width = height = -1;
      if (size)
        {
          width = size->width;
          height = size->height;
        }

      retval = create_cell (status, width, height, font_name);
    }

out:
  g_free (font_name);
  clutter_color_free (background_color);
  clutter_color_free (active_color);
  clutter_color_free (text_color);

  return retval;
}

static void
tweet_status_renderer_class_init (TweetStatusRendererClass *klass)
{
  TidyCellRendererClass *renderer_class = TIDY_CELL_RENDERER_CLASS (klass);

  renderer_class->get_cell_actor = tweet_status_renderer_get_cell_actor;
}

static void
tweet_status_renderer_init (TweetStatusRenderer *renderer)
{

}

TidyCellRenderer *
tweet_status_renderer_new (void)
{
  return g_object_new (TWEET_TYPE_STATUS_RENDERER, NULL);
}