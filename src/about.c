/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2018 Hiroyuki Yamamoto
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
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#if HAVE_SYS_UTSNAME_H
#  include <sys/utsname.h>
#endif

#include "about.h"
#include "gtkutils.h"
#include "stock_pixmap.h"
#include "prefs_common.h"
#include "utils.h"
#include "version.h"

#ifdef G_OS_WIN32
#  include <windows.h>
#endif

/* the upstream version this fork is based on */
#define SYLPHEED_BASE_VERSION	"3.8.0beta1"
#define ZYLPHEED_HOMEPAGE_URI	"https://github.com/creathcarter/zylpheed"

/* The licence text box keeps at least this height (the height it had in
   Sylpheed's dialog), so that it is apparent that the text scrolls. */
#define ABOUT_TEXT_MIN_HEIGHT	96

static GtkWidget *window;

static void about_create(void);
static gboolean key_pressed(GtkWidget *widget, GdkEventKey *event);
static void about_uri_clicked(GtkButton *button, gpointer data);
static void about_fork_uri_clicked(GtkButton *button, gpointer data);

void about_show(void)
{
	if (!window)
		about_create();
	else
		gtk_window_present(GTK_WINDOW(window));
}

#ifdef G_OS_WIN32
/* "Windows 11 (64-bit)". GetVersionEx() reports Windows 8 to a program
   without a matching manifest, so the real version is taken from ntdll. */
static void about_get_os_name(gchar *buf, gsize len)
{
	typedef LONG (WINAPI *RtlGetVersionFunc)(OSVERSIONINFOEXW *);
	RtlGetVersionFunc rtl_get_version = NULL;
	OSVERSIONINFOEXW info;
	HMODULE ntdll;
	const gchar *bits = sizeof(gpointer) == 8 ? "64-bit" : "32-bit";

	memset(&info, 0, sizeof(info));
	info.dwOSVersionInfoSize = sizeof(info);

	ntdll = GetModuleHandleW(L"ntdll.dll");
	if (ntdll)
		rtl_get_version = (RtlGetVersionFunc)(gpointer)
			GetProcAddress(ntdll, "RtlGetVersion");
	if (!rtl_get_version || rtl_get_version(&info) != 0) {
		g_snprintf(buf, len, "Windows (%s)", bits);
		return;
	}

	if (info.wProductType != VER_NT_WORKSTATION)
		g_snprintf(buf, len, "Windows Server %lu.%lu (%s)",
			   (gulong)info.dwMajorVersion,
			   (gulong)info.dwMinorVersion, bits);
	else if (info.dwMajorVersion == 10 && info.dwBuildNumber >= 22000)
		g_snprintf(buf, len, "Windows 11 (%s)", bits);
	else if (info.dwMajorVersion == 10)
		g_snprintf(buf, len, "Windows 10 (%s)", bits);
	else
		g_snprintf(buf, len, "Windows %lu.%lu (%s)",
			   (gulong)info.dwMajorVersion,
			   (gulong)info.dwMinorVersion, bits);
}
#endif /* G_OS_WIN32 */

static void about_create(void)
{
	GtkWidget *vbox;
	GtkWidget *pixmap;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *scrolledwin;
	GtkWidget *text;
	GtkWidget *confirm_area;
	GtkWidget *ok_button;
	GtkTextBuffer *buffer;
	GtkTextIter iter;

#if HAVE_SYS_UTSNAME_H
	struct utsname utsbuf;
#endif
	gchar buf[1024];
#ifdef G_OS_WIN32
	gchar os_name[64];
#endif
	gint i;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("About"));
	gtk_container_set_border_width(GTK_CONTAINER(window), 8);
	gtk_widget_set_size_request(window, 518 * gtkut_get_dpi_multiplier(), 420 * gtkut_get_dpi_multiplier());
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	g_signal_connect(G_OBJECT(window), "delete_event",
			 G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	g_signal_connect(G_OBJECT(window), "key_press_event",
			 G_CALLBACK(key_pressed), NULL);
	gtk_widget_realize(window);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	pixmap = stock_pixbuf_widget(window, STOCK_PIXMAP_SYLPHEED_LOGO);
	gtk_box_pack_start(GTK_BOX(vbox), pixmap, FALSE, FALSE, 0);

	label = gtk_label_new("Version " VERSION
			      " (Build " Xstr(BUILD_REVISION) ")");
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

#if HAVE_SYS_UTSNAME_H
	uname(&utsbuf);
	g_snprintf(buf, sizeof(buf),
		   "GTK+ %d.%d.%d / GLib %d.%d.%d\n"
		   "Operating System: %s %s (%s)",
		   gtk_major_version, gtk_minor_version, gtk_micro_version,
		   glib_major_version, glib_minor_version, glib_micro_version,
		   utsbuf.sysname, utsbuf.release, utsbuf.machine);
#elif defined(G_OS_WIN32)
	about_get_os_name(os_name, sizeof(os_name));
	g_snprintf(buf, sizeof(buf),
		   "GTK+ %d.%d.%d / GLib %d.%d.%d\n"
		   "Operating System: %s",
		   gtk_major_version, gtk_minor_version, gtk_micro_version,
		   glib_major_version, glib_minor_version, glib_micro_version,
		   os_name);
#else
	g_snprintf(buf, sizeof(buf),
		   "GTK+ %d.%d.%d / GLib %d.%d.%d\n"
		   "Operating System: unknown",
		   gtk_major_version, gtk_minor_version, gtk_micro_version,
		   glib_major_version, glib_minor_version, glib_micro_version);
#endif

	label = gtk_label_new(buf);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* this fork */
	label = gtk_label_new("Forked from Sylpheed " SYLPHEED_BASE_VERSION
			      " by Creath and Claude, 2026");
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_label(" "ZYLPHEED_HOMEPAGE_URI" ");
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(about_fork_uri_clicked), NULL);
	buf[0] = ' ';
	for (i = 1; i <= strlen(ZYLPHEED_HOMEPAGE_URI); i++) buf[i] = '_';
	strcpy(buf + i, " ");
	gtk_label_set_pattern(GTK_LABEL(gtk_bin_get_child(GTK_BIN(button))), buf);
	/* same link colours as the Sylpheed homepage button below */
	gtk_widget_set_name(gtk_bin_get_child(GTK_BIN(button)),
			    "sylpheed-about-uri");

	/* from here on: the original About information of Sylpheed */
	g_snprintf(buf, sizeof(buf),
		   "Compiled-in features:%s",
#if USE_THREADS
		   " gthread"
#endif
#if INET6
		   " IPv6"
#endif
#if HAVE_ICONV
		   " iconv"
#endif
#if HAVE_LIBCOMPFACE
		   " compface"
#endif
#if USE_GPGME
		   " GnuPG"
#endif
#if USE_SSL
		   " OpenSSL"
#endif
#if USE_LDAP
		   " LDAP"
#endif
#if USE_JPILOT
		   " JPilot"
#endif
#if USE_GTKSPELL
		   " GtkSpell"
#endif
#if USE_ONIGURUMA
		   " Oniguruma"
#endif
	"");

	label = gtk_label_new(buf);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new
		("Copyright (C) 1999-2018 Hiroyuki Yamamoto <hiro-y@kcn.ne.jp>");
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_label(" "HOMEPAGE_URI" ");
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(about_uri_clicked), NULL);
	buf[0] = ' ';
	for (i = 1; i <= strlen(HOMEPAGE_URI); i++) buf[i] = '_';
	strcpy(buf + i, " ");
	gtk_label_set_pattern(GTK_LABEL(gtk_bin_get_child(GTK_BIN(button))), buf);
	/* link colours: blue, red while the button is pressed */
	gtk_widget_set_name(gtk_bin_get_child(GTK_BIN(button)),
			    "sylpheed-about-uri");
	gtkut_css_set_rule("#sylpheed-about-uri", "color: #0000ff;");
	gtkut_css_set_rule("button:active #sylpheed-about-uri",
			   "color: #ff0000;");

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
				       GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin),
					    GTK_SHADOW_IN);
	/* a real scrollbar beside the text, not the GTK3 overlay one that is
	   only shown while the pointer moves over the text */
	gtk_scrolled_window_set_overlay_scrolling
		(GTK_SCROLLED_WINDOW(scrolledwin), FALSE);
	gtk_scrolled_window_set_min_content_height
		(GTK_SCROLLED_WINDOW(scrolledwin),
		 ABOUT_TEXT_MIN_HEIGHT * gtkut_get_dpi_multiplier());
	gtk_box_pack_start(GTK_BOX(vbox), scrolledwin, TRUE, TRUE, 0);

	text = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 6);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text), 6);
	gtk_container_add(GTK_CONTAINER(scrolledwin), text);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

#if USE_GPGME
	gtk_text_buffer_insert(buffer, &iter,
		_("GPGME is copyright 2001 by Werner Koch <dd9jn@gnu.org>\n\n"), -1);
#endif /* USE_GPGME */

	gtk_text_buffer_insert(buffer, &iter,
		_("This program is free software; you can redistribute it and/or modify "
		  "it under the terms of the GNU General Public License as published by "
		  "the Free Software Foundation; either version 2, or (at your option) "
		  "any later version.\n\n"), -1);

	gtk_text_buffer_insert(buffer, &iter,
		_("This program is distributed in the hope that it will be useful, "
		  "but WITHOUT ANY WARRANTY; without even the implied warranty of "
		  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. "
		  "See the GNU General Public License for more details.\n\n"), -1);

	gtk_text_buffer_insert(buffer, &iter,
		_("You should have received a copy of the GNU General Public License "
		  "along with this program; if not, write to the Free Software "
		  "Foundation, Inc., 59 Temple Place - Suite 330, Boston, "
		  "MA 02111-1307, USA."), -1);

	gtk_text_buffer_get_start_iter(buffer, &iter);
	gtk_text_buffer_place_cursor(buffer, &iter);

	gtkut_stock_button_set_create(&confirm_area, &ok_button, GTK_STOCK_OK,
				      NULL, NULL, NULL, NULL);
	gtk_box_pack_end(GTK_BOX(vbox), confirm_area, FALSE, FALSE, 0);
	gtk_widget_grab_default(ok_button);
	gtk_widget_grab_focus(ok_button);
	g_signal_connect_closure
		(G_OBJECT(ok_button), "clicked",
		 g_cclosure_new_swap(G_CALLBACK(gtk_widget_hide_on_delete),
				     window, NULL), FALSE);

	gtk_widget_show_all(window);
}

static gboolean key_pressed(GtkWidget *widget, GdkEventKey *event)
{
	if (event && event->keyval == GDK_KEY_Escape)
		gtk_widget_hide(window);
	return FALSE;
}

static void about_uri_clicked(GtkButton *button, gpointer data)
{
	open_uri(HOMEPAGE_URI, prefs_common.uri_cmd);
}

static void about_fork_uri_clicked(GtkButton *button, gpointer data)
{
	open_uri(ZYLPHEED_HOMEPAGE_URI, prefs_common.uri_cmd);
}
