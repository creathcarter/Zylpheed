/*
 * Sylpheed templates subsystem 
 * Copyright (C) 2001 Alexander Barinov
 * Copyright (C) 2001-2015 Hiroyuki Yamamoto
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

#include "defs.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "template.h"
#include "main.h"
#include "inc.h"
#include "utils.h"
#include "gtkutils.h"
#include "alertpanel.h"
#include "manage_window.h"
#include "prefs_common.h"
#include "prefs_common_dialog.h"
#include "compose.h"
#include "mainwindow.h"
#include "addr_compl.h"
#include "quote_fmt.h"
#include "plugin.h"

enum {
	TMPL_COL_NAME,
	TMPL_COL_DATA,
	TMPL_N_COLS
};

static struct Templates {
	GtkWidget *window;
	GtkWidget *clist_tmpls;
	GtkWidget *entry_name;
	GtkWidget *entry_to;
	GtkWidget *entry_cc;
	GtkWidget *entry_bcc;
	GtkWidget *entry_replyto;
	GtkWidget *entry_subject;
	GtkWidget *text_value;
	GtkWidget *confirm_area;
	GtkWidget *ok_btn;
	GtkWidget *cancel_btn;

	gboolean entry_modified;
	gboolean list_modified;
} templates;

/* widget creating functions */
static void prefs_template_window_create	(void);
static void prefs_template_window_setup		(void);
static void prefs_template_clear		(void);

static GSList *prefs_template_get_list		(void);

/* callbacks */
static gint prefs_template_deleted_cb		(GtkWidget	*widget,
						 GdkEventAny	*event,
						 gpointer	 data);
static gboolean prefs_template_key_pressed_cb	(GtkWidget	*widget,
						 GdkEventKey	*event,
						 gpointer	 data);
static void prefs_template_changed_cb		(GtkEditable	*editable,
						 gpointer	 data);
static void prefs_template_cancel_cb		(void);
static void prefs_template_ok_cb		(void);
static void prefs_template_select_cb		(GtkTreeSelection *selection,
						 gpointer	 data);
static void prefs_template_register_cb		(void);
static void prefs_template_substitute_cb	(void);
static void prefs_template_delete_cb		(void);
static void prefs_template_up_cb		(void);
static void prefs_template_down_cb		(void);

/* Called from mainwindow.c */
void prefs_template_open(void)
{
	inc_lock();

	if (!templates.window)
		prefs_template_window_create();

	prefs_template_window_setup();
	gtk_widget_show(templates.window);

	syl_plugin_signal_emit("prefs-template-open", templates.window);
}

#define ADD_ENTRY(entry, str, row) \
{ \
	label1 = gtk_label_new(str); \
	gtk_widget_show(label1); \
	gtk_table_attach(GTK_TABLE(table), label1, 0, 1, row, (row + 1), \
			 GTK_FILL, 0, 0, 0); \
	gtk_misc_set_alignment(GTK_MISC(label1), 1, 0.5); \
 \
	entry = gtk_entry_new(); \
	gtk_widget_show(entry); \
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, row, (row + 1), \
			 GTK_EXPAND|GTK_SHRINK|GTK_FILL, 0, 0, 0); \
	g_signal_connect(G_OBJECT(entry), "changed", \
			 G_CALLBACK(prefs_template_changed_cb), NULL); \
}

static void prefs_template_window_create(void)
{
	/* window structure ;) */
	GtkWidget *window;
	GtkWidget   *vpaned;
	GtkWidget     *vbox1;
	GtkWidget       *hbox1;
	GtkWidget         *label1;
	GtkWidget         *entry_name;
	GtkWidget       *table;
	GtkWidget         *entry_to;
	GtkWidget         *entry_cc;
	GtkWidget         *entry_bcc;
	GtkWidget         *entry_replyto;
	GtkWidget         *entry_subject;
	GtkWidget       *scroll2;
	GtkWidget         *text_value;
	GtkWidget     *vbox2;
	GtkWidget       *hbox2;
	GtkWidget         *arrow1;
	GtkWidget         *hbox3;
	GtkWidget           *reg_btn;
	GtkWidget           *subst_btn;
	GtkWidget           *del_btn;
	GtkWidget         *desc_btn;
	GtkWidget       *hbox4;
	GtkWidget         *scroll1;
	GtkWidget           *clist_tmpls;
	GtkWidget         *vbox3;
	GtkWidget           *vbox4;
	GtkWidget             *up_btn;
	GtkWidget             *down_btn;
	GtkWidget       *confirm_area;
	GtkWidget         *ok_btn;
	GtkWidget         *cancel_btn;

	GtkTextBuffer *buffer;

	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	/* main window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

	/* vpaned to separate template settings from templates list */
	vpaned = gtk_vpaned_new();
	gtk_widget_show(vpaned);
	gtk_container_add(GTK_CONTAINER(window), vpaned);

	/* vbox to handle template name and content */
	vbox1 = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(vbox1);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 8);
	gtk_paned_pack1(GTK_PANED(vpaned), vbox1, FALSE, FALSE);

	hbox1 = gtk_hbox_new(FALSE, 8);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);

	label1 = gtk_label_new(_("Template name"));
	gtk_widget_show(label1);
	gtk_box_pack_start(GTK_BOX(hbox1), label1, FALSE, FALSE, 0);

	entry_name = gtk_entry_new();
	gtk_widget_show(entry_name);
	gtk_box_pack_start(GTK_BOX(hbox1), entry_name, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(entry_name), "changed",
			 G_CALLBACK(prefs_template_changed_cb), NULL);

	/* table for headers */
	table = gtk_table_new(5, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(vbox1), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 4);

	ADD_ENTRY(entry_to, _("To:"), 0);
	address_completion_register_entry(GTK_ENTRY(entry_to));
	ADD_ENTRY(entry_cc, _("Cc:"), 1);
	address_completion_register_entry(GTK_ENTRY(entry_cc));
	ADD_ENTRY(entry_bcc, _("Bcc:"), 2);
	address_completion_register_entry(GTK_ENTRY(entry_bcc));
	ADD_ENTRY(entry_replyto, _("Reply-To:"), 3);
	address_completion_register_entry(GTK_ENTRY(entry_replyto));
	ADD_ENTRY(entry_subject, _("Subject:"), 4);

#undef ADD_ENTRY

	/* template content */
	scroll2 = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll2);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll2),
				       GTK_POLICY_NEVER,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll2),
					    GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox1), scroll2, TRUE, TRUE, 0);

	text_value = gtk_text_view_new();
	gtk_widget_show(text_value);
	gtk_widget_set_size_request
		(text_value,
		 360 * gtkut_get_dpi_multiplier(),
		 120 * gtkut_get_dpi_multiplier());
	gtk_container_add(GTK_CONTAINER(scroll2), text_value);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_value), TRUE);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_value));
	g_signal_connect(G_OBJECT(buffer), "changed",
			 G_CALLBACK(prefs_template_changed_cb), NULL);

	/* vbox for buttons and templates list */
	vbox2 = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(vbox2);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 8);
	gtk_paned_pack2(GTK_PANED(vpaned), vbox2, TRUE, FALSE);

	/* register | substitute | delete */
	hbox2 = gtk_hbox_new(FALSE, 4);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox2, FALSE, FALSE, 0);

	arrow1 = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_widget_show(arrow1);
	gtk_box_pack_start(GTK_BOX(hbox2), arrow1, FALSE, FALSE, 0);
	gtk_widget_set_size_request(arrow1, -1,
				    16 * gtkut_get_dpi_multiplier());

	hbox3 = gtk_hbox_new(TRUE, 4);
	gtk_widget_show(hbox3);
	gtk_box_pack_start(GTK_BOX(hbox2), hbox3, FALSE, FALSE, 0);

	reg_btn = gtk_button_new_with_label(_("Register"));
	gtk_widget_show(reg_btn);
	gtk_box_pack_start(GTK_BOX(hbox3), reg_btn, FALSE, TRUE, 0);
	g_signal_connect(G_OBJECT (reg_btn), "clicked",
			 G_CALLBACK (prefs_template_register_cb), NULL);

	subst_btn = gtk_button_new_with_label(_(" Substitute "));
	gtk_widget_show(subst_btn);
	gtk_box_pack_start(GTK_BOX(hbox3), subst_btn, FALSE, TRUE, 0);
	g_signal_connect(G_OBJECT(subst_btn), "clicked",
			 G_CALLBACK(prefs_template_substitute_cb), NULL);

	del_btn = gtk_button_new_with_label(_("Delete"));
	gtk_widget_show(del_btn);
	gtk_box_pack_start(GTK_BOX(hbox3), del_btn, FALSE, TRUE, 0);
	g_signal_connect(G_OBJECT(del_btn), "clicked",
			 G_CALLBACK(prefs_template_delete_cb), NULL);

	desc_btn = gtk_button_new_with_label(_(" Symbols "));
	gtk_widget_show(desc_btn);
	gtk_box_pack_end(GTK_BOX(hbox2), desc_btn, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(desc_btn), "clicked",
			 G_CALLBACK(prefs_quote_description), NULL);

	/* templates list */
	hbox4 = gtk_hbox_new(FALSE, 8);
	gtk_widget_show(hbox4);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox4, TRUE, TRUE, 0);

	scroll1 = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll1);
	gtk_box_pack_start(GTK_BOX(hbox4), scroll1, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll1),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	store = gtk_list_store_new(TMPL_N_COLS, G_TYPE_STRING, G_TYPE_POINTER);
	clist_tmpls = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	gtk_widget_show(clist_tmpls);
	gtk_widget_set_size_request(scroll1, -1,
				    160 * gtkut_get_dpi_multiplier());
	gtk_container_add(GTK_CONTAINER(scroll1), clist_tmpls);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes
		(_("Registered templates"), renderer, "text", TMPL_COL_NAME,
		 NULL);
	gtk_tree_view_column_set_min_width(column, 80);
	gtk_tree_view_append_column(GTK_TREE_VIEW(clist_tmpls), column);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(clist_tmpls));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
	g_signal_connect(G_OBJECT(selection), "changed",
			 G_CALLBACK(prefs_template_select_cb), NULL);

	vbox3 = gtk_vbox_new(TRUE, 0);
	gtk_widget_show(vbox3);
	gtk_box_pack_start(GTK_BOX(hbox4), vbox3, FALSE, FALSE, 0);

	vbox4 = gtk_vbox_new(TRUE, 8);
	gtk_widget_show(vbox4);
	gtk_box_pack_start(GTK_BOX(vbox3), vbox4, TRUE, FALSE, 0);

	up_btn = gtk_button_new_with_label(_("Up"));
	gtk_widget_show(up_btn);
	gtk_box_pack_start(GTK_BOX(vbox4), up_btn, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT (up_btn), "clicked",
			 G_CALLBACK (prefs_template_up_cb), NULL);

	down_btn = gtk_button_new_with_label(_("Down"));
	gtk_widget_show(down_btn);
	gtk_box_pack_start(GTK_BOX(vbox4), down_btn, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT (down_btn), "clicked",
			 G_CALLBACK (prefs_template_down_cb), NULL);

	/* ok | cancel */
	gtkut_stock_button_set_create(&confirm_area, &ok_btn, GTK_STOCK_OK,
				      &cancel_btn, GTK_STOCK_CANCEL,
				      NULL, NULL);
	gtk_widget_show(confirm_area);
	gtk_box_pack_end(GTK_BOX(vbox2), confirm_area, FALSE, FALSE, 0);
	gtk_widget_grab_default(ok_btn);

	gtk_window_set_title(GTK_WINDOW(window), _("Templates"));

	g_signal_connect(G_OBJECT(window), "delete_event",
			 G_CALLBACK(prefs_template_deleted_cb), NULL);
	g_signal_connect(G_OBJECT(window), "key_press_event",
			 G_CALLBACK(prefs_template_key_pressed_cb), NULL);
	MANAGE_WINDOW_SIGNALS_CONNECT(window);
	g_signal_connect(G_OBJECT(ok_btn), "clicked",
			 G_CALLBACK(prefs_template_ok_cb), NULL);
	g_signal_connect(G_OBJECT(cancel_btn), "clicked",
			 G_CALLBACK(prefs_template_cancel_cb), NULL);

	address_completion_start(window);

	templates.window = window;
	templates.clist_tmpls = clist_tmpls;
	templates.entry_name = entry_name;
	templates.entry_to = entry_to;
	templates.entry_cc = entry_cc;
	templates.entry_bcc = entry_bcc;
	templates.entry_replyto = entry_replyto;
	templates.entry_subject = entry_subject;
	templates.text_value = text_value;
	templates.confirm_area = confirm_area;
	templates.ok_btn = ok_btn;
	templates.cancel_btn = cancel_btn;
	templates.entry_modified = FALSE;
	templates.list_modified = FALSE;
}

static void prefs_template_window_setup(void)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GSList *tmpl_list;
	GSList *cur;
	Template *tmpl;

	gtkut_box_set_reverse_order(GTK_BOX(templates.confirm_area),
				    !prefs_common.comply_gnome_hig);
	manage_window_set_transient(GTK_WINDOW(templates.window));
	gtk_widget_grab_focus(templates.ok_btn);

	store = GTK_LIST_STORE(gtk_tree_view_get_model
			       (GTK_TREE_VIEW(templates.clist_tmpls)));
	gtk_list_store_clear(store);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, TMPL_COL_NAME, _("(New)"),
			   TMPL_COL_DATA, NULL, -1);

	tmpl_list = template_read_config();

	for (cur = tmpl_list; cur != NULL; cur = cur->next) {
		tmpl = (Template *)cur->data;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, TMPL_COL_NAME, tmpl->name,
				   TMPL_COL_DATA, tmpl, -1);
	}

	g_slist_free(tmpl_list);

	/* GtkCList in browse mode selected the first row */
	gtkut_list_view_select_row(GTK_TREE_VIEW(templates.clist_tmpls), 0);

	templates.entry_modified = FALSE;
	templates.list_modified = FALSE;
}

static void prefs_template_clear(void)
{
	GtkTreeModel *model;
	Template *tmpl;
	gint row = 1;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(templates.clist_tmpls));
	while ((tmpl = gtkut_tree_model_get_nth_data
		(model, row, TMPL_COL_DATA)) != NULL) {
		template_free(tmpl);
		row++;
	}

	gtk_list_store_clear(GTK_LIST_STORE(model));
}

static gint prefs_template_deleted_cb(GtkWidget *widget, GdkEventAny *event,
				      gpointer data)
{
	prefs_template_cancel_cb();
	return TRUE;
}

static gboolean prefs_template_key_pressed_cb(GtkWidget *widget,
					      GdkEventKey *event, gpointer data)
{
	if (event && event->keyval == GDK_KEY_Escape)
		prefs_template_cancel_cb();
	return FALSE;
}

static void prefs_template_changed_cb(GtkEditable *editable, gpointer data)
{
	templates.entry_modified = TRUE;
}

static void prefs_template_ok_cb(void)
{
	GSList *tmpl_list;

	if (templates.entry_modified) {
		if (alertpanel(_("Template is modified"),
			       _("Current modification is not applied. Finish without saving it?"),
			       GTK_STOCK_YES, GTK_STOCK_NO, NULL)
		    != G_ALERTDEFAULT)
			return;
	}

	tmpl_list = prefs_template_get_list();
	template_set_config(tmpl_list);
	compose_reflect_prefs_all();
	gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model
					    (GTK_TREE_VIEW(templates.clist_tmpls))));
	gtk_widget_hide(templates.window);
	main_window_popup(main_window_get());
	inc_unlock();
}

static void prefs_template_cancel_cb(void)
{
	if (templates.entry_modified || templates.list_modified) {
		if (alertpanel(_("Templates are modified"),
			       _("Really discard modification to templates?"),
			       GTK_STOCK_YES, GTK_STOCK_NO, NULL)
		    != G_ALERTDEFAULT)
			return;
	}

	prefs_template_clear();
	gtk_widget_hide(templates.window);
	main_window_popup(main_window_get());
	inc_unlock();
}

static void prefs_template_select_cb(GtkTreeSelection *selection,
				     gpointer data)
{
	Template *tmpl;
	Template tmpl_def;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GtkTreeModel *model;
	GtkTreeIter row_iter;

	if (!gtk_tree_selection_get_selected(selection, &model, &row_iter))
		return;

	tmpl_def.name = _("Template");
	tmpl_def.subject = "";
	tmpl_def.to = "";
	tmpl_def.cc = "";
	tmpl_def.bcc = "";
	tmpl_def.replyto = "";
	tmpl_def.value = "";

	gtk_tree_model_get(model, &row_iter, TMPL_COL_DATA, &tmpl, -1);
	if (!tmpl)
		tmpl = &tmpl_def;

	gtk_entry_set_text(GTK_ENTRY(templates.entry_name), tmpl->name);
	gtk_entry_set_text(GTK_ENTRY(templates.entry_to),
			   tmpl->to ? tmpl->to : "");
	gtk_entry_set_text(GTK_ENTRY(templates.entry_cc),
			   tmpl->cc ? tmpl->cc : "");
	gtk_entry_set_text(GTK_ENTRY(templates.entry_bcc),
			   tmpl->bcc ? tmpl->bcc : "");
	gtk_entry_set_text(GTK_ENTRY(templates.entry_replyto),
			   tmpl->replyto ? tmpl->replyto : "");
	gtk_entry_set_text(GTK_ENTRY(templates.entry_subject),
			   tmpl->subject ? tmpl->subject : "");

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(templates.text_value));
	gtk_text_buffer_set_text(buffer, "", 0);
	gtk_text_buffer_get_start_iter(buffer, &iter);
	gtk_text_buffer_insert(buffer, &iter, tmpl->value, -1);

	templates.entry_modified = FALSE;
}

static GSList *prefs_template_get_list(void)
{
	GtkTreeModel *model;
	gint row = 1;
	GSList *tmpl_list = NULL;
	Template *tmpl;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(templates.clist_tmpls));
	while ((tmpl = gtkut_tree_model_get_nth_data
		(model, row, TMPL_COL_DATA)) != NULL) {
		tmpl->tmplid = row;
		tmpl_list = g_slist_append(tmpl_list, tmpl);
		row++;
	}

	return tmpl_list;
}

static gint prefs_template_clist_set_row(gint row)
{
	GtkTreeModel *model;
	GtkTreeIter row_iter;
	Template *tmpl;
	Template *tmp_tmpl;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar *name;
	gchar *to;
	gchar *cc;
	gchar *bcc;
	gchar *replyto;
	gchar *subject;
	gchar *value;

	g_return_val_if_fail(row != 0, -1);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(templates.clist_tmpls));

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(templates.text_value));
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	value = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	if (value && *value != '\0') {
		gchar *parsed_buf;
		MsgInfo dummyinfo;

		memset(&dummyinfo, 0, sizeof(MsgInfo));
		quote_fmt_init(&dummyinfo, NULL, NULL);
		quote_fmt_scan_string(value);
		quote_fmt_parse();
		parsed_buf = quote_fmt_get_buffer();
		if (!parsed_buf) {
			alertpanel_error(_("Template format error."));
			g_free(value);
			return -1;
		}
	}

	name = gtk_editable_get_chars(GTK_EDITABLE(templates.entry_name),
				      0, -1);
	to = gtk_editable_get_chars(GTK_EDITABLE(templates.entry_to), 0, -1);
	cc = gtk_editable_get_chars(GTK_EDITABLE(templates.entry_cc), 0, -1);
	bcc = gtk_editable_get_chars(GTK_EDITABLE(templates.entry_bcc), 0, -1);
	replyto = gtk_editable_get_chars(GTK_EDITABLE(templates.entry_replyto),
					 0, -1);
	subject = gtk_editable_get_chars(GTK_EDITABLE(templates.entry_subject),
					 0, -1);

#define NULLIFY_IF_EMPTY(val)		\
	if (val && *val == '\0') {	\
		g_free(val);		\
		val = NULL;		\
	}

	NULLIFY_IF_EMPTY(to);
	NULLIFY_IF_EMPTY(cc);
	NULLIFY_IF_EMPTY(bcc);
	NULLIFY_IF_EMPTY(replyto);
	NULLIFY_IF_EMPTY(subject);

#undef NULLIFY_IF_EMPTY

	tmpl = g_new(Template, 1);
	tmpl->name = name;
	tmpl->to = to;
	tmpl->cc = cc;
	tmpl->bcc = bcc;
	tmpl->replyto = replyto;
	tmpl->subject = subject;
	tmpl->value = value;

	if (row < 0) {
		gtk_list_store_append(GTK_LIST_STORE(model), &row_iter);
		row = gtk_tree_model_iter_n_children(model, NULL) - 1;
	} else {
		gtk_tree_model_iter_nth_child(model, &row_iter, NULL, row);
		gtk_tree_model_get(model, &row_iter, TMPL_COL_DATA, &tmp_tmpl,
				   -1);
		if (tmp_tmpl)
			template_free(tmp_tmpl);
	}

	gtk_list_store_set(GTK_LIST_STORE(model), &row_iter,
			   TMPL_COL_NAME, name, TMPL_COL_DATA, tmpl, -1);
	templates.entry_modified = FALSE;
	templates.list_modified = TRUE;

	return row;
}

static void prefs_template_register_cb(void)
{
	prefs_template_clist_set_row(-1);
}

static void prefs_template_substitute_cb(void)
{
	GtkTreeView *treeview = GTK_TREE_VIEW(templates.clist_tmpls);
	Template *tmpl;
	gint row;

	row = gtkut_list_view_get_selected_row(treeview);
	if (row < 0) return;
	if (row == 0) return;

	tmpl = gtkut_tree_model_get_nth_data(gtk_tree_view_get_model(treeview),
					     row, TMPL_COL_DATA);
	if (!tmpl) return;

	prefs_template_clist_set_row(row);
}

static void prefs_template_delete_cb(void)
{
	GtkTreeView *treeview = GTK_TREE_VIEW(templates.clist_tmpls);
	Template *tmpl;
	gint row;

	row = gtkut_list_view_get_selected_row(treeview);
	if (row < 0) return;
	if (row == 0) return;

	if (alertpanel(_("Delete template"),
		       _("Do you really want to delete this template?"),
		       GTK_STOCK_YES, GTK_STOCK_NO, NULL) != G_ALERTDEFAULT)
		return;

	tmpl = gtkut_tree_model_get_nth_data(gtk_tree_view_get_model(treeview),
					     row, TMPL_COL_DATA);
	template_free(tmpl);
	gtkut_list_view_remove_row(treeview, row);
	templates.list_modified = TRUE;
}

static void prefs_template_up_cb(void)
{
	GtkTreeView *treeview = GTK_TREE_VIEW(templates.clist_tmpls);
	gint row;

	row = gtkut_list_view_get_selected_row(treeview);
	if (row < 0) return;
	if (row > 1) {
		gtkut_list_view_move_selected_row(treeview, -1);
		templates.list_modified = TRUE;
	}
}

static void prefs_template_down_cb(void)
{
	GtkTreeView *treeview = GTK_TREE_VIEW(templates.clist_tmpls);
	gint row, n_rows;

	row = gtkut_list_view_get_selected_row(treeview);
	if (row < 0) return;
	n_rows = gtk_tree_model_iter_n_children
		(gtk_tree_view_get_model(treeview), NULL);
	if (row > 0 && row < n_rows - 1) {
		gtkut_list_view_move_selected_row(treeview, 1);
		templates.list_modified = TRUE;
	}
}
