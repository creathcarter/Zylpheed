/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2014 Hiroyuki Yamamoto
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include <string.h>

#include "prefs_common.h"
#include "manual.h"
#include "alertpanel.h"
#include "utils.h"

/* The local manual and FAQ describe Sylpheed 3.x; open the Sylpheed web
   site instead. */
#define SYLPHEED_MANUAL_URI	"https://sylpheed.sraoss.jp/"
#define SYLPHEED_FAQ_URI	"https://sylpheed.sraoss.jp/"

static void manual_notice(void);

static void manual_notice(void)
{
	static gboolean shown = FALSE;

	if (shown) return;
	shown = TRUE;

	/* not translated */
	alertpanel_notice("%s", "The Sylpheed manual and FAQ describe "
			  "Sylpheed 3.x. Zylpheed differs in places; see the "
			  "README at https://github.com/creathcarter/zylpheed.");
}

void manual_open(ManualLang lang)
{
	manual_notice();
	debug_print("Opening manual: %s\n", SYLPHEED_MANUAL_URI);
	open_uri(SYLPHEED_MANUAL_URI, prefs_common.uri_cmd);
}

void faq_open(ManualLang lang)
{
	manual_notice();
	debug_print("Opening FAQ: %s\n", SYLPHEED_FAQ_URI);
	open_uri(SYLPHEED_FAQ_URI, prefs_common.uri_cmd);
}
