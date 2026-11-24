/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2015 Hiroyuki Yamamoto
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

#include "prefs.h"
#include "prefs_ui.h"
#include "prefs_common.h"
#include "prefs_display_items.h"
#include "manage_window.h"
#include "mainwindow.h"
#include "gtkutils.h"
#include "utils.h"

#define SCROLLED_WINDOW_WIDTH	180
#define SCROLLED_WINDOW_HEIGHT	210

enum {
	DISPITEM_COL_NAME,
	DISPITEM_COL_DATA,
	DISPITEM_COL_FG,
	DISPITEM_N_COLS
};

static void prefs_display_items_set_sensitive(PrefsDisplayItemsDialog *dialog);

/* callback functions */
static void prefs_display_items_add	(GtkWidget	*widget,
					 gpointer	 data);
static void prefs_display_items_remove	(GtkWidget	*widget,
					 gpointer	 data);

static void prefs_display_items_up	(GtkWidget	*widget,
					 gpointer	 data);
static void prefs_display_items_down	(GtkWidget	*widget,
					 gpointer	 data);

static void prefs_display_items_default	(GtkWidget	*widget,
					 gpointer	 data);

static void prefs_display_items_ok	(GtkWidget	*widget,
					 gpointer	 data);
static void prefs_display_items_cancel	(GtkWidget	*widget,
					 gpointer	 data);

static void prefs_display_items_shown_select_row(GtkTreeSelection *selection,
						 gpointer	 data);

static GtkWidget *prefs_display_items_list_view_create
						(const gchar	*title);

static gint prefs_display_items_delete_event	(GtkWidget	*widget,
						 GdkEventAny	*event,
						 gpointer	 data);
static gboolean prefs_display_items_key_pressed	(GtkWidget	*widget,
						 GdkEventKey	*event,
						 gpointer	 data);

PrefsDisplayItemsDialog *prefs_display_items_dialog_create(void)
{
	PrefsDisplayItemsDialog *dialog;

	GtkWidget *window;
	GtkWidget *vbox;

	GtkWidget *label_hbox;
	GtkWidget *label;

	GtkWidget *vbox1;

	GtkWidget *hbox1;
	GtkWidget *clist_hbox;
	GtkWidget *scrolledwin;
	GtkWidget *stock_clist;
	GtkWidget *shown_clist;

	GtkWidget *btn_vbox;
	GtkWidget *btn_vbox1;
	GtkWidget *add_btn;
	GtkWidget *remove_btn;
	GtkWidget *up_btn;
	GtkWidget *down_btn;

	GtkWidget *btn_hbox;
	GtkWidget *default_btn;
	GtkWidget *confirm_area;
	GtkWidget *ok_btn;
	GtkWidget *cancel_btn;

	dialog = g_new0(PrefsDisplayItemsDialog, 1);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 8);
	gtk_window_set_position(GTK_WINDOW(window),
				GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_window_set_title(GTK_WINDOW(window), _("Display items setting"));
	g_signal_connect(G_OBJECT(window), "delete_event",
			 G_CALLBACK(prefs_display_items_delete_event), dialog);
	g_signal_connect(G_OBJECT(window), "key_press_event",
			 G_CALLBACK(prefs_display_items_key_pressed), dialog);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	label_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(label_hbox);
	gtk_box_pack_start(GTK_BOX(vbox), label_hbox, FALSE, FALSE, 4);

	label = gtk_label_new("");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(label_hbox), label, FALSE, FALSE, 4);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	vbox1 = gtk_vbox_new(FALSE, VSPACING);
	gtk_widget_show(vbox1);
	gtk_box_pack_start(GTK_BOX(vbox), vbox1, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 2);

	hbox1 = gtk_hbox_new(FALSE, 8);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, TRUE, 0);

	clist_hbox = gtk_hbox_new(FALSE, 8);
	gtk_widget_show(clist_hbox);
	gtk_box_pack_start(GTK_BOX(hbox1), clist_hbox, TRUE, TRUE, 0);

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request
		(scrolledwin,
		 SCROLLED_WINDOW_WIDTH * gtkut_get_dpi_multiplier(),
		 SCROLLED_WINDOW_HEIGHT * gtkut_get_dpi_multiplier());
	gtk_widget_show(scrolledwin);
	gtk_box_pack_start(GTK_BOX(clist_hbox), scrolledwin, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	stock_clist = prefs_display_items_list_view_create
		(_("Available items"));
	gtk_container_add(GTK_CONTAINER(scrolledwin), stock_clist);

	/* add/remove button */
	btn_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(btn_vbox);
	gtk_box_pack_start(GTK_BOX(hbox1), btn_vbox, FALSE, FALSE, 0);

	btn_vbox1 = gtk_vbox_new(FALSE, 8);
	gtk_widget_show(btn_vbox1);
	gtk_box_pack_start(GTK_BOX(btn_vbox), btn_vbox1, TRUE, FALSE, 0);

	add_btn = gtk_button_new_with_label(_("  ->  "));
	gtk_widget_show(add_btn);
	gtk_box_pack_start(GTK_BOX(btn_vbox1), add_btn, FALSE, FALSE, 0);

	remove_btn = gtk_button_new_with_label(_("  <-  "));
	gtk_widget_show(remove_btn);
	gtk_box_pack_start(GTK_BOX(btn_vbox1), remove_btn, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(add_btn), "clicked",
			 G_CALLBACK(prefs_display_items_add), dialog);
	g_signal_connect(G_OBJECT(remove_btn), "clicked",
			 G_CALLBACK(prefs_display_items_remove), dialog);

	clist_hbox = gtk_hbox_new(FALSE, 8);
	gtk_widget_show(clist_hbox);
	gtk_box_pack_start(GTK_BOX(hbox1), clist_hbox, TRUE, TRUE, 0);

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request
		(scrolledwin,
		 SCROLLED_WINDOW_WIDTH * gtkut_get_dpi_multiplier(),
		 SCROLLED_WINDOW_HEIGHT * gtkut_get_dpi_multiplier());
	gtk_widget_show(scrolledwin);
	gtk_box_pack_start(GTK_BOX(clist_hbox), scrolledwin, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	shown_clist = prefs_display_items_list_view_create
		(_("Displayed items"));
	gtk_container_add(GTK_CONTAINER(scrolledwin), shown_clist);

	g_signal_connect(G_OBJECT(gtk_tree_view_get_selection
				  (GTK_TREE_VIEW(shown_clist))), "changed",
			 G_CALLBACK(prefs_display_items_shown_select_row),
			 dialog);

	/* up/down button */
	btn_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(btn_vbox);
	gtk_box_pack_start(GTK_BOX(hbox1), btn_vbox, FALSE, FALSE, 0);

	btn_vbox1 = gtk_vbox_new(FALSE, 8);
	gtk_widget_show(btn_vbox1);
	gtk_box_pack_start(GTK_BOX(btn_vbox), btn_vbox1, TRUE, FALSE, 0);

	up_btn = gtk_button_new_with_label(_("Up"));
	gtk_widget_show(up_btn);
	gtk_box_pack_start(GTK_BOX(btn_vbox1), up_btn, FALSE, FALSE, 0);

	down_btn = gtk_button_new_with_label(_("Down"));
	gtk_widget_show(down_btn);
	gtk_box_pack_start(GTK_BOX(btn_vbox1), down_btn, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(up_btn), "clicked",
			 G_CALLBACK(prefs_display_items_up), dialog);
	g_signal_connect(G_OBJECT(down_btn), "clicked",
			 G_CALLBACK(prefs_display_items_down), dialog);

	btn_hbox = gtk_hbox_new(FALSE, 8);
	gtk_widget_show(btn_hbox);
	gtk_box_pack_end(GTK_BOX(vbox), btn_hbox, FALSE, FALSE, 0);

	btn_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(btn_vbox);
	gtk_box_pack_start(GTK_BOX(btn_hbox), btn_vbox, FALSE, FALSE, 0);

	default_btn = gtk_button_new_with_label(_(" Revert to default "));
	gtk_widget_show(default_btn);
	gtk_box_pack_start(GTK_BOX(btn_vbox), default_btn, TRUE, FALSE, 0);

	g_signal_connect(G_OBJECT(default_btn), "clicked",
			 G_CALLBACK(prefs_display_items_default), dialog);

	gtkut_stock_button_set_create(&confirm_area, &ok_btn, GTK_STOCK_OK,
				      &cancel_btn, GTK_STOCK_CANCEL,
				      NULL, NULL);
	gtk_widget_show(confirm_area);
	gtk_box_pack_end(GTK_BOX(btn_hbox), confirm_area, FALSE, FALSE, 0);
	gtk_widget_grab_default(ok_btn);

	g_signal_connect(G_OBJECT(ok_btn), "clicked",
			 G_CALLBACK(prefs_display_items_ok), dialog);
	g_signal_connect(G_OBJECT(cancel_btn), "clicked",
			 G_CALLBACK(prefs_display_items_cancel), dialog);

	dialog->window       = window;
	dialog->label        = label;
	dialog->stock_clist  = stock_clist;
	dialog->shown_clist  = shown_clist;
	dialog->add_btn      = add_btn;
	dialog->remove_btn   = remove_btn;
	dialog->up_btn       = up_btn;
	dialog->down_btn     = down_btn;
	dialog->confirm_area = confirm_area;
	dialog->ok_btn       = ok_btn;
	dialog->cancel_btn   = cancel_btn;

	gtkut_box_set_reverse_order(GTK_BOX(dialog->confirm_area),
				    !prefs_common.comply_gnome_hig);
	manage_window_set_transient(GTK_WINDOW(dialog->window));
	gtk_widget_grab_focus(dialog->ok_btn);

	dialog->finished = FALSE;
	dialog->cancelled = FALSE;

	return dialog;
}

void prefs_display_items_dialog_destroy(PrefsDisplayItemsDialog *dialog)
{
	if (!dialog)
		return;

	if (dialog->available_items)
		g_list_free(dialog->available_items);
	if (dialog->visible_items)
		g_list_free(dialog->visible_items);
	gtk_widget_destroy(dialog->window);
	g_free(dialog);
}

static GtkWidget *prefs_display_items_list_view_create(const gchar *title)
{
	GtkListStore *store;
	GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	store = gtk_list_store_new(DISPITEM_N_COLS, G_TYPE_STRING,
				   G_TYPE_POINTER, GDK_TYPE_RGBA);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	gtk_widget_show(treeview);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes
		(title, renderer, "text", DISPITEM_COL_NAME,
		 "foreground-rgba", DISPITEM_COL_FG, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	gtk_tree_selection_set_mode
		(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
		 GTK_SELECTION_BROWSE);

	return treeview;
}

static void prefs_display_items_update_available
	(PrefsDisplayItemsDialog *dialog)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GList *cur;

	g_return_if_fail(dialog->available_items != NULL);

	store = GTK_LIST_STORE(gtk_tree_view_get_model
			       (GTK_TREE_VIEW(dialog->stock_clist)));
	gtk_list_store_clear(store);

	for (cur = dialog->available_items; cur != NULL; cur = cur->next) {
		PrefsDisplayItem *item = cur->data;
		gchar *name;

		if (item->allow_multiple || item->in_use == FALSE) {
			name = gettext(item->label);
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
					   DISPITEM_COL_NAME, name,
					   DISPITEM_COL_DATA, item, -1);
		}
	}

	/* GtkCList in browse mode selected the first row */
	gtkut_list_view_select_row(GTK_TREE_VIEW(dialog->stock_clist), 0);
}

static PrefsDisplayItem *prefs_display_items_get_item_from_id
	(PrefsDisplayItemsDialog *dialog, gint id)
{
	gint i;

	for (i = 0; dialog->all_items[i].id != -1; i++) {
		if (id == dialog->all_items[i].id)
			return (PrefsDisplayItem *)&dialog->all_items[i];
	}

	return NULL;
}

void prefs_display_items_dialog_set_available(PrefsDisplayItemsDialog *dialog,
					      PrefsDisplayItem *all_items,
					      const gint *ids)
{
	gint i;
	GList *list = NULL;

	dialog->all_items = all_items;
	for (i = 0; ids[i] != -1; i++) {
		PrefsDisplayItem *item;

		item = prefs_display_items_get_item_from_id(dialog, ids[i]);
		if (item)
			list = g_list_append(list, item);
	}
	dialog->available_items = list;
	prefs_display_items_update_available(dialog);
}

void prefs_display_items_dialog_set_default_visible
					(PrefsDisplayItemsDialog *dialog,
					 const gint *ids)
{
	dialog->default_visible_ids = ids;
}

void prefs_display_items_dialog_set_visible(PrefsDisplayItemsDialog *dialog,
					    const gint *ids)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GList *cur;
	PrefsDisplayItem *item;
	gint i;
	gchar *name;

	g_return_if_fail(dialog->available_items != NULL);

	if (!ids)
		ids = dialog->default_visible_ids;
	g_return_if_fail(ids != NULL);

	store = GTK_LIST_STORE(gtk_tree_view_get_model
			       (GTK_TREE_VIEW(dialog->shown_clist)));
	gtk_list_store_clear(store);

	if (dialog->visible_items) {
		g_list_free(dialog->visible_items);
		dialog->visible_items = NULL;
	}

	for (cur = dialog->available_items; cur != NULL; cur = cur->next) {
		item = cur->data;
		item->in_use = FALSE;
	}

	for (i = 0; ids[i] != -1; i++) {
		gint id = ids[i];

		item = prefs_display_items_get_item_from_id(dialog, id);

		g_return_if_fail(item != NULL);
		g_return_if_fail(item->allow_multiple || item->in_use == FALSE);

		item->in_use = TRUE;

		name = gettext(item->label);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, DISPITEM_COL_NAME, name,
				   DISPITEM_COL_DATA, item, -1);
	}

	name = "--------";
	gtk_list_store_append(store, &iter);
	{
		GtkStyleContext *context;
		GdkRGBA color;

		/* the insensitive text colour of the list */
		context = gtk_widget_get_style_context(dialog->shown_clist);
		gtk_style_context_save(context);
		gtk_style_context_set_state(context,
					    GTK_STATE_FLAG_INSENSITIVE);
		gtk_style_context_get_color
			(context, gtk_style_context_get_state(context), &color);
		gtk_style_context_restore(context);
		gtk_list_store_set(store, &iter, DISPITEM_COL_NAME, name,
				   DISPITEM_COL_FG, &color, -1);
	}

	/* GtkCList in browse mode selected the first row */
	gtkut_list_view_select_row(GTK_TREE_VIEW(dialog->shown_clist), 0);

	prefs_display_items_update_available(dialog);
	prefs_display_items_set_sensitive(dialog);
	if (gtk_widget_get_realized(dialog->shown_clist))
		gtk_tree_view_scroll_to_point
			(GTK_TREE_VIEW(dialog->shown_clist), -1, 0);
}

static void prefs_display_items_set_sensitive(PrefsDisplayItemsDialog *dialog)
{
	GtkTreeView *treeview = GTK_TREE_VIEW(dialog->shown_clist);
	GtkTreeModel *model = gtk_tree_view_get_model(treeview);
	GtkTreePath *path;
	gint row, n_rows;

	row = gtkut_list_view_get_selected_row(treeview);
	if (row < 0) return;
	n_rows = gtk_tree_model_iter_n_children(model, NULL);

	if (gtkut_tree_model_get_nth_data(model, row, DISPITEM_COL_DATA))
		gtk_widget_set_sensitive(dialog->remove_btn, TRUE);
	else
		gtk_widget_set_sensitive(dialog->remove_btn, FALSE);

	if (row > 0 && row < n_rows - 1)
		gtk_widget_set_sensitive(dialog->up_btn, TRUE);
	else
		gtk_widget_set_sensitive(dialog->up_btn, FALSE);

	if (row >= 0 && row < n_rows - 2)
		gtk_widget_set_sensitive(dialog->down_btn, TRUE);
	else
		gtk_widget_set_sensitive(dialog->down_btn, FALSE);

	if (gtk_widget_get_realized(GTK_WIDGET(treeview))) {
		path = gtk_tree_path_new_from_indices(row, -1);
		gtkut_tree_view_scroll_to_cell(treeview, path, TRUE);
		gtk_tree_path_free(path);
	}
}

static void prefs_display_items_add(GtkWidget *widget, gpointer data)
{
	PrefsDisplayItemsDialog *dialog = data;
	GtkTreeView *stock_view = GTK_TREE_VIEW(dialog->stock_clist);
	GtkTreeView *shown_view = GTK_TREE_VIEW(dialog->shown_clist);
	GtkListStore *shown_store;
	GtkTreeIter iter;
	PrefsDisplayItem *item;
	gint row;
	gchar *name;

	row = gtkut_list_view_get_selected_row(stock_view);
	if (row < 0) return;

	item = (PrefsDisplayItem *)gtkut_tree_model_get_nth_data
		(gtk_tree_view_get_model(stock_view), row, DISPITEM_COL_DATA);
	if (!item->allow_multiple)
		gtkut_list_view_remove_row(stock_view, row);

	row = gtkut_list_view_get_selected_row(shown_view);
	if (row < 0)
		row = 0;

	item->in_use = TRUE;

	name = gettext(item->label);
	shown_store = GTK_LIST_STORE(gtk_tree_view_get_model(shown_view));
	gtk_list_store_insert(shown_store, &iter, row);
	gtk_list_store_set(shown_store, &iter, DISPITEM_COL_NAME, name,
			   DISPITEM_COL_DATA, item, -1);

	prefs_display_items_set_sensitive(dialog);
}

static void prefs_display_items_remove(GtkWidget *widget, gpointer data)
{
	PrefsDisplayItemsDialog *dialog = data;
	GtkTreeView *shown_view = GTK_TREE_VIEW(dialog->shown_clist);
	PrefsDisplayItem *item;
	gint row;

	row = gtkut_list_view_get_selected_row(shown_view);
	if (row < 0) return;

	item = (PrefsDisplayItem *)gtkut_tree_model_get_nth_data
		(gtk_tree_view_get_model(shown_view), row, DISPITEM_COL_DATA);
	if (!item)
		return;
	gtkut_list_view_remove_row(shown_view, row);

	if (!item->allow_multiple) {
		item->in_use = FALSE;
		prefs_display_items_update_available(dialog);
	}

	prefs_display_items_set_sensitive(dialog);
}

static void prefs_display_items_up(GtkWidget *widget, gpointer data)
{
	PrefsDisplayItemsDialog *dialog = data;
	GtkTreeView *shown_view = GTK_TREE_VIEW(dialog->shown_clist);
	gint row, n_rows;

	row = gtkut_list_view_get_selected_row(shown_view);
	if (row < 0) return;

	n_rows = gtk_tree_model_iter_n_children
		(gtk_tree_view_get_model(shown_view), NULL);
	if (row > 0 && row < n_rows - 1) {
		gtkut_list_view_move_selected_row(shown_view, -1);
		prefs_display_items_set_sensitive(dialog);
	}
}

static void prefs_display_items_down(GtkWidget *widget, gpointer data)
{
	PrefsDisplayItemsDialog *dialog = data;
	GtkTreeView *shown_view = GTK_TREE_VIEW(dialog->shown_clist);
	gint row, n_rows;

	row = gtkut_list_view_get_selected_row(shown_view);
	if (row < 0) return;

	n_rows = gtk_tree_model_iter_n_children
		(gtk_tree_view_get_model(shown_view), NULL);
	if (row >= 0 && row < n_rows - 2) {
		gtkut_list_view_move_selected_row(shown_view, 1);
		prefs_display_items_set_sensitive(dialog);
	}
}

static void prefs_display_items_default(GtkWidget *widget, gpointer data)
{
	PrefsDisplayItemsDialog *dialog = data;

	prefs_display_items_dialog_set_visible(dialog, NULL);
}

static void prefs_display_items_ok(GtkWidget *widget, gpointer data)
{
	PrefsDisplayItemsDialog *dialog = data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	GList *list = NULL;
	PrefsDisplayItem *item;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dialog->shown_clist));
	for (valid = gtk_tree_model_get_iter_first(model, &iter); valid;
	     valid = gtk_tree_model_iter_next(model, &iter)) {
		gtk_tree_model_get(model, &iter, DISPITEM_COL_DATA, &item, -1);
		if (item)
			list = g_list_append(list, item);
	}

	dialog->visible_items = list;
	dialog->finished = TRUE;
}

static void prefs_display_items_cancel(GtkWidget *widget, gpointer data)
{
	PrefsDisplayItemsDialog *dialog = data;

	dialog->finished = TRUE;
	dialog->cancelled = TRUE;
}

static void prefs_display_items_shown_select_row(GtkTreeSelection *selection,
						 gpointer	 data)
{
	PrefsDisplayItemsDialog *dialog = data;

	prefs_display_items_set_sensitive(dialog);
}

static gint prefs_display_items_delete_event(GtkWidget *widget,
					     GdkEventAny *event,
					     gpointer data)
{
	PrefsDisplayItemsDialog *dialog = data;

	dialog->finished = TRUE;
	dialog->cancelled = TRUE;
	return TRUE;
}

static gboolean prefs_display_items_key_pressed(GtkWidget *widget,
						GdkEventKey *event,
						gpointer data)
{
	PrefsDisplayItemsDialog *dialog = data;

	if (event && event->keyval == GDK_KEY_Escape) {
		dialog->finished = TRUE;
		dialog->cancelled = TRUE;
	}
	return FALSE;
}
