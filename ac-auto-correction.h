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

#ifndef AC_AUTO_CORRECTION_H
#define AC_AUTO_CORRECTION_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _AcAutoCorrection        AcAutoCorrection;
typedef struct _AcAutoCorrectionPrivate AcAutoCorrectionPrivate;
typedef struct _AcAutoCorrectionClass   AcAutoCorrectionClass;

typedef struct _AutoCompletion AutoCompletion;

#define AC_TYPE_AUTO_CORRECTION         (ac_auto_correction_get_type ())
#define AC_AUTO_CORRECTION(i)           (G_TYPE_CHECK_INSTANCE_CAST ((i), AC_TYPE_AUTO_CORRECTION, AcAutoCorrection))
#define AC_AUTO_CORRECTION_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), AC_TYPE_AUTO_CORRCTION, AcAutoCorrectionClass))
#define AC_IS_AUTO_CORRECTION(i)        (G_TYPE_CHECK_INSTANCE_TYPE ((i), AC_TYPE_AUTO_CORRECTION))
#define AC_IS_AUTO_CORRECTION_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), AC_TYPE_AUTO_CORRECTION))
#define AC_AUTO_CORRECTION_GET_CLASS(i) (G_TYPE_INSTANCE_GET_CLASS ((i), AC_TYPE_AUTO_CORRECTION, AcAutoCorrection))

GType             ac_auto_correction_get_type        (void);
AcAutoCorrection* ac_auto_correction_new             (void);
AcAutoCorrection* ac_auto_correction_new_from_path   (gchar const     * path,
                                                      GError          **error);
GList*            ac_auto_correction_get_corrections (AcAutoCorrection* self);
void              ac_auto_correction_prepend         (AcAutoCorrection* self,
                                                      AutoCompletion  * cmp);
void              ac_auto_correction_remove          (AcAutoCorrection* self,
                                                      AutoCompletion  * cmp);

void              auto_completion_free               (AutoCompletion  * self);

struct _AcAutoCorrection {
        GObject                  base_instance;
        AcAutoCorrectionPrivate* _private;
};

struct _AcAutoCorrectionClass {
        GObjectClass             base_class;
};

typedef enum {
        AUTO_COMPLETION_NONE = 0,
        AUTO_COMPLETION_AFTER_WHITESPACE = 1
} AutoCompletionFlags;

struct _AutoCompletion {
        gchar              * before;
        gchar              * after;
        AutoCompletionFlags  flags;
};

G_END_DECLS

#endif /* !AC_AUTO_CORRECTION_H */
