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

int
main (int   argc,
      char**argv)
{
        g_test_init (&argc, &argv, NULL);
#if 0
        g_test_add_func ("/auto-correction/new", ac_new);
        g_test_add_func ("/auto-correction/new-from-path", ac_new_from_path);

        g_test_add_func ("/auto-correction/list/prepend", ac_prepend);
        g_test_add_func ("/auto-correction/list/append", ac_append);
        g_test_add_func ("/auto-correction/list/remove", ac_remove);

        g_test_add_func ("/auto-correction/merge", ac_merge);
#endif
        return g_test_run ();
}

