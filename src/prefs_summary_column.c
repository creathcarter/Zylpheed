/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2007 Hiroyuki Yamamoto
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
#include "prefs_summary_column.h"
#include "manage_window.h"
#include "summaryview.h"
#include "mainwindow.h"
#include "inc.h"
#include "gtkutils.h"
#include "utils.h"

enum {
	SUMCOL_COL_NAME,
	SUMCOL_COL_TYPE,
	SUMCOL_N_COLS
};

static struct _SummaryColumnDialog
{
	GtkWidget *window;

	GtkWidget *stock_clist;
	GtkWidget *shown_clist;

	GtkWidget *add_btn;
	GtkWidget *remove_btn;
	GtkWidget *up_btn;
	GtkWidget *down_btn;

	GtkWidget *default_btn;

	GtkWidget *confirm_area;
	GtkWidget *ok_btn;
	GtkWidget *cancel_btn;

	gboolean sent_folder;

	gboolean finished;
} summary_col;

static const gchar *const col_name[N_SUMMARY_VISIBLE_COLS] = {
	N_("Mark"),		/* S_COL_MARK    */
	N_("Unread"),		/* S_COL_UNREAD  */
	N_("Attachment"),	/* S_COL_MIME    */
	N_("Subject"),		/* S_COL_SUBJECT */
	N_("From"),		/* S_COL_FROM    */
	N_("Date"),		/* S_COL_DATE    */
	N_("Size"),		/* S_COL_SIZE    */
	N_("Number"),		/* S_COL_NUMBER  */
	N_("To")		/* S_COL_TO      */
};

static SummaryColumnState default_state[N_SUMMARY_VISIBLE_COLS] = {
	{ S_COL_MARK   , TRUE  },
	{ S_COL_UNREAD , TRUE  },
	{ S_COL_MIME   , TRUE  },
	{ S_COL_SUBJECT, TRUE  },
	{ S_COL_FROM   , TRUE  },
	{ S_COL_DATE   , TRUE  },
	{ S_COL_SIZE   , TRUE  },
	{ S_COL_NUMBER , FALSE },
	{ S_COL_TO     , FALSE }
};

static SummaryColumnState default_sent_state[N_SUMMARY_VISIBLE_COLS] = {
	{ S_COL_MARK   , TRUE  },
	{ S_COL_UNREAD , TRUE  },
	{ S_COL_MIME   , TRUE  },
	{ S_COL_SUBJECT, TRUE  },
	{ S_COL_TO     , TRUE  },
	{ S_COL_DATE   , TRUE  },
	{ S_COL_SIZE   , TRUE  },
	{ S_COL_NUMBER , FALSE },
	{ S_COL_FROM   , FALSE }
};

static void prefs_summary_column_create	(void);

static void prefs_summary_column_set_dialog	(SummaryColumnState *state);
static void prefs_summary_column_set_view	(void);

/* callback functions */
static void prefs_summary_column_add	(void);
static void prefs_summary_column_remove	(void);

static void prefs_summary_column_up	(void);
static void prefs_summary_column_down	(void);

static void prefs_summary_column_set_to_default	(void);

static void prefs_summary_column_ok	(void);
static void prefs_summary_column_cancel	(void);

static gint prefs_summary_column_delete_event	(GtkWidget	*widget,
						 GdkEventAny	*event,
						 gpointer	 data);
static gboolean prefs_summary_column_key_pressed(GtkWidget	*widget,
						 GdkEventKey	*event,
						 gpointer	 data);

void prefs_summary_column_open(gboolean sent_folder)
{
	inc_lock();

	prefs_summary_column_create();
	summary_col.sent_folder = sent_folder;

	gtkut_box_set_reverse_order(GTK_BOX(summary_col.confirm_area),
				    !prefs_common.comply_gnome_hig);
	manage_window_set_transient(GTK_WINDOW(summary_col.window));
	gtk_widget_grab_focus(summary_col.ok_btn);

	prefs_summary_column_set_dialog(NULL);

	gtk_widget_show(summary_col.window);

	summary_col.finished = FALSE;
	while (summary_col.finished == FALSE)
		gtk_main_iteration();

	gtk_widget_destroy(summary_col.window);
	summary_col.window = NULL;
	main_window_popup(main_window_get());

	inc_unlock();
}

static void prefs_summary_column_create(void)
{
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

	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	debug_print(_("Creating summary column setting window...\n"));

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 8);
	gtk_window_set_position(GTK_WINDOW(window),
				GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_window_set_title(GTK_WINDOW(window),
			     _("Summary display item setting"));
	g_signal_connect(G_OBJECT(window), "delete_event",
			 G_CALLBACK(prefs_summary_column_delete_event), NULL);
	g_signal_connect(G_OBJECT(window), "key_press_event",
			 G_CALLBACK(prefs_summary_column_key_pressed), NULL);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	label_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(label_hbox);
	gtk_box_pack_start(GTK_BOX(vbox), label_hbox, FALSE, FALSE, 4);

	label = gtk_label_new
		(_("Select items to be displayed on the summary view. You can modify\n"
		   "the order by using the Up / Down button."));
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
	gtk_widget_set_size_request(scrolledwin, 180, 210);
	gtk_widget_show(scrolledwin);
	gtk_box_pack_start(GTK_BOX(clist_hbox), scrolledwin, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	store = gtk_list_store_new(SUMCOL_N_COLS, G_TYPE_STRING, G_TYPE_INT);
	stock_clist = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	gtk_widget_show(stock_clist);
	gtk_container_add(GTK_CONTAINER(scrolledwin), stock_clist);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes
		(_("Available items"), renderer, "text", SUMCOL_COL_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(stock_clist), column);
	gtk_tree_selection_set_mode
		(gtk_tree_view_get_selection(GTK_TREE_VIEW(stock_clist)),
		 GTK_SELECTION_BROWSE);

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
			 G_CALLBACK(prefs_summary_column_add), NULL);
	g_signal_connect(G_OBJECT(remove_btn), "clicked",
			 G_CALLBACK(prefs_summary_column_remove), NULL);

	clist_hbox = gtk_hbox_new(FALSE, 8);
	gtk_widget_show(clist_hbox);
	gtk_box_pack_start(GTK_BOX(hbox1), clist_hbox, TRUE, TRUE, 0);

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scrolledwin, 180, 210);
	gtk_widget_show(scrolledwin);
	gtk_box_pack_start(GTK_BOX(clist_hbox), scrolledwin, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	store = gtk_list_store_new(SUMCOL_N_COLS, G_TYPE_STRING, G_TYPE_INT);
	shown_clist = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	gtk_widget_show(shown_clist);
	gtk_container_add(GTK_CONTAINER(scrolledwin), shown_clist);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes
		(_("Displayed items"), renderer, "text", SUMCOL_COL_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(shown_clist), column);
	gtk_tree_selection_set_mode
		(gtk_tree_view_get_selection(GTK_TREE_VIEW(shown_clist)),
		 GTK_SELECTION_BROWSE);

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
			 G_CALLBACK(prefs_summary_column_up), NULL);
	g_signal_connect(G_OBJECT(down_btn), "clicked",
			 G_CALLBACK(prefs_summary_column_down), NULL);

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
			 G_CALLBACK(prefs_summary_column_set_to_default), NULL);

	gtkut_stock_button_set_create(&confirm_area, &ok_btn, GTK_STOCK_OK,
				      &cancel_btn, GTK_STOCK_CANCEL,
				      NULL, NULL);
	gtk_widget_show(confirm_area);
	gtk_box_pack_end(GTK_BOX(btn_hbox), confirm_area, FALSE, FALSE, 0);
	gtk_widget_grab_default(ok_btn);

	g_signal_connect(G_OBJECT(ok_btn), "clicked",
			 G_CALLBACK(prefs_summary_column_ok), NULL);
	g_signal_connect(G_OBJECT(cancel_btn), "clicked",
			 G_CALLBACK(prefs_summary_column_cancel), NULL);

	summary_col.window       = window;
	summary_col.stock_clist  = stock_clist;
	summary_col.shown_clist  = shown_clist;
	summary_col.add_btn      = add_btn;
	summary_col.remove_btn   = remove_btn;
	summary_col.up_btn       = up_btn;
	summary_col.down_btn     = down_btn;
	summary_col.confirm_area = confirm_area;
	summary_col.ok_btn       = ok_btn;
	summary_col.cancel_btn   = cancel_btn;
}

SummaryColumnState *prefs_summary_column_get_config(gboolean sent_folder)
{
	static SummaryColumnState state[N_SUMMARY_VISIBLE_COLS];
	SummaryColumnType type;
	gboolean *col_visible;
	gint *col_pos;
	SummaryColumnState *def_state;
	gint pos;

	debug_print("prefs_summary_column_get_config(): "
		    "getting %s folder setting\n",
		    sent_folder ? "sent" : "normal");

	if (sent_folder) {
		col_visible = prefs_common.summary_sent_col_visible;
		col_pos = prefs_common.summary_sent_col_pos;
		def_state = default_sent_state;
	} else {
		col_visible = prefs_common.summary_col_visible;
		col_pos = prefs_common.summary_col_pos;
		def_state = default_state;
	}

	for (pos = 0; pos < N_SUMMARY_VISIBLE_COLS; pos++)
		state[pos].type = -1;

	for (type = 0; type < N_SUMMARY_VISIBLE_COLS; type++) {
		pos = col_pos[type];
		if (pos < 0 || pos >= N_SUMMARY_VISIBLE_COLS ||
		    state[pos].type != -1) {
			g_warning("Wrong column position\n");
			prefs_summary_column_set_config(def_state, sent_folder);
			return def_state;
		}

		state[pos].type = type;
		state[pos].visible = col_visible[type];
	}

	return state;
}

void prefs_summary_column_set_config(SummaryColumnState *state,
				     gboolean sent_folder)
{
	SummaryColumnType type;
	gboolean *col_visible;
	gint *col_pos;
	gint pos;

	if (sent_folder) {
		col_visible = prefs_common.summary_sent_col_visible;
		col_pos = prefs_common.summary_sent_col_pos;
	} else {
		col_visible = prefs_common.summary_col_visible;
		col_pos = prefs_common.summary_col_pos;
	}

	for (pos = 0; pos < N_SUMMARY_VISIBLE_COLS; pos++) {
		type = state[pos].type;
		col_visible[type] = state[pos].visible;
		col_pos[type] = pos;
	}
}

static void prefs_summary_column_set_dialog(SummaryColumnState *state)
{
	GtkListStore *stock_store;
	GtkListStore *shown_store;
	GtkTreeIter iter;
	gint pos;
	SummaryColumnType type;
	gchar *name;

	stock_store = GTK_LIST_STORE(gtk_tree_view_get_model
				     (GTK_TREE_VIEW(summary_col.stock_clist)));
	shown_store = GTK_LIST_STORE(gtk_tree_view_get_model
				     (GTK_TREE_VIEW(summary_col.shown_clist)));
	gtk_list_store_clear(stock_store);
	gtk_list_store_clear(shown_store);

	if (!state)
		state = prefs_summary_column_get_config
			(summary_col.sent_folder);

	for (pos = 0; pos < N_SUMMARY_VISIBLE_COLS; pos++) {
		GtkListStore *store;

		type = state[pos].type;
		name = gettext(col_name[type]);

		store = state[pos].visible ? shown_store : stock_store;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, SUMCOL_COL_NAME, name,
				   SUMCOL_COL_TYPE, type, -1);
	}

	/* GtkCList in browse mode selected the first row */
	gtkut_list_view_select_row(GTK_TREE_VIEW(summary_col.stock_clist), 0);
	gtkut_list_view_select_row(GTK_TREE_VIEW(summary_col.shown_clist), 0);
}

static SummaryColumnType prefs_summary_column_get_nth_type(GtkWidget *view,
							   gint row)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
	GtkTreeIter iter;
	gint type = -1;

	if (gtk_tree_model_iter_nth_child(model, &iter, NULL, row))
		gtk_tree_model_get(model, &iter, SUMCOL_COL_TYPE, &type, -1);

	return (SummaryColumnType)type;
}

static void prefs_summary_column_set_view(void)
{
	SummaryColumnState state[N_SUMMARY_VISIBLE_COLS];
	SummaryColumnType type;
	gint row, pos = 0;
	gint stock_rows, shown_rows;

	stock_rows = gtk_tree_model_iter_n_children
		(gtk_tree_view_get_model
		 (GTK_TREE_VIEW(summary_col.stock_clist)), NULL);
	shown_rows = gtk_tree_model_iter_n_children
		(gtk_tree_view_get_model
		 (GTK_TREE_VIEW(summary_col.shown_clist)), NULL);

	g_return_if_fail(stock_rows + shown_rows == N_SUMMARY_VISIBLE_COLS);

	for (row = 0; row < stock_rows; row++) {
		type = prefs_summary_column_get_nth_type
			(summary_col.stock_clist, row);
		state[row].type = type;
		state[row].visible = FALSE;
	}

	pos = row;
	for (row = 0; row < shown_rows; row++) {
		type = prefs_summary_column_get_nth_type
			(summary_col.shown_clist, row);
		state[pos + row].type = type;
		state[pos + row].visible = TRUE;
	}

	prefs_summary_column_set_config(state, summary_col.sent_folder);
	main_window_set_summary_column();
}

static void prefs_summary_column_add(void)
{
	GtkTreeView *stock_view = GTK_TREE_VIEW(summary_col.stock_clist);
	GtkTreeView *shown_view = GTK_TREE_VIEW(summary_col.shown_clist);
	GtkListStore *shown_store;
	GtkTreeIter iter;
	gint row;
	SummaryColumnType type;
	gchar *name;

	row = gtkut_list_view_get_selected_row(stock_view);
	if (row < 0) return;

	type = prefs_summary_column_get_nth_type(summary_col.stock_clist, row);
	gtkut_list_view_remove_row(stock_view, row);

	row = gtkut_list_view_get_selected_row(shown_view);
	if (row < 0)
		row = 0;

	name = gettext(col_name[type]);
	shown_store = GTK_LIST_STORE(gtk_tree_view_get_model(shown_view));
	gtk_list_store_insert(shown_store, &iter, row);
	gtk_list_store_set(shown_store, &iter, SUMCOL_COL_NAME, name,
			   SUMCOL_COL_TYPE, type, -1);
}

static void prefs_summary_column_remove(void)
{
	GtkTreeView *stock_view = GTK_TREE_VIEW(summary_col.stock_clist);
	GtkTreeView *shown_view = GTK_TREE_VIEW(summary_col.shown_clist);
	GtkListStore *stock_store;
	GtkTreeIter iter;
	gint row;
	SummaryColumnType type;
	gchar *name;

	row = gtkut_list_view_get_selected_row(shown_view);
	if (row < 0) return;

	type = prefs_summary_column_get_nth_type(summary_col.shown_clist, row);
	gtkut_list_view_remove_row(shown_view, row);

	row = gtkut_list_view_get_selected_row(stock_view);
	if (row < 0)
		row = 0;
	else
		row++;

	name = gettext(col_name[type]);
	stock_store = GTK_LIST_STORE(gtk_tree_view_get_model(stock_view));
	gtk_list_store_insert(stock_store, &iter, row);
	gtk_list_store_set(stock_store, &iter, SUMCOL_COL_NAME, name,
			   SUMCOL_COL_TYPE, type, -1);
	gtkut_list_view_select_row(stock_view, row);
}

static void prefs_summary_column_up(void)
{
	gtkut_list_view_move_selected_row
		(GTK_TREE_VIEW(summary_col.shown_clist), -1);
}

static void prefs_summary_column_down(void)
{
	gtkut_list_view_move_selected_row
		(GTK_TREE_VIEW(summary_col.shown_clist), 1);
}

static void prefs_summary_column_set_to_default(void)
{
	if (summary_col.sent_folder)
		prefs_summary_column_set_dialog(default_sent_state);
	else
		prefs_summary_column_set_dialog(default_state);
}

static void prefs_summary_column_ok(void)
{
	if (!summary_col.finished) {
		summary_col.finished = TRUE;
		prefs_summary_column_set_view();
	}
}

static void prefs_summary_column_cancel(void)
{
	summary_col.finished = TRUE;
}

static gint prefs_summary_column_delete_event(GtkWidget *widget,
					      GdkEventAny *event,
					      gpointer data)
{
	summary_col.finished = TRUE;
	return TRUE;
}

static gboolean prefs_summary_column_key_pressed(GtkWidget *widget,
						 GdkEventKey *event,
						 gpointer data)
{
	if (event && event->keyval == GDK_KEY_Escape)
		summary_col.finished = TRUE;
	return FALSE;
}
