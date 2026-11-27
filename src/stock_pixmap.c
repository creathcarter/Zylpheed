/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2013 Hiroyuki Yamamoto
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

#include <glib.h>
#include <gtk/gtk.h>

#include "stock_pixmap.h"
#include "gtkutils.h"
#include "utils.h"

#include "icons/address.h"
#include "icons/category.h"
#include "icons/clip.h"
#include "icons/complete.h"
#include "icons/continue.h"
#include "icons/deleted.h"
#include "icons/error.h"
#include "icons/forwarded.h"
#include "icons/interface.h"
#include "icons/jpilot.h"
#include "icons/ldap.h"
#include "icons/linewrap.h"
#include "icons/mark.h"
#include "icons/new.h"
#include "icons/replied.h"
#include "icons/unread.h"
#include "icons/vcard.h"
#include "icons/online.h"
#include "icons/offline.h"
#include "icons/mail.h"
#include "icons/stock_inbox.h"
#include "icons/stock_outbox.h"
#include "icons/stock_mail.h"
#include "icons/stock_attach.h"
#include "icons/stock_mail-compose.h"
#include "icons/stock_mail-compose_16.h"
#include "icons/stock_mail-forward.h"
#include "icons/stock_mail-receive.h"
#include "icons/stock_mail_receive_all.h"
#include "icons/stock_mail-reply.h"
#include "icons/stock_mail-reply-to-all.h"
#include "icons/stock_mail-send.h"
#include "icons/stock_mail_send_queue.h"
#include "icons/stock_insert-file.h"
#include "icons/stock_addressbook.h"
#include "icons/stock_delete.h"
#include "icons/stock_delete_16.h"
#include "icons/stock_spam.h"
#include "icons/stock_spam_16.h"
#include "icons/stock_notspam.h"
#include "icons/stock_hand-signed.h"
#include "icons/stock_sylpheed.h"
#include "icons/stock_sylpheed_16.h"
#include "icons/stock_sylpheed_32.h"
#include "icons/stock_sylpheed_newmail.h"
#include "icons/stock_sylpheed_newmail_16.h"
#include "icons/sylpheed-logo.h"
#include "icons/stock_person.h"
#include "icons/stock_book.h"
#include "icons/folder-close.h"
#include "icons/folder-open.h"
#include "icons/folder-noselect.h"
#include "icons/folder-search.h"
#include "icons/group.h"
#include "icons/html.h"
#include "icons/stock_remote-mailbox.h"
#include "icons/stock_next.h"
#include "icons/stock_prev.h"
#include "icons/stock_search.h"
#include "icons/stock_print.h"
#include "icons/stock_stop.h"
#include "icons/stock_execute.h"
#include "icons/stock_common-prefs.h"
#include "icons/stock_account-prefs.h"
#include "icons/stock_save.h"
#include "icons/stock_editor.h"

typedef struct _StockPixmapData	StockPixmapData;

struct _StockPixmapData
{
	gchar **data;

	GdkPixbuf *pixbuf;
	const guint8 *pixbuf_data;
	gint pixbuf_data_len;

	gchar *icon_name;
	gint size;

	gchar *icon_file;
	GdkPixbuf *icon_file_pixbuf;
};

static StockPixmapData pixmaps[] =
{
	{NULL, NULL, icon_address, sizeof(icon_address), NULL, 0, "address.png"},
	{NULL, NULL, icon_stock_book, sizeof(icon_stock_book), "stock_book", 16, "stock_book.png"},
	{NULL, NULL, icon_category, sizeof(icon_category), NULL, 0, "category.png"},
	{NULL, NULL, icon_clip, sizeof(icon_clip), NULL, 0, "clip.png"},
	{NULL, NULL, icon_complete, sizeof(icon_complete), NULL, 0, "complete.png"},
	{NULL, NULL, icon_continue, sizeof(icon_continue), NULL, 0, "continue.png"},
	{NULL, NULL, icon_deleted, sizeof(icon_deleted), NULL, 0, "deleted.png"},
	{NULL, NULL, icon_folder_close, sizeof(icon_folder_close), "folder-close", 0, "folder-close.png"},
	{NULL, NULL, icon_folder_open, sizeof(icon_folder_open), "folder-open", 0, "folder-open.png"},
	{NULL, NULL, icon_folder_noselect, sizeof(icon_folder_noselect), "folder-noselect", 0, "folder-noselect.png"},
	{NULL, NULL, icon_error, sizeof(icon_error), NULL, 0, "error.png"},
	{NULL, NULL, icon_forwarded, sizeof(icon_forwarded), NULL, 0, "forwarded.png"},
	{NULL, NULL, icon_group, sizeof(icon_group), "group", 0, "group.png"},
	{NULL, NULL, icon_html, sizeof(icon_html), "html", 0, "html.png"},
	{NULL, NULL, icon_interface, sizeof(icon_interface), NULL, 0, "interface.png"},
	{NULL, NULL, icon_jpilot, sizeof(icon_jpilot), NULL, 0, "jpilot.png"},
	{NULL, NULL, icon_ldap, sizeof(icon_ldap), NULL, 0, "ldap.png"},
	{NULL, NULL, icon_linewrap, sizeof(icon_linewrap), NULL, 0, "linewrap.png"},
	{NULL, NULL, icon_mark, sizeof(icon_mark), NULL, 0, "mark.png"},
	{NULL, NULL, icon_new, sizeof(icon_new), NULL, 0, "new.png"},
	{NULL, NULL, icon_replied, sizeof(icon_replied), NULL, 0, "replied.png"},
	{NULL, NULL, icon_unread, sizeof(icon_unread), NULL, 0, "unread.png"},
	{NULL, NULL, icon_vcard, sizeof(icon_vcard), NULL, 0, "vcard.png"},
	{NULL, NULL, icon_online, sizeof(icon_online), NULL, 0, "online.png"},
	{NULL, NULL, icon_offline, sizeof(icon_offline), NULL, 0, "offline.png"},
	{NULL, NULL, icon_mail, sizeof(icon_mail), NULL, 0, "mail.png"},

	{NULL, NULL, icon_stock_inbox, sizeof(icon_stock_inbox), "stock_inbox", 16, "stock_inbox.png"},
	{NULL, NULL, icon_stock_outbox, sizeof(icon_stock_outbox), "stock_outbox", 16, "stock_outbox.png"},
	{NULL, NULL, icon_stock_mail_compose_16, sizeof(icon_stock_mail_compose_16), "stock_mail-compose", 16, "stock_mail-compose_16.png"},
	{NULL, NULL, icon_stock_delete_16, sizeof(icon_stock_delete_16), GTK_STOCK_DELETE, 16, "stock_delete_16.png"},
	{NULL, NULL, icon_stock_mail, sizeof(icon_stock_mail), "stock_mail", 24, "stock_mail.png"},
	{NULL, NULL, icon_stock_attach, sizeof(icon_stock_attach), "stock_attach", 24, "stock_attach.png"},
	{NULL, NULL, icon_stock_mail_compose, sizeof(icon_stock_mail_compose), "stock_mail-compose", 24, "stock_mail-compose.png"},
	{NULL, NULL, icon_stock_mail_forward, sizeof(icon_stock_mail_forward), "stock_mail-forward", 24, "stock_mail-forward.png"},
	{NULL, NULL, icon_stock_mail_receive, sizeof(icon_stock_mail_receive), "stock_mail-receive", 24, "stock_mail-receive.png"},
	{NULL, NULL, icon_stock_mail_receive_all, sizeof(icon_stock_mail_receive_all), NULL, 0, "stock_mail_receive_all.png"},
	{NULL, NULL, icon_stock_mail_reply, sizeof(icon_stock_mail_reply), "stock_mail-reply", 24, "stock_mail-reply.png"},
	{NULL, NULL, icon_stock_mail_reply_to_all, sizeof(icon_stock_mail_reply_to_all), "stock_mail-reply-to-all", 24, "stock_mail-reply-to-all.png"},
	{NULL, NULL, icon_stock_mail_send, sizeof(icon_stock_mail_send), "stock_mail-send", 24, "stock_mail-send.png"},
	{NULL, NULL, icon_stock_mail_send_queue, sizeof(icon_stock_mail_send_queue), NULL, 0, "stock_mail_send_queue.png"},
	{NULL, NULL, icon_stock_insert_file, sizeof(icon_stock_insert_file), "stock_insert-file", 24, "stock_insert-file.png"},
	{NULL, NULL, icon_stock_addressbook, sizeof(icon_stock_addressbook), "stock_addressbook", 24, "stock_addressbook.png"},
	{NULL, NULL, icon_stock_delete, sizeof(icon_stock_delete), GTK_STOCK_DELETE, 24, "stock_delete.png"},
	{NULL, NULL, icon_stock_spam, sizeof(icon_stock_spam), "stock_spam", 24, "stock_spam.png"},
	{NULL, NULL, icon_stock_notspam, sizeof(icon_stock_notspam), "stock_notspam", 24, "stock_notspam.png"},
	{NULL, NULL, icon_stock_hand_signed, sizeof(icon_stock_hand_signed), "stock_hand-signed", 24, "stock_hand-signed.png"},
	{NULL, NULL, icon_stock_sylpheed, sizeof(icon_stock_sylpheed), NULL, 0, "stock_sylpheed.png"},
	{NULL, NULL, icon_stock_sylpheed_16, sizeof(icon_stock_sylpheed_16), NULL, 0, "stock_sylpheed_16.png"},
	{NULL, NULL, icon_stock_sylpheed_32, sizeof(icon_stock_sylpheed_32), NULL, 0, "stock_sylpheed_32.png"},
	{NULL, NULL, icon_stock_sylpheed_newmail, sizeof(icon_stock_sylpheed_newmail), NULL, 0, "stock_sylpheed_newmail.png"},
	{NULL, NULL, icon_stock_sylpheed_newmail_16, sizeof(icon_stock_sylpheed_newmail_16), NULL, 0, "stock_sylpheed_newmail_16.png"},
	{NULL, NULL, icon_sylpheed_logo, sizeof(icon_sylpheed_logo), NULL, 0, "sylpheed-logo.png"},
	{NULL, NULL, icon_stock_person, sizeof(icon_stock_person), "stock_person", 16, "stock_person.png"},
	{NULL, NULL, icon_folder_search, sizeof(icon_folder_search), "folder-search", 0, "folder-search.png"},
	{NULL, NULL, icon_stock_spam_16, sizeof(icon_stock_spam_16), "stock_spam", 16, "stock_spam_16.png"},

	/* for toolbar */
	{NULL, NULL, icon_stock_remote_mailbox, sizeof(icon_stock_remote_mailbox), GTK_STOCK_NETWORK, 24, "remote-mailbox.png"},
	{NULL, NULL, icon_stock_next, sizeof(icon_stock_next), GTK_STOCK_GO_DOWN, 24, "next.png"},
	{NULL, NULL, icon_stock_prev, sizeof(icon_stock_prev), GTK_STOCK_GO_UP, 24, "prev.png"},
	{NULL, NULL, icon_stock_search, sizeof(icon_stock_search), GTK_STOCK_FIND, 24, "search.png"},
	{NULL, NULL, icon_stock_print, sizeof(icon_stock_print), GTK_STOCK_PRINT, 24, "print.png"},
	{NULL, NULL, icon_stock_stop, sizeof(icon_stock_stop), GTK_STOCK_STOP, 24, "stop.png"},
	{NULL, NULL, icon_stock_execute, sizeof(icon_stock_execute), GTK_STOCK_EXECUTE, 24, "execute.png"},
	{NULL, NULL, icon_stock_common_prefs, sizeof(icon_stock_common_prefs), GTK_STOCK_PREFERENCES, 24, "common-prefs.png"},
	{NULL, NULL, icon_stock_account_prefs, sizeof(icon_stock_account_prefs), GTK_STOCK_PREFERENCES, 24, "account-prefs.png"},
	{NULL, NULL, icon_stock_save, sizeof(icon_stock_save), GTK_STOCK_SAVE, 24, "save.png"},
	{NULL, NULL, icon_stock_editor, sizeof(icon_stock_editor), GTK_STOCK_EDIT, 24, "editor.png"},
};

static gchar *theme_dir = NULL;

/* The built-in icons other than the program icons and the logo are
   single-colour line art: only their alpha channel is meaningful. They are
   painted in the text colour of the theme so that they are visible on
   light and dark themes alike. */
static gboolean stock_pixbuf_is_line_art(StockPixmap icon)
{
	switch (icon) {
	case STOCK_PIXMAP_SYLPHEED:
	case STOCK_PIXMAP_SYLPHEED_SMALL:
	case STOCK_PIXMAP_SYLPHEED_32:
	case STOCK_PIXMAP_SYLPHEED_NEWMAIL:
	case STOCK_PIXMAP_SYLPHEED_NEWMAIL_SMALL:
	case STOCK_PIXMAP_SYLPHEED_LOGO:
		return FALSE;
	default:
		return TRUE;
	}
}

static void stock_pixbuf_tint(GdkPixbuf *pixbuf)
{
	static gboolean color_set = FALSE;
	static guchar r, g, b;
	guchar *pixels, *p;
	gint width, height, rowstride, x, y;

	if (!color_set) {
		GtkWidget *window;
		GdkRGBA color;

		/* an unshown toplevel has the default text colour */
		window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_style_context_get_color
			(gtk_widget_get_style_context(window),
			 GTK_STATE_FLAG_NORMAL, &color);
		gtk_widget_destroy(window);
		r = (guchar)(color.red * 255.0 + 0.5);
		g = (guchar)(color.green * 255.0 + 0.5);
		b = (guchar)(color.blue * 255.0 + 0.5);
		color_set = TRUE;
	}

	if (gdk_pixbuf_get_n_channels(pixbuf) != 4 ||
	    gdk_pixbuf_get_bits_per_sample(pixbuf) != 8)
		return;

	pixels = gdk_pixbuf_get_pixels(pixbuf);
	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);
	rowstride = gdk_pixbuf_get_rowstride(pixbuf);

	for (y = 0; y < height; y++) {
		p = pixels + y * rowstride;
		for (x = 0; x < width; x++, p += 4) {
			p[0] = r;
			p[1] = g;
			p[2] = b;
		}
	}
}


GtkWidget *stock_pixbuf_widget(GtkWidget *window, StockPixmap icon)
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail(icon >= 0 && icon < N_STOCK_PIXMAPS, NULL);

	stock_pixbuf_gdk(window, icon, &pixbuf);
	return gtk_image_new_from_pixbuf(pixbuf);
}

GtkWidget *stock_pixbuf_widget_scale(GtkWidget *window, StockPixmap icon,
				     gint width, gint height)
{
	GdkPixbuf *pixbuf;
	GdkPixbuf *scaled_pixbuf;
	GtkWidget *image;

	g_return_val_if_fail(icon >= 0 && icon < N_STOCK_PIXMAPS, NULL);

	stock_pixbuf_gdk(window, icon, &pixbuf);
	scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, width, height,
						GDK_INTERP_HYPER);
	image = gtk_image_new_from_pixbuf(scaled_pixbuf);
	g_object_unref(scaled_pixbuf);

	return image;
}

static gboolean stock_pixbuf_load_theme_icon_file(StockPixmap icon)
{
	StockPixmapData *pix_d;

	g_return_val_if_fail(icon >= 0 && icon < N_STOCK_PIXMAPS, FALSE);

	pix_d = &pixmaps[icon];

	if (!pix_d->icon_file_pixbuf && pix_d->icon_file && theme_dir) {
		gchar *path;

		path = g_strconcat(theme_dir, G_DIR_SEPARATOR_S,
				   pix_d->icon_file, NULL);
		debug_print("stock_pixbuf_load_theme_icon_file: checking theme icon: %s\n", path);
		if (is_file_exist(path)) {
			debug_print("stock_pixbuf_load_theme_icon_file: loading theme icon: %s\n", path);
			pix_d->icon_file_pixbuf = gdk_pixbuf_new_from_file(path, NULL);
			if (!pix_d->icon_file_pixbuf) {
				g_warning("stock_pixbuf_load_theme_icon_file: loading of theme icon failed: %s", path);
				pix_d->icon_file = NULL;
			} else if (!pix_d->pixbuf)
				pix_d->pixbuf = pix_d->icon_file_pixbuf;
		} else
			pix_d->icon_file = NULL;
		g_free(path);
	}

	return pix_d->icon_file_pixbuf != NULL;
}

GtkWidget *stock_pixbuf_widget_for_toolbar(StockPixmap icon)
{
	GtkWidget *image = NULL;
	StockPixmapData *pix_d;

	g_return_val_if_fail(icon >= 0 && icon < N_STOCK_PIXMAPS, NULL);

	stock_pixbuf_load_theme_icon_file(icon);

	pix_d = &pixmaps[icon];

	if (pix_d->icon_file_pixbuf) {
		image = gtk_image_new_from_pixbuf(pix_d->icon_file_pixbuf);
	}

	/* the built-in artwork takes precedence over the stock icons of the
	   theme */
	if (!image && pix_d->icon_name && !pix_d->pixbuf_data) {
		if (gtk_icon_factory_lookup_default(pix_d->icon_name))
			image = gtk_image_new_from_stock(pix_d->icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
		else
			pix_d->icon_name = NULL;
	}

	if (!image)
		image = stock_pixbuf_widget(NULL, icon);

	return image;
}

gint stock_pixbuf_gdk(GtkWidget *window, StockPixmap icon, GdkPixbuf **pixbuf)
{
	StockPixmapData *pix_d;

	if (pixbuf)
		*pixbuf = NULL;

	g_return_val_if_fail(icon >= 0 && icon < N_STOCK_PIXMAPS, -1);

	stock_pixbuf_load_theme_icon_file(icon);

	pix_d = &pixmaps[icon];

	if (!pix_d->pixbuf && pix_d->pixbuf_data) {
		pix_d->pixbuf = gdk_pixbuf_new_from_inline
			(pix_d->pixbuf_data_len, pix_d->pixbuf_data,
			 TRUE, NULL);
		if (pix_d->pixbuf && stock_pixbuf_is_line_art(icon))
			stock_pixbuf_tint(pix_d->pixbuf);
	}
	if (!pix_d->pixbuf && pix_d->icon_name)
		pix_d->pixbuf = gtk_icon_theme_load_icon
			(gtk_icon_theme_get_default(),
			 pix_d->icon_name, pix_d->size, 0, NULL);
	if (!pix_d->pixbuf && pix_d->data)
		pix_d->pixbuf = gdk_pixbuf_new_from_xpm_data
			((const gchar **)pix_d->data);
	if (!pix_d->pixbuf) {
		g_warning("can't read image %d\n", icon);
		return -1;
	}

	if (pixbuf)
		*pixbuf = pix_d->pixbuf;

	return 0;
}

gint stock_pixbuf_set_theme_dir(const gchar *dir)
{
	g_free(theme_dir);
	theme_dir = g_strdup(dir);

	return 0;
}
