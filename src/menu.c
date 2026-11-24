/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2012 Hiroyuki Yamamoto
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "menu.h"
#include "utils.h"

/*
 * Menu factory.
 *
 * Every registered widget (the root container, each menu item) holds one
 * reference on the factory, released when the widget is destroyed, so
 * the factory lives exactly as long as any of its widgets.  Items are
 * kept in a hash table keyed by their full path without mnemonic
 * underscores, for example "<Main>/View/Sort/by date".
 */

#define MENU_FACTORY_KEY	"Sylpheed::MenuFactory"
#define MENU_FACTORY_PATH_KEY	"Sylpheed::MenuFactory::Path"
#define MENU_FACTORY_BRANCH_KEY	"Sylpheed::MenuFactory::Branch"

struct _MenuFactory
{
	gchar *path;
	GtkAccelGroup *accel_group;
	GtkWidget *widget;
	GHashTable *items;
	gint ref_count;
};

typedef struct _MenuFactoryCBData
{
	MenuFactoryCallback func;
	guint callback_type;
	gpointer func_data;
	guint callback_action;
} MenuFactoryCBData;

typedef void (*MenuFactoryCallback1)	(gpointer	 callback_data,
					 guint		 callback_action,
					 GtkWidget	*widget);
typedef void (*MenuFactoryCallback2)	(GtkWidget	*widget,
					 gpointer	 callback_data,
					 guint		 callback_action);

static void menu_factory_unref		(MenuFactory	*factory);
static void menu_factory_widget_destroyed
					(GtkWidget	*widget,
					 gpointer	 data);
static void menu_factory_add_widget	(MenuFactory	*factory,
					 const gchar	*fpath,
					 GtkWidget	*widget,
					 gboolean	 is_branch);

static const gchar *menu_translate(const gchar *path)
{
	return gettext(path);
}

/* remove mnemonic underscores ("__" stands for a literal '_') */
static gchar *menu_factory_strip_path(const gchar *str)
{
	gchar *path, *q;
	const gchar *p;

	path = g_malloc(strlen(str) + 1);
	for (p = str, q = path; *p != '\0'; p++) {
		if (*p == '_') {
			if (p[1] == '_') {
				p++;
				*q++ = '_';
			}
		} else
			*q++ = *p;
	}
	*q = '\0';

	return path;
}

static gchar *menu_factory_full_path(MenuFactory *factory, const gchar *path)
{
	if (path[0] == '<')
		return g_strdup(path);
	return g_strconcat(factory->path, path, NULL);
}

MenuFactory *menu_factory_new(GType container_type, const gchar *path,
			      GtkAccelGroup *accel_group)
{
	MenuFactory *factory;
	GtkWidget *widget;

	g_return_val_if_fail(path != NULL && path[0] == '<', NULL);
	g_return_val_if_fail(g_type_is_a(container_type, GTK_TYPE_MENU_SHELL),
			     NULL);

	factory = g_new0(MenuFactory, 1);
	factory->path = g_strdup(path);
	if (accel_group)
		factory->accel_group = g_object_ref(accel_group);
	else
		factory->accel_group = gtk_accel_group_new();
	factory->items = g_hash_table_new_full(g_str_hash, g_str_equal,
					       g_free, NULL);
	factory->ref_count = 0;

	widget = g_object_new(container_type, NULL);
	g_object_ref_sink(widget);
	factory->widget = widget;
	if (GTK_IS_MENU(widget))
		gtk_menu_set_accel_group(GTK_MENU(widget),
					 factory->accel_group);
	menu_factory_add_widget(factory, path, widget, FALSE);

	return factory;
}

static void menu_factory_unref(MenuFactory *factory)
{
	g_return_if_fail(factory->ref_count > 0);

	if (--factory->ref_count > 0)
		return;

	g_hash_table_destroy(factory->items);
	g_object_unref(factory->accel_group);
	g_free(factory->path);
	g_free(factory);
}

static void menu_factory_widget_destroyed(GtkWidget *widget, gpointer data)
{
	MenuFactory *factory = (MenuFactory *)data;
	const gchar *fpath;

	/* "destroy" can be emitted more than once on GTK+ 2 */
	g_signal_handlers_disconnect_by_func
		(widget, menu_factory_widget_destroyed, data);

	fpath = g_object_get_data(G_OBJECT(widget), MENU_FACTORY_PATH_KEY);
	if (fpath &&
	    g_hash_table_lookup(factory->items, fpath) == (gpointer)widget)
		g_hash_table_remove(factory->items, fpath);
	g_object_set_data(G_OBJECT(widget), MENU_FACTORY_KEY, NULL);

	if (widget == factory->widget) {
		factory->widget = NULL;
		g_object_unref(widget);
	}

	menu_factory_unref(factory);
}

static void menu_factory_add_widget(MenuFactory *factory, const gchar *fpath,
				    GtkWidget *widget, gboolean is_branch)
{
	g_hash_table_replace(factory->items, g_strdup(fpath), widget);
	g_object_set_data_full(G_OBJECT(widget), MENU_FACTORY_PATH_KEY,
			       g_strdup(fpath), g_free);
	g_object_set_data(G_OBJECT(widget), MENU_FACTORY_KEY, factory);
	if (is_branch)
		g_object_set_data(G_OBJECT(widget), MENU_FACTORY_BRANCH_KEY,
				  GINT_TO_POINTER(1));
	factory->ref_count++;
	g_signal_connect(G_OBJECT(widget), "destroy",
			 G_CALLBACK(menu_factory_widget_destroyed), factory);
}

static void menu_factory_cb_data_free(gpointer data, GObject *where)
{
	g_free(data);
}

static void menu_factory_callback_marshal(GtkWidget *widget, gpointer data)
{
	MenuFactoryCBData *cb_data = (MenuFactoryCBData *)data;

	if (cb_data->callback_type == 1) {
		MenuFactoryCallback1 func1 =
			(MenuFactoryCallback1)cb_data->func;
		func1(cb_data->func_data, cb_data->callback_action, widget);
	} else if (cb_data->callback_type == 2) {
		MenuFactoryCallback2 func2 =
			(MenuFactoryCallback2)cb_data->func;
		func2(widget, cb_data->func_data, cb_data->callback_action);
	}
}

enum {
	MF_ITEM,
	MF_TITLE,
	MF_CHECK_ITEM,
	MF_TOGGLE_ITEM,
	MF_RADIO_ITEM,
	MF_SEPARATOR,
	MF_TEAROFF,
	MF_BRANCH,
	MF_LAST_BRANCH
};

void menu_factory_create_item(MenuFactory *factory, MenuFactoryEntry *entry,
			      gpointer callback_data, guint callback_type)
{
	const gchar *item_type;
	gint type_id;
	GType type;
	GSList *radio_group = NULL;
	gchar *path;
	gchar *parent_path;
	gchar *fpath;
	const gchar *translation;
	const gchar *name;
	GtkWidget *parent;
	GtkWidget *widget;
	gchar *p;

	g_return_if_fail(factory != NULL);
	g_return_if_fail(entry != NULL);
	g_return_if_fail(entry->path != NULL);
	g_return_if_fail(entry->path[0] == '/');
	g_return_if_fail(callback_type >= 1 && callback_type <= 2);

	item_type = entry->item_type;
	if (!item_type || item_type[0] == '\0' ||
	    !strcmp(item_type, "<Item>"))
		type_id = MF_ITEM;
	else if (!strcmp(item_type, "<Title>"))
		type_id = MF_TITLE;
	else if (!strcmp(item_type, "<CheckItem>"))
		type_id = MF_CHECK_ITEM;
	else if (!strcmp(item_type, "<ToggleItem>"))
		type_id = MF_TOGGLE_ITEM;
	else if (!strcmp(item_type, "<RadioItem>"))
		type_id = MF_RADIO_ITEM;
	else if (!strcmp(item_type, "<Separator>"))
		type_id = MF_SEPARATOR;
	else if (!strcmp(item_type, "<Tearoff>"))
		type_id = MF_TEAROFF;
	else if (!strcmp(item_type, "<Branch>"))
		type_id = MF_BRANCH;
	else if (!strcmp(item_type, "<LastBranch>"))
		type_id = MF_LAST_BRANCH;
	else {
		GtkWidget *radio_link;

		/* path of the radio item whose group to join */
		radio_link = menu_factory_get_widget(factory, item_type);
		if (radio_link && GTK_IS_RADIO_MENU_ITEM(radio_link)) {
			type_id = MF_RADIO_ITEM;
			radio_group = gtk_radio_menu_item_get_group
				(GTK_RADIO_MENU_ITEM(radio_link));
		} else {
			g_warning("MenuFactory: entry path `%s' has invalid "
				  "type `%s'", entry->path, item_type);
			return;
		}
	}

	switch (type_id) {
	case MF_TITLE:
	case MF_ITEM:
		type = GTK_TYPE_MENU_ITEM; break;
	case MF_CHECK_ITEM:
	case MF_TOGGLE_ITEM:
		type = GTK_TYPE_CHECK_MENU_ITEM; break;
	case MF_RADIO_ITEM:
		type = GTK_TYPE_RADIO_MENU_ITEM; break;
	case MF_SEPARATOR:
		type = GTK_TYPE_SEPARATOR_MENU_ITEM; break;
	case MF_TEAROFF:
		type = GTK_TYPE_TEAROFF_MENU_ITEM; break;
	default:
		type = GTK_TYPE_MENU_ITEM; break;
	}

	path = menu_factory_strip_path(entry->path);
	parent_path = g_strdup(path);
	p = strrchr(parent_path, '/');
	*p = '\0';

	parent = menu_factory_get_widget(factory, parent_path);
	if (!parent) {
		MenuFactoryEntry pentry = {NULL, NULL, NULL, 0, "<Branch>"};
		gchar *ppath;

		ppath = g_strdup(entry->path);
		p = strrchr(ppath, '/');
		*p = '\0';
		pentry.path = ppath;
		menu_factory_create_item(factory, &pentry, NULL, 1);
		g_free(ppath);

		parent = menu_factory_get_widget(factory, parent_path);
	}
	g_free(parent_path);
	if (!parent || !GTK_IS_MENU_SHELL(parent)) {
		g_warning("MenuFactory: no parent menu for `%s'", entry->path);
		g_free(path);
		return;
	}

	translation = menu_translate(entry->path);
	p = strrchr(translation, '/');
	name = p ? p + 1 : translation;

	widget = g_object_new(type, NULL);
	gtk_widget_set_sensitive(widget, type_id != MF_SEPARATOR &&
				 type_id != MF_TITLE);
	gtk_widget_show(widget);
	gtk_menu_shell_append(GTK_MENU_SHELL(parent), widget);

	if (GTK_IS_RADIO_MENU_ITEM(widget))
		gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(widget),
					      radio_group);

	if (type_id != MF_SEPARATOR && type_id != MF_TEAROFF &&
	    *name != '\0') {
		GtkWidget *label;

		label = gtk_accel_label_new("");
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_label_set_text_with_mnemonic(GTK_LABEL(label), name);
		gtk_container_add(GTK_CONTAINER(widget), label);
		gtk_accel_label_set_accel_widget(GTK_ACCEL_LABEL(label),
						 widget);
		gtk_widget_show(label);
	}

	fpath = menu_factory_full_path(factory, path);

	/* accelerator path in the GtkAccelMap, as with GtkItemFactory */
	if (type_id != MF_BRANCH && type_id != MF_LAST_BRANCH) {
		if (entry->accelerator) {
			guint keyval;
			GdkModifierType mods;

			gtk_accelerator_parse(entry->accelerator,
					      &keyval, &mods);
			gtk_accel_map_add_entry(fpath, keyval, mods);
		}
		gtk_widget_set_accel_path(widget, fpath,
					  factory->accel_group);
	}

	if (type_id == MF_BRANCH || type_id == MF_LAST_BRANCH) {
		GtkWidget *submenu;

		if (type_id == MF_LAST_BRANCH)
			gtk_menu_item_set_right_justified
				(GTK_MENU_ITEM(widget), TRUE);

		submenu = gtk_menu_new();
		gtk_menu_set_accel_group(GTK_MENU(submenu),
					 factory->accel_group);
		gtk_menu_set_accel_path(GTK_MENU(submenu), fpath);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(widget), submenu);
		g_object_set_data(G_OBJECT(submenu), MENU_FACTORY_KEY,
				  factory);
	} else if (entry->callback) {
		MenuFactoryCBData *cb_data;

		cb_data = g_new(MenuFactoryCBData, 1);
		cb_data->func = entry->callback;
		cb_data->callback_type = callback_type;
		cb_data->func_data = callback_data;
		cb_data->callback_action = entry->callback_action;
		g_object_weak_ref(G_OBJECT(widget), menu_factory_cb_data_free,
				  cb_data);
		g_signal_connect(G_OBJECT(widget), "activate",
				 G_CALLBACK(menu_factory_callback_marshal),
				 cb_data);
	}

	menu_factory_add_widget(factory, fpath, widget,
				type_id == MF_BRANCH ||
				type_id == MF_LAST_BRANCH);

	g_free(fpath);
	g_free(path);
}

void menu_factory_create_items(MenuFactory *factory, guint n_entries,
			       MenuFactoryEntry *entries,
			       gpointer callback_data)
{
	guint i;

	g_return_if_fail(factory != NULL);
	g_return_if_fail(n_entries == 0 || entries != NULL);

	for (i = 0; i < n_entries; i++)
		menu_factory_create_item(factory, entries + i, callback_data,
					 1);
}

MenuFactory *menu_factory_from_widget(GtkWidget *widget)
{
	MenuFactory *factory;

	g_return_val_if_fail(GTK_IS_WIDGET(widget), NULL);

	factory = g_object_get_data(G_OBJECT(widget), MENU_FACTORY_KEY);
	if (!factory && GTK_IS_MENU_ITEM(widget)) {
		GtkWidget *submenu;

		submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget));
		if (submenu)
			factory = g_object_get_data(G_OBJECT(submenu),
						    MENU_FACTORY_KEY);
	}

	return factory;
}

static GtkWidget *menu_factory_lookup(MenuFactory *factory, const gchar *path)
{
	GtkWidget *widget;
	gchar *fpath;

	fpath = menu_factory_full_path(factory, path);
	widget = g_hash_table_lookup(factory->items, fpath);
	g_free(fpath);

	return widget;
}

/* Returns the submenu for a branch, the widget itself otherwise. */
GtkWidget *menu_factory_get_widget(MenuFactory *factory, const gchar *path)
{
	GtkWidget *widget;

	g_return_val_if_fail(factory != NULL, NULL);
	g_return_val_if_fail(path != NULL, NULL);

	widget = menu_factory_lookup(factory, path);
	if (widget && GTK_IS_MENU_ITEM(widget) &&
	    g_object_get_data(G_OBJECT(widget), MENU_FACTORY_BRANCH_KEY))
		widget = gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget));

	return widget;
}

/* Returns the menu item (also for a branch), or NULL. */
GtkWidget *menu_factory_get_item(MenuFactory *factory, const gchar *path)
{
	GtkWidget *widget;

	g_return_val_if_fail(factory != NULL, NULL);
	g_return_val_if_fail(path != NULL, NULL);

	widget = menu_factory_lookup(factory, path);
	if (widget && GTK_IS_MENU_ITEM(widget))
		return widget;

	return NULL;
}

GtkAccelGroup *menu_factory_get_accel_group(MenuFactory *factory)
{
	g_return_val_if_fail(factory != NULL, NULL);

	return factory->accel_group;
}

void menu_factory_destroy(MenuFactory *factory)
{
	g_return_if_fail(factory != NULL);

	if (factory->widget)
		gtk_widget_destroy(factory->widget);
}

GtkWidget *menubar_create(GtkWidget *window, MenuFactoryEntry *entries,
			  guint n_entries, const gchar *path, gpointer data)
{
	MenuFactory *factory;

	factory = menu_factory_new(GTK_TYPE_MENU_BAR, path, NULL);
	menu_factory_create_items(factory, n_entries, entries, data);
	gtk_window_add_accel_group(GTK_WINDOW(window), factory->accel_group);

	return menu_factory_get_widget(factory, path);
}

GtkWidget *menu_create_items(MenuFactoryEntry *entries,
			     guint n_entries, const gchar *path,
			     MenuFactory **factory, gpointer data)
{
	*factory = menu_factory_new(GTK_TYPE_MENU, path, NULL);
	menu_factory_create_items(*factory, n_entries, entries, data);

	return menu_factory_get_widget(*factory, path);
}

void menu_set_sensitive(MenuFactory *ifactory, const gchar *path,
			gboolean sensitive)
{
	GtkWidget *widget;

	g_return_if_fail(ifactory != NULL);

	widget = menu_factory_get_item(ifactory, path);
	gtk_widget_set_sensitive(widget, sensitive);
}

void menu_set_sensitive_all(GtkMenuShell *menu_shell, gboolean sensitive)
{
	GList *children;
	GList *cur;

	children = gtk_container_get_children(GTK_CONTAINER(menu_shell));
	for (cur = children; cur != NULL; cur = cur->next)
		gtk_widget_set_sensitive(GTK_WIDGET(cur->data), sensitive);
	g_list_free(children);
}

void menu_set_active(MenuFactory *ifactory, const gchar *path,
		     gboolean is_active)
{
	GtkWidget *widget;

	g_return_if_fail(ifactory != NULL);

	widget = menu_factory_get_item(ifactory, path);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), is_active);
}

void menu_button_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in,
			  gpointer user_data)
{
	GtkWidget *button;
	GtkRequisition requisition;
	GtkAllocation allocation;
	gint button_xpos, button_ypos;
	gint xpos, ypos;
	gint width, height;
	gint scr_width, scr_height;

	g_return_if_fail(x != NULL && y != NULL && push_in != NULL);

	button = GTK_WIDGET(user_data);

	gtk_widget_get_child_requisition(GTK_WIDGET(menu), &requisition);
	width = requisition.width;
	height = requisition.height;
	gdk_window_get_origin(gtk_widget_get_window(button), &button_xpos, &button_ypos);

	gtk_widget_get_allocation(button, &allocation);
	xpos = button_xpos + allocation.x;
	ypos = button_ypos + allocation.y + allocation.height;

	scr_width = gdk_screen_width();
	scr_height = gdk_screen_height();

	if (xpos + width > scr_width)
		xpos -= (xpos + width) - scr_width;
	if (ypos + height > scr_height)
		ypos -= allocation.height + height;
	if (xpos < 0)
		xpos = 0;
	if (ypos < 0)
		ypos = 0;

	*x = xpos;
	*y = ypos;
	*push_in = FALSE;
}

void menu_widget_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in,
			  gpointer user_data)
{
	GtkWidget *widget;
	GtkRequisition requisition;
	gint w_xpos, w_ypos;
	gint xpos, ypos;
	gint width, height;
	gint scr_width, scr_height;

	g_return_if_fail(x != NULL && y != NULL && push_in != NULL);

	widget = GTK_WIDGET(user_data);

	gtk_widget_get_child_requisition(GTK_WIDGET(menu), &requisition);
	width = requisition.width;
	height = requisition.height;
	gdk_window_get_origin(gtk_widget_get_window(widget), &w_xpos, &w_ypos);

	xpos = w_xpos;
	ypos = w_ypos;

	scr_width = gdk_screen_width();
	scr_height = gdk_screen_height();

	if (xpos + width > scr_width)
		xpos -= (xpos + width) - scr_width;
	if (ypos + height > scr_height)
		ypos -= height;
	if (xpos < 0)
		xpos = 0;
	if (ypos < 0)
		ypos = 0;

	*x = xpos;
	*y = ypos;
	*push_in = FALSE;
}

/*
 * Option combo box: a GtkComboBox with a (label, value) list, replacing
 * GtkOptionMenu.  The value plays the role of the MENU_VAL_ID object data
 * of the option menu items.  A NULL label adds a separator row.
 */

enum {
	MENU_COMBO_COL_LABEL,
	MENU_COMBO_COL_DATA,
	MENU_COMBO_COL_SENSITIVE,
	MENU_COMBO_COL_SEPARATOR,
	MENU_COMBO_COL_EXTRA,
	MENU_COMBO_N_COLS
};

static gboolean menu_combo_is_separator(GtkTreeModel *model, GtkTreeIter *iter,
					gpointer data)
{
	gboolean separator = FALSE;

	gtk_tree_model_get(model, iter, MENU_COMBO_COL_SEPARATOR, &separator,
			   -1);

	return separator;
}

GtkWidget *menu_combo_new(void)
{
	GtkListStore *store;
	GtkWidget *combo;
	GtkCellRenderer *renderer;

	store = gtk_list_store_new(MENU_COMBO_N_COLS, G_TYPE_STRING,
				   G_TYPE_POINTER, G_TYPE_BOOLEAN,
				   G_TYPE_BOOLEAN, G_TYPE_POINTER);
	combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer,
				       "text", MENU_COMBO_COL_LABEL,
				       "sensitive", MENU_COMBO_COL_SENSITIVE,
				       NULL);
	gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(combo),
					     menu_combo_is_separator,
					     NULL, NULL);

	return combo;
}

gint menu_combo_append(GtkComboBox *combo, const gchar *label, gpointer data)
{
	return menu_combo_insert(combo, -1, label, data);
}

/* position -1 appends; returns the index of the new row */
gint menu_combo_insert(GtkComboBox *combo, gint position, const gchar *label,
		       gpointer data)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreePath *path;
	gint index;

	g_return_val_if_fail(GTK_IS_COMBO_BOX(combo), -1);

	store = GTK_LIST_STORE(gtk_combo_box_get_model(combo));
	if (position < 0)
		gtk_list_store_append(store, &iter);
	else
		gtk_list_store_insert(store, &iter, position);
	gtk_list_store_set(store, &iter,
			   MENU_COMBO_COL_LABEL, label ? label : "",
			   MENU_COMBO_COL_DATA, data,
			   MENU_COMBO_COL_SENSITIVE, label != NULL,
			   MENU_COMBO_COL_SEPARATOR, label == NULL,
			   -1);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);

	/* GtkOptionMenu showed its first item */
	if (gtk_combo_box_get_active(combo) < 0)
		gtk_combo_box_set_active(combo, 0);

	return index;
}

void menu_combo_remove(GtkComboBox *combo, gint index)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_if_fail(GTK_IS_COMBO_BOX(combo));

	model = gtk_combo_box_get_model(combo);
	if (gtk_tree_model_iter_nth_child(model, &iter, NULL, index))
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
}

/* an additional pointer stored with a row (object data of the old menu
   item) */
void menu_combo_set_nth_extra(GtkComboBox *combo, gint index, gpointer extra)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_if_fail(GTK_IS_COMBO_BOX(combo));

	model = gtk_combo_box_get_model(combo);
	if (gtk_tree_model_iter_nth_child(model, &iter, NULL, index))
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				   MENU_COMBO_COL_EXTRA, extra, -1);
}

gpointer menu_combo_get_nth_extra(GtkComboBox *combo, gint index)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gpointer extra = NULL;

	g_return_val_if_fail(GTK_IS_COMBO_BOX(combo), NULL);

	model = gtk_combo_box_get_model(combo);
	if (index >= 0 &&
	    gtk_tree_model_iter_nth_child(model, &iter, NULL, index))
		gtk_tree_model_get(model, &iter, MENU_COMBO_COL_EXTRA, &extra,
				   -1);

	return extra;
}

void menu_combo_clear(GtkComboBox *combo)
{
	g_return_if_fail(GTK_IS_COMBO_BOX(combo));

	gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(combo)));
}

gint menu_combo_get_n_items(GtkComboBox *combo)
{
	g_return_val_if_fail(GTK_IS_COMBO_BOX(combo), 0);

	return gtk_tree_model_iter_n_children
		(gtk_combo_box_get_model(combo), NULL);
}

gpointer menu_combo_get_nth_data(GtkComboBox *combo, gint index)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gpointer data = NULL;

	g_return_val_if_fail(GTK_IS_COMBO_BOX(combo), NULL);

	model = gtk_combo_box_get_model(combo);
	if (index >= 0 &&
	    gtk_tree_model_iter_nth_child(model, &iter, NULL, index))
		gtk_tree_model_get(model, &iter, MENU_COMBO_COL_DATA, &data,
				   -1);

	return data;
}

gpointer menu_combo_get_active_data(GtkComboBox *combo)
{
	g_return_val_if_fail(GTK_IS_COMBO_BOX(combo), NULL);

	return menu_combo_get_nth_data(combo, gtk_combo_box_get_active(combo));
}

gint menu_combo_set_active_data(GtkComboBox *combo, gpointer data)
{
	gint index;

	index = menu_find_option_menu_index(combo, data, NULL);
	if (index >= 0)
		gtk_combo_box_set_active(combo, index);

	return index;
}

void menu_combo_set_item_sensitive(GtkComboBox *combo, gint index,
				   gboolean sensitive)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_if_fail(GTK_IS_COMBO_BOX(combo));

	model = gtk_combo_box_get_model(combo);
	if (gtk_tree_model_iter_nth_child(model, &iter, NULL, index))
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				   MENU_COMBO_COL_SENSITIVE, sensitive, -1);
}

void menu_combo_set_sensitive_all(GtkComboBox *combo, gboolean sensitive)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gboolean separator;

	g_return_if_fail(GTK_IS_COMBO_BOX(combo));

	model = gtk_combo_box_get_model(combo);
	for (valid = gtk_tree_model_get_iter_first(model, &iter); valid;
	     valid = gtk_tree_model_iter_next(model, &iter)) {
		gtk_tree_model_get(model, &iter,
				   MENU_COMBO_COL_SEPARATOR, &separator, -1);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				   MENU_COMBO_COL_SENSITIVE,
				   separator ? FALSE : sensitive, -1);
	}
}

gint menu_find_option_menu_index(GtkComboBox *combo, gpointer data,
				 GCompareFunc func)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gpointer menu_data;
	gint n;

	g_return_val_if_fail(GTK_IS_COMBO_BOX(combo), -1);

	model = gtk_combo_box_get_model(combo);
	for (valid = gtk_tree_model_get_iter_first(model, &iter), n = 0;
	     valid; valid = gtk_tree_model_iter_next(model, &iter), n++) {
		gtk_tree_model_get(model, &iter, MENU_COMBO_COL_DATA,
				   &menu_data, -1);
		if (func) {
			if (func(menu_data, data) == 0)
				return n;
		} else if (menu_data == data)
			return n;
	}

	return -1;
}

gint menu_get_option_menu_active_index(GtkComboBox *combo)
{
	return GPOINTER_TO_INT(menu_combo_get_active_data(combo));
}
