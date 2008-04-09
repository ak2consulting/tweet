#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib.h>

#include <json-glib/json-glib.h>

#include "twitter-common.h"
#include "twitter-enum-types.h"
#include "twitter-private.h"
#include "twitter-status.h"
#include "twitter-timeline.h"
#include "twitter-user.h"

#define TWITTER_TIMELINE_GET_PRIVATE(obj)       (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TWITTER_TYPE_TIMELINE, TwitterTimelinePrivate))

struct _TwitterTimelinePrivate
{
  GHashTable *status_by_id;
  GList *status_list;
};

G_DEFINE_TYPE (TwitterTimeline, twitter_timeline, G_TYPE_OBJECT);

static void
twitter_timeline_finalize (GObject *gobject)
{
  TwitterTimelinePrivate *priv = TWITTER_TIMELINE (gobject)->priv;

  g_hash_table_destroy (priv->status_by_id);
  g_list_free (priv->status_list);

  G_OBJECT_CLASS (twitter_timeline_parent_class)->finalize (gobject);
}

static void
twitter_timeline_class_init (TwitterTimelineClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (TwitterTimelinePrivate));

  gobject_class->finalize = twitter_timeline_finalize;
}

static void
twitter_timeline_init (TwitterTimeline *timeline)
{
  TwitterTimelinePrivate *priv;

  timeline->priv = priv = TWITTER_TIMELINE_GET_PRIVATE (timeline);

  priv->status_by_id = g_hash_table_new_full (NULL, NULL,
                                              NULL,
                                              g_object_unref);
}

static void
twitter_timeline_clean (TwitterTimeline *timeline)
{
  TwitterTimelinePrivate *priv = timeline->priv;

  if (priv->status_list)
    {
      g_list_free (priv->status_list);
      priv->status_list = NULL;
    }

  if (priv->status_by_id)
    {
      g_hash_table_destroy (priv->status_by_id);
      priv->status_by_id = g_hash_table_new_full (NULL, NULL,
                                                  NULL,
                                                  g_object_unref);
    }
}

static void
twitter_timeline_build (TwitterTimeline *timeline,
                        JsonNode        *node)
{
  TwitterTimelinePrivate *priv = timeline->priv;
  JsonArray *array;
  GList *elements, *l;
  GList *status_list = NULL;

  if (!node || JSON_NODE_TYPE (node) != JSON_NODE_ARRAY)
    return;

  array = json_node_get_array (node);
  elements = json_array_get_elements (array);

  for (l = elements; l != NULL; l = l->next)
    {
      JsonNode *element = l->data;

      if (JSON_NODE_TYPE (element) == JSON_NODE_OBJECT)
        {
          TwitterStatus *status;
          guint status_id;

          status = twitter_status_new_from_node (element);
          status_id = twitter_status_get_id (status);
          if (status_id == 0)
            {
              g_object_unref (status);
              continue;
            }

          g_hash_table_replace (priv->status_by_id,
                                GUINT_TO_POINTER (status_id),
                                g_object_ref_sink (status));
          status_list = g_list_prepend (status_list, status);
        }
    }

  priv->status_list = g_list_reverse (status_list);
}

TwitterTimeline *
twitter_timeline_new (void)
{
  return g_object_new (TWITTER_TYPE_TIMELINE, NULL);
}

TwitterTimeline *
twitter_timeline_new_from_data (const gchar *buffer)
{
  TwitterTimeline *retval;
  JsonParser *parser;
  GError *parse_error;

  g_return_val_if_fail (buffer != NULL, NULL);

  retval = twitter_timeline_new ();

  parser = json_parser_new ();
  parse_error = NULL;
  json_parser_load_from_data (parser, buffer, -1, &parse_error);
  if (parse_error)
    {
      g_warning ("Unable to parse data into a timeline: %s",
                 parse_error->message);
      g_error_free (parse_error);
    }
  else
    twitter_timeline_build (retval, json_parser_get_root (parser));

  g_object_unref (parser);

  return retval;
}

void
twitter_timeline_load_from_data (TwitterTimeline *timeline,
                                 const gchar     *buffer)
{
  JsonParser *parser;
  GError *parse_error;

  g_return_if_fail (TWITTER_IS_TIMELINE (timeline));
  g_return_if_fail (buffer != NULL);

  twitter_timeline_clean (timeline);

  parser = json_parser_new ();
  parse_error = NULL;
  json_parser_load_from_data (parser, buffer, -1, &parse_error);
  if (parse_error)
    {
      g_warning ("Unable to parse data into a timeline: %s",
                 parse_error->message);
      g_error_free (parse_error);
    }
  else
    twitter_timeline_build (timeline, json_parser_get_root (parser));

  g_object_unref (parser);
}

guint
twitter_timeline_get_count (TwitterTimeline *timeline)
{
  g_return_val_if_fail (TWITTER_IS_TIMELINE (timeline), 0);

  return g_hash_table_size (timeline->priv->status_by_id);
}

TwitterStatus *
twitter_timeline_get_status (TwitterTimeline *timeline,
                             guint            id)
{
  g_return_val_if_fail (TWITTER_IS_TIMELINE (timeline), NULL);
  g_return_val_if_fail (index_ < twitter_timeline_get_count (timeline), NULL);

  return g_hash_table_lookup (timeline->priv->status_by_id,
                              GUINT_TO_POINTER (id));
}

GList *
twitter_timeline_get_all (TwitterTimeline *timeline)
{
  g_return_val_if_fail (TWITTER_IS_TIMELINE (timeline), NULL);

  return g_list_copy (timeline->priv->status_list);
}
