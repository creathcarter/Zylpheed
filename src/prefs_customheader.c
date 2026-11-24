/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto
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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "main.h"
#include "prefs.h"
#include "prefs_ui.h"
#include "prefs_customheader.h"
#include "prefs_common.h"
#include "prefs_account.h"
#include "mainwindow.h"
#include "foldersel.h"
#include "manage_window.h"
#include "customheader.h"
#include "folder.h"
#include "utils.h"
#include "gtkutils.h"
#include "alertpanel.h"

static struct CustomHdr {
	GtkWidget *window;

	GtkWidget *ok_btn;
	GtkWidget *cancel_btn;

	GtkWidget *hdr_combo;
	GtkWidget *hdr_entry;
	GtkWidget *val_entry;
	GtkWidget *customhdr_clist;
	GtkListStore *customhdr_store;
} customhdr;

enum {
	CUSTOMHDR_COL_TEXT,
	CUSTOMHDR_COL_DATA,
	CUSTOMHDR_N_COLS
};

/* widget creating functions */
static void prefs_custom_header_create	(void);

static void prefs_custom_header_set_dialog	(PrefsAccount *ac);
static void prefs_custom_header_set_list	(PrefsAccount *ac);
static gint prefs_custom_header_clist_set_row	(PrefsAccount *ac,
						 gint	       row);

/* callback functions */
static void prefs_custom_header_add_cb		(void);
static void prefs_custom_header_delete_cb	(void);
static void prefs_custom_header_up		(void);
static void prefs_custom_header_down		(void);
static void prefs_custom_header_select		(GtkTreeSelection *selection,
						 gpointer	 data);

static void prefs_custom_header_row_moved	(GtkWidget	*widget,
						 GdkDragContext	*context,
						 gpointer	 data);

static gboolean prefs_custom_header_key_pressed	(GtkWidget	*widget,
						 GdkEventKey	*event,
						 gpointer	 data);
static void prefs_custom_header_ok		(void);
static void prefs_custom_header_cancel		(void);
static gint prefs_custom_header_deleted		(GtkWidget	*widget,
						 GdkEventAny	*event,
						 gpointer	 data);

static PrefsAccount *cur_ac = NULL;

void prefs_custom_header_open(PrefsAccount *ac)
{
	if (!customhdr.window) {
		prefs_custom_header_create();
	}

	manage_window_set_transient(GTK_WINDOW(customhdr.window));
	gtk_widget_grab_focus(customhdr.ok_btn);

	prefs_custom_header_set_dialog(ac);

	cur_ac = ac;

	gtk_widget_show(customhdr.window);
}

static void prefs_custom_header_create(void)
{
	GtkWidget *window;
	GtkWidget *vbox;

	GtkWidget *ok_btn;
	GtkWidget *cancel_btn;

	GtkWidget *confirm_area;

	GtkWidget *vbox1;

	GtkWidget *table1;
	GtkWidget *hdr_label;
	GtkWidget *hdr_combo;
	GtkWidget *val_label;
	GtkWidget *val_entry;

	GtkWidget *reg_hbox;
	GtkWidget *btn_hbox;
	GtkWidget *arrow;
	GtkWidget *add_btn;
	GtkWidget *del_btn;

	GtkWidget *ch_hbox;
	GtkWidget *ch_scrolledwin;
	GtkWidget *customhdr_clist;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	GtkWidget *btn_vbox;
	GtkWidget *up_btn;
	GtkWidget *down_btn;

	debug_print("Creating custom header setting window...\n");

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width (GTK_CONTAINER (window), 8);
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (window), TRUE);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	gtkut_stock_button_set_create(&confirm_area, &ok_btn, GTK_STOCK_OK,
				      &cancel_btn, GTK_STOCK_CANCEL,
				      NULL, NULL);
	gtk_widget_show (confirm_area);
	gtk_box_pack_end (GTK_BOX(vbox), confirm_area, FALSE, FALSE, 0);
	gtk_widget_grab_default (ok_btn);

	gtk_window_set_title (GTK_WINDOW(window), _("Custom header setting"));
	MANAGE_WINDOW_SIGNALS_CONNECT (window);
	g_signal_connect (G_OBJECT(window), "delete_event",
			  G_CALLBACK(prefs_custom_header_deleted),
			  NULL);
	g_signal_connect (G_OBJECT(window), "key_press_event",
			  G_CALLBACK(prefs_custom_header_key_pressed),
			  NULL);
	g_signal_connect (G_OBJECT(ok_btn), "clicked",
			  G_CALLBACK(prefs_custom_header_ok), NULL);
	g_signal_connect (G_OBJECT(cancel_btn), "clicked",
			  G_CALLBACK(prefs_custom_header_cancel), NULL);

	vbox1 = gtk_vbox_new (FALSE, VSPACING);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (vbox), vbox1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 2);

	table1 = gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table1);
	gtk_box_pack_start (GTK_BOX (vbox1), table1,
			    FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 8);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 8);

	hdr_label = gtk_label_new (_("Header"));
	gtk_widget_show (hdr_label);
	gtk_table_attach (GTK_TABLE (table1), hdr_label, 0, 1, 0, 1,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (hdr_label), 0, 0.5);
	
	hdr_combo = gtk_combo_box_text_new_with_entry ();
	gtk_widget_show (hdr_combo);
	gtk_table_attach (GTK_TABLE (table1), hdr_combo, 0, 1, 1, 2,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  0, 0, 0);
	gtk_widget_set_size_request (hdr_combo, 150, -1);
	gtkut_combo_set_items (GTK_COMBO_BOX_TEXT (hdr_combo),
			       "User-Agent", "X-Face", "X-Operating-System",
			       NULL);

	val_label = gtk_label_new (_("Value"));
	gtk_widget_show (val_label);
	gtk_table_attach (GTK_TABLE (table1), val_label, 1, 2, 0, 1,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (val_label), 0, 0.5);
	
	val_entry = gtk_entry_new ();
	gtk_widget_show (val_entry);
	gtk_table_attach (GTK_TABLE (table1), val_entry, 1, 2, 1, 2,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  0, 0, 0);
	gtk_widget_set_size_request (val_entry, 200, -1);

	/* add / delete */

	reg_hbox = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (reg_hbox);
	gtk_box_pack_start (GTK_BOX (vbox1), reg_hbox, FALSE, FALSE, 0);

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_widget_show (arrow);
	gtk_box_pack_start (GTK_BOX (reg_hbox), arrow, FALSE, FALSE, 0);
	gtk_widget_set_size_request (arrow, -1, 16);

	btn_hbox = gtk_hbox_new (TRUE, 4);
	gtk_widget_show (btn_hbox);
	gtk_box_pack_start (GTK_BOX (reg_hbox), btn_hbox, FALSE, FALSE, 0);

	add_btn = gtk_button_new_with_label (_("Add"));
	gtk_widget_show (add_btn);
	gtk_box_pack_start (GTK_BOX (btn_hbox), add_btn, FALSE, TRUE, 0);
	g_signal_connect (G_OBJECT (add_btn), "clicked",
			  G_CALLBACK (prefs_custom_header_add_cb), NULL);

	del_btn = gtk_button_new_with_label (_(" Delete "));
	gtk_widget_show (del_btn);
	gtk_box_pack_start (GTK_BOX (btn_hbox), del_btn, FALSE, TRUE, 0);
	g_signal_connect (G_OBJECT (del_btn), "clicked",
			  G_CALLBACK (prefs_custom_header_delete_cb), NULL);


	ch_hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (ch_hbox);
	gtk_box_pack_start (GTK_BOX (vbox1), ch_hbox, TRUE, TRUE, 0);

	ch_scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_size_request (ch_scrolledwin, -1, 200);
	gtk_widget_show (ch_scrolledwin);
	gtk_box_pack_start (GTK_BOX (ch_hbox), ch_scrolledwin, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ch_scrolledwin),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	store = gtk_list_store_new (CUSTOMHDR_N_COLS,
				    G_TYPE_STRING, G_TYPE_POINTER);
	customhdr_clist = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref (G_OBJECT (store));
	gtk_widget_show (customhdr_clist);
	gtk_container_add (GTK_CONTAINER (ch_scrolledwin), customhdr_clist);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes
		(_("Custom headers"), renderer, "text", CUSTOMHDR_COL_TEXT,
		 NULL);
	gtk_tree_view_column_set_min_width (column, 80);
	gtk_tree_view_append_column (GTK_TREE_VIEW (customhdr_clist), column);
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (customhdr_clist), TRUE);
	selection = gtk_tree_view_get_selection
		(GTK_TREE_VIEW (customhdr_clist));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (prefs_custom_header_select), NULL);
	g_signal_connect_after
		(G_OBJECT (customhdr_clist), "drag_end",
		 G_CALLBACK (prefs_custom_header_row_moved), NULL);

	btn_vbox = gtk_vbox_new (FALSE, 8);
	gtk_widget_show (btn_vbox);
	gtk_box_pack_start (GTK_BOX (ch_hbox), btn_vbox, FALSE, FALSE, 0);

	up_btn = gtk_button_new_with_label (_("Up"));
	gtk_widget_show (up_btn);
	gtk_box_pack_start (GTK_BOX (btn_vbox), up_btn, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (up_btn), "clicked",
			  G_CALLBACK (prefs_custom_header_up), NULL);

	down_btn = gtk_button_new_with_label (_("Down"));
	gtk_widget_show (down_btn);
	gtk_box_pack_start (GTK_BOX (btn_vbox), down_btn, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (down_btn), "clicked",
			  G_CALLBACK (prefs_custom_header_down), NULL);

	gtk_widget_show_all(window);

	customhdr.window     = window;
	customhdr.ok_btn     = ok_btn;
	customhdr.cancel_btn = cancel_btn;

	customhdr.hdr_combo  = hdr_combo;
	customhdr.hdr_entry  = gtk_bin_get_child (GTK_BIN (hdr_combo));
	customhdr.val_entry  = val_entry;

	customhdr.customhdr_clist   = customhdr_clist;
	customhdr.customhdr_store   = store;
}

static void prefs_custom_header_set_dialog(PrefsAccount *ac)
{
	GtkListStore *store = customhdr.customhdr_store;
	GtkTreeIter iter;
	GSList *cur;
	gchar *ch_str;

	gtk_list_store_clear(store);

	for (cur = ac->customhdr_list; cur != NULL; cur = cur->next) {
 		CustomHeader *ch = (CustomHeader *)cur->data;

		ch_str = g_strdup_printf("%s: %s", ch->name,
					 ch->value ? ch->value : "");
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, CUSTOMHDR_COL_TEXT, ch_str,
				   CUSTOMHDR_COL_DATA, ch, -1);

		g_free(ch_str);
	}

	/* GtkCList in browse mode selected the first row */
	gtkut_list_view_select_row
		(GTK_TREE_VIEW(customhdr.customhdr_clist), 0);
}

static void prefs_custom_header_set_list(PrefsAccount *ac)
{
	GtkTreeModel *model = GTK_TREE_MODEL(customhdr.customhdr_store);
	GtkTreeIter iter;
	gboolean valid;
	CustomHeader *ch;

	g_slist_free(ac->customhdr_list);
	ac->customhdr_list = NULL;

	for (valid = gtk_tree_model_get_iter_first(model, &iter); valid;
	     valid = gtk_tree_model_iter_next(model, &iter)) {
		gtk_tree_model_get(model, &iter, CUSTOMHDR_COL_DATA, &ch, -1);
		if (!ch)
			break;
		ac->customhdr_list = g_slist_append(ac->customhdr_list, ch);
	}
}

static gint prefs_custom_header_clist_set_row(PrefsAccount *ac, gint row)
{
	GtkListStore *store = customhdr.customhdr_store;
	GtkTreeIter iter;
	CustomHeader *ch;
	const gchar *entry_text;
	gchar *ch_str;

	entry_text = gtk_entry_get_text(GTK_ENTRY(customhdr.hdr_entry));
	if (entry_text[0] == '\0') {
		alertpanel_error(_("Header name is not set."));
		return -1;
	}

	ch = g_new0(CustomHeader, 1);

	ch->account_id = ac->account_id;

	ch->name = g_strdup(entry_text);
	unfold_line(ch->name);
	g_strstrip(ch->name);
	gtk_entry_set_text(GTK_ENTRY(customhdr.hdr_entry), ch->name);

	entry_text = gtk_entry_get_text(GTK_ENTRY(customhdr.val_entry));
	if (entry_text[0] != '\0') {
		ch->value = g_strdup(entry_text);
		unfold_line(ch->value);
		g_strstrip(ch->value);
		gtk_entry_set_text(GTK_ENTRY(customhdr.val_entry), ch->value);
	}

	ch_str = g_strdup_printf("%s: %s", ch->name,
				 ch->value ? ch->value : "");

	if (row < 0) {
		gtk_list_store_append(store, &iter);
		row = gtk_tree_model_iter_n_children
			(GTK_TREE_MODEL(store), NULL) - 1;
	} else {
		CustomHeader *tmp_ch;

		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter,
					      NULL, row);
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
				   CUSTOMHDR_COL_DATA, &tmp_ch, -1);
		if (tmp_ch)
			custom_header_free(tmp_ch);
	}

	gtk_list_store_set(store, &iter, CUSTOMHDR_COL_TEXT, ch_str,
			   CUSTOMHDR_COL_DATA, ch, -1);

	g_free(ch_str);

	prefs_custom_header_set_list(cur_ac);

	return row;
}

static void prefs_custom_header_add_cb(void)
{
	prefs_custom_header_clist_set_row(cur_ac, -1);
}

static void prefs_custom_header_delete_cb(void)
{
	GtkTreeView *treeview = GTK_TREE_VIEW(customhdr.customhdr_clist);
	GtkTreeModel *model = GTK_TREE_MODEL(customhdr.customhdr_store);
	GtkTreeIter iter;
	CustomHeader *ch;
	gint row;

	row = gtkut_list_view_get_selected_row(treeview);
	if (row < 0) return;

	if (alertpanel(_("Delete header"),
		       _("Do you really want to delete this header?"),
		       GTK_STOCK_YES, GTK_STOCK_NO, NULL) != G_ALERTDEFAULT)
		return;

	gtk_tree_model_iter_nth_child(model, &iter, NULL, row);
	gtk_tree_model_get(model, &iter, CUSTOMHDR_COL_DATA, &ch, -1);
	custom_header_free(ch);
	gtk_list_store_remove(customhdr.customhdr_store, &iter);
	cur_ac->customhdr_list = g_slist_remove(cur_ac->customhdr_list, ch);
}

static void prefs_custom_header_up(void)
{
	if (gtkut_list_view_move_selected_row
		(GTK_TREE_VIEW(customhdr.customhdr_clist), -1) >= 0)
		prefs_custom_header_set_list(cur_ac);
}

static void prefs_custom_header_down(void)
{
	if (gtkut_list_view_move_selected_row
		(GTK_TREE_VIEW(customhdr.customhdr_clist), 1) >= 0)
		prefs_custom_header_set_list(cur_ac);
}

#define ENTRY_SET_TEXT(entry, str) \
	gtk_entry_set_text(GTK_ENTRY(entry), str ? str : "")

static void prefs_custom_header_select(GtkTreeSelection *selection,
				       gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	CustomHeader *ch;
	CustomHeader default_ch = { 0, "", NULL };

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	gtk_tree_model_get(model, &iter, CUSTOMHDR_COL_DATA, &ch, -1);
	if (!ch) ch = &default_ch;

	ENTRY_SET_TEXT(customhdr.hdr_entry, ch->name);
	ENTRY_SET_TEXT(customhdr.val_entry, ch->value);
}

#undef ENTRY_SET_TEXT

static void prefs_custom_header_row_moved(GtkWidget *widget,
					  GdkDragContext *context,
					  gpointer data)
{
	prefs_custom_header_set_list(cur_ac);
}

static gboolean prefs_custom_header_key_pressed(GtkWidget *widget,
						GdkEventKey *event,
						gpointer data)
{
	if (event && event->keyval == GDK_KEY_Escape)
		prefs_custom_header_cancel();
	return FALSE;
}

static void prefs_custom_header_ok(void)
{
	custom_header_write_config(cur_ac);
	gtk_widget_hide(customhdr.window);
}

static void prefs_custom_header_cancel(void)
{
	custom_header_read_config(cur_ac); 
	gtk_widget_hide(customhdr.window);
}

static gint prefs_custom_header_deleted(GtkWidget *widget, GdkEventAny *event,
					gpointer data)
{
	prefs_custom_header_cancel();
	return TRUE;
}
