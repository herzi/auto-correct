/* This file is part of auto-correct
 *
 * AUTHORS
 *     Sven Herzberg  <sven@imendio.com>
 *
 * Copyright (C) 2009  Sven Herzberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "ac-auto-correction.h"

#include <string.h>
#include <libxml/parser.h>

struct _AcAutoCorrectionPrivate {
        GList* corrections;
};

#define P(i) (((AcAutoCorrection*)(i))->_private)

void
auto_completion_free (AutoCompletion* c)
{
        g_free (c->before);
        g_free (c->after);
        g_slice_free (AutoCompletion, c);
}

G_DEFINE_TYPE (AcAutoCorrection, ac_auto_correction, G_TYPE_OBJECT);

static void
ac_auto_correction_init (AcAutoCorrection* self)
{
        P (self) = G_TYPE_INSTANCE_GET_PRIVATE (self, AC_TYPE_AUTO_CORRECTION, AcAutoCorrectionPrivate);
}

static void
correction_finalize (GObject* object)
{
        g_list_foreach (P (object)->corrections, (GFunc)auto_completion_free, NULL);
        g_list_free (P (object)->corrections);

        G_OBJECT_CLASS (ac_auto_correction_parent_class)->finalize (object);
}

static void
ac_auto_correction_class_init (AcAutoCorrectionClass* self_class)
{
        GObjectClass* object_class = G_OBJECT_CLASS (self_class);

        object_class->finalize = correction_finalize;

        g_type_class_add_private (self_class, sizeof (AcAutoCorrectionPrivate));
}

AcAutoCorrection*
ac_auto_correction_new (void)
{
        return g_object_new (AC_TYPE_AUTO_CORRECTION,
                             NULL);
}

static void
start_element_ns (gpointer      ctxt,
                  guchar const* local_name,
                  guchar const* prefix,
                  guchar const* uri,
                  int           n_namespaces,
                  guchar const**namespaces,
                  int           n_attributes,
                  int           n_defaulted,
                  guchar const**attributes)
{
        AcAutoCorrection* ac;

        g_return_if_fail (g_strcmp0 ("http://www.adeal.eu/auto-correct/0.0.1", (gchar const*)uri) == 0);

        ac = AC_AUTO_CORRECTION (((xmlParserCtxt*)(ctxt))->_private);

        if (!strcmp ("entry", (gchar const*)local_name)) {
                AutoCompletion* cmp = g_slice_new0 (AutoCompletion);
                gint i;

                for (i = 0; i < n_attributes; i++) {
                        if (g_strcmp0 ("before", (gchar const*)attributes[5*i]) == 0) {
                                g_return_if_fail (!cmp->before);
                                cmp->before = g_strndup ((gchar*)attributes[5*i+3],
                                                         attributes[5*i+4] - attributes[5*i+3]);
                        } else if (g_strcmp0 ("after", (gchar const*)attributes[5*i]) == 0) {
                                g_return_if_fail (!cmp->after);
                                cmp->after = g_strndup ((gchar*)attributes[5*i+3],
                                                        attributes[5*i+4] - attributes[5*i+3]);
                        } else if (g_strcmp0 ("flags", (gchar const*)attributes[5*i]) == 0) {
                                gchar* value = g_strndup ((gchar*)attributes[5*i+3],
                                                          attributes[5*i+4] - attributes[5*i+3]);
                                gchar**values = g_strsplit (value, " ", 0);
                                gchar**iter;
                                for (iter = values; *iter; iter++) {
                                        if (g_strcmp0 ("after-whitespace", *iter) == 0) {
                                                cmp->flags |= AUTO_COMPLETION_AFTER_WHITESPACE;
                                        } else {
                                                g_assert_not_reached ();
                                        }
                                }
                                g_strfreev (values);
                                g_free (value);
                        } else {
                                g_assert_not_reached ();
                        }
                }

                g_return_if_fail (cmp->before != NULL);
                g_return_if_fail (cmp->after  != NULL);

                /* FIXME: use a GQueue and append properly */
                ac_auto_correction_prepend (ac, cmp);
        } else if (!strcmp ("auto-correction", (gchar const*)local_name)) {
#if 0
                for (i = 0; i < n_namespaces; i++) {
                        g_print ("\t%s:%s\n", namespaces[2*i], namespaces[2*i+1]);
                }
#endif
        } else {
                g_assert_not_reached ();
        }
}

static void
end_element_ns (gpointer      ctxt,
                  guchar const* local_name,
                  guchar const* prefix,
                  guchar const* uri)
{
        g_return_if_fail (g_strcmp0 ("http://www.adeal.eu/auto-correct/0.0.1", (gchar const*)uri) == 0);

        if (!g_strcmp0 ("entry", (gchar const*)local_name)) {
        } else if (!g_strcmp0 ("auto-correction", (gchar const*)local_name)) {
                AcAutoCorrection* self = AC_AUTO_CORRECTION (((xmlParserCtxt*)(ctxt))->_private);
                P (self)->corrections = g_list_reverse (P (self)->corrections);
        }
}

AcAutoCorrection*
ac_auto_correction_new_from_path (gchar const* path,
                                  GError     **error)
{
        AcAutoCorrection* self;
        xmlSAXHandler     sax;

        g_return_val_if_fail (path != NULL, NULL);
        g_return_val_if_fail (!error || !*error, NULL);

        self = ac_auto_correction_new ();

        xmlSAXVersion (&sax, 2);
        sax.startDocument  = NULL;
        sax.startElementNs = start_element_ns;
        sax.endElementNs   = end_element_ns;
        sax.comment        = NULL;
        xmlSAXParseFileWithData (&sax, path, 0, self);

        /* FIXME: watch and reload file */

        /* FIXME: use the error to return a message from the XML parser */

        return self;
}

GList*
ac_auto_correction_get_corrections (AcAutoCorrection* self)
{
        g_return_val_if_fail (AC_IS_AUTO_CORRECTION (self), NULL);

        return P (self)->corrections;
}

void
ac_auto_correction_prepend (AcAutoCorrection* self,
                            AutoCompletion  * cmp)
{
        g_return_if_fail (AC_IS_AUTO_CORRECTION (self));
        g_return_if_fail (cmp != NULL);

        P (self)->corrections = g_list_prepend (P (self)->corrections, cmp);
}

void
ac_auto_correction_remove (AcAutoCorrection* self,
                           AutoCompletion  * cmp)
{
        g_return_if_fail (AC_IS_AUTO_CORRECTION (self));
        g_return_if_fail (cmp != NULL);
        g_return_if_fail (g_list_find (P (self)->corrections, cmp));

        P (self)->corrections = g_list_remove (P (self)->corrections, cmp);
}

