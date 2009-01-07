/* This file is part of auto-correct
 *
 * AUTHORS
 *     Sven Herzberg  <sven@imendio.com>
 *
 * Copyright (C) 2009  Sven Herzberg
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <glib.h>

#include "ac-auto-correction.h"

static void
ac_new (void)
{
        AcAutoCorrection* ac = ac_auto_correction_new ();

        g_assert (AC_IS_AUTO_CORRECTION (ac));
        g_assert (!ac_auto_correction_get_corrections (ac));

        g_object_unref (ac);
}

static void
ac_new_from_path (void)
{
        /* FIXME: test error conditions, too */
        AcAutoCorrection* ac = ac_auto_correction_new_from_path ("auto-correct-de.xml", NULL);

        g_assert (AC_IS_AUTO_CORRECTION (ac));
        g_assert (g_list_length (ac_auto_correction_get_corrections (ac)) == 2);

        /* FIXME: query children */

        g_object_unref (ac);
}

static void
ac_prepend (void)
{
        AcAutoCorrection* ac = ac_auto_correction_new ();
        AutoCompletion* cmp;

        g_assert (AC_IS_AUTO_CORRECTION (ac));
        g_assert (!ac_auto_correction_get_corrections (ac));

        cmp = g_slice_new0 (AutoCompletion);
        cmp->before = "sliff";
        cmp->after  = "sloff";

        ac_auto_correction_prepend (ac, cmp);

        g_assert (g_list_length (ac_auto_correction_get_corrections (ac)) == 1);
        g_assert (ac_auto_correction_get_corrections (ac)->data == cmp);

        cmp = g_slice_new0 (AutoCompletion);
        cmp->before = "foo";
        cmp->after  = "bar";

        ac_auto_correction_prepend (ac, cmp);

        g_assert (g_list_length (ac_auto_correction_get_corrections (ac)) == 2);
        g_assert (ac_auto_correction_get_corrections (ac)->data == cmp);

        g_object_unref (ac);
}

static void
ac_remove (void)
{
        /* FIXME: check error condition */
        AcAutoCorrection* ac = ac_auto_correction_new_from_path ("auto-correct-de.xml", NULL);
        AutoCompletion* cmp;

        g_assert (AC_IS_AUTO_CORRECTION (ac));
        g_assert (g_list_length (ac_auto_correction_get_corrections (ac)) == 2);

        cmp = ac_auto_correction_get_corrections (ac)->next->data;
        ac_auto_correction_remove (ac, cmp);

        g_assert (g_list_length (ac_auto_correction_get_corrections (ac)) == 1);
        g_assert (ac_auto_correction_get_corrections (ac)->data != cmp);

        ac_auto_correction_remove (ac, ac_auto_correction_get_corrections (ac)->data);

        g_assert (!ac_auto_correction_get_corrections (ac));

        g_object_unref (ac);
}

int
main (int   argc,
      char**argv)
{
        g_test_init (&argc, &argv, NULL);

        g_type_init ();

        g_test_add_func ("/auto-correction/new", ac_new);
        g_test_add_func ("/auto-correction/new-from-path", ac_new_from_path);

        g_test_add_func ("/auto-correction/list/prepend", ac_prepend);
#if 0
        g_test_add_func ("/auto-correction/list/append", ac_append);
#endif
        g_test_add_func ("/auto-correction/list/remove", ac_remove);

#if 0
        g_test_add_func ("/auto-correction/merge/disjunct", ac_merge_disjunct);
        g_test_add_func ("/auto-correction/merge/equal", ac_merge_equal);
        g_test_add_func ("/auto-correction/merge/subset", ac_merge_subset);
        g_test_add_func ("/auto-correction/merge/superset", ac_merge_superset);
        g_test_add_func ("/auto-correction/merge/intersected", ac_merge_intersected);

        g_test_add_func ("/auto-correction/merge/append-real");
        g_test_add_func ("/auto-correction/merge/prepend-real");
        g_test_add_func ("/auto-correction/merge/remove-real");

        g_test_add_func ("/auto-correction/merge/append-proxied");
        g_test_add_func ("/auto-correction/merge/prepend-proxied");
        g_test_add_func ("/auto-correction/merge/remove-proxied");
#endif
        return g_test_run ();
}

