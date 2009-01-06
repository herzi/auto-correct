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

struct _AcAutoCorrectionPrivate {
        GList* corrections;
};

#define P(i) (((AcAutoCorrection*)(i))->_private)

G_DEFINE_TYPE (AcAutoCorrection, ac_auto_correction, G_TYPE_OBJECT);

static void
ac_auto_correction_init (AcAutoCorrection* self)
{
        P (self) = G_TYPE_INSTANCE_GET_PRIVATE (self, AC_TYPE_AUTO_CORRECTION, AcAutoCorrectionPrivate);
}

static void
ac_auto_correction_class_init (AcAutoCorrectionClass* self_class)
{
        g_type_class_add_private (self_class, sizeof (AcAutoCorrectionPrivate));
}

AcAutoCorrection*
ac_auto_correction_new (void)
{
        return g_object_new (AC_TYPE_AUTO_CORRECTION,
                             NULL);
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

void
ac_auto_correction_reverse (AcAutoCorrection* self)
{
        g_return_if_fail (AC_IS_AUTO_CORRECTION (self));

        P (self)->corrections = g_list_reverse (P (self)->corrections);
}
