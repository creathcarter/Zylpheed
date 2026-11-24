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

#ifndef __MENU_H__
#define __MENU_H__

#include <glib.h>
#include <gtk/gtk.h>

#define MENU_VAL_ID "Sylpheed::Menu::ValueID"

#define MENUITEM_ADD(menu, menuitem, label, data)			\
{									\
	if (label)							\
		menuitem = gtk_menu_item_new_with_label(label);		\
	else {								\
		menuitem = gtk_menu_item_new();				\
		gtk_widget_set_sensitive(menuitem, FALSE);		\
	}								\
	gtk_widget_show(menuitem);					\
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);		\
	if (data)							\
		g_object_set_data(G_OBJECT(menuitem),			\
				  MENU_VAL_ID,				\
				  GINT_TO_POINTER(data));		\
}

#define MENUITEM_ADD_WITH_MNEMONIC(menu, menuitem, label, data)		\
{									\
	if (label)							\
		menuitem = gtk_menu_item_new_with_mnemonic(label);	\
	else {								\
		menuitem = gtk_menu_item_new();				\
		gtk_widget_set_sensitive(menuitem, FALSE);		\
	}								\
	gtk_widget_show(menuitem);					\
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);		\
	if (data)							\
		g_object_set_data(G_OBJECT(menuitem),			\
				  MENU_VAL_ID,				\
				  GINT_TO_POINTER(data));		\
}

#define MENUITEM_ADD_FROM_STOCK(menu, menuitem, label, data)		\
{									\
	if (label)							\
		menuitem = gtk_image_menu_item_new_from_stock(label, NULL); \
	else {								\
		menuitem = gtk_menu_item_new();				\
		gtk_widget_set_sensitive(menuitem, FALSE);		\
	}								\
	gtk_widget_show(menuitem);					\
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);		\
	if (data)							\
		g_object_set_data(G_OBJECT(menuitem),			\
				  MENU_VAL_ID,				\
				  GINT_TO_POINTER(data));		\
}

#define MENUITEM_ADD_RADIO(menu, menuitem, widget, label, data)		\
{									\
	if (label) {							\
		if (widget)						\
			menuitem = gtk_radio_menu_item_new_with_mnemonic_from_widget \
				(GTK_RADIO_MENU_ITEM(widget), label);	\
		else							\
			menuitem = gtk_radio_menu_item_new_with_mnemonic \
				(NULL, label);				\
	} else {							\
		menuitem = gtk_menu_item_new();				\
		gtk_widget_set_sensitive(menuitem, FALSE);		\
	}								\
	gtk_widget_show(menuitem);					\
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);		\
	if (data)							\
		g_object_set_data(G_OBJECT(menuitem),			\
				  MENU_VAL_ID,				\
				  GINT_TO_POINTER(data));		\
}

#define menu_set_insensitive_all(menu_shell) \
	menu_set_sensitive_all(menu_shell, FALSE);

/*
 * Menu factory: a small replacement for the subset of GtkItemFactory
 * that Sylpheed uses (GtkItemFactory was removed in GTK+ 3).
 *
 * Entry tables, menu paths, accelerator paths ("<Main>/File/Exit") and
 * callback conventions are the same as with GtkItemFactory:
 *
 *   callback_type 1: callback(callback_data, callback_action, widget)
 *   callback_type 2: callback(widget, callback_data, callback_action)
 *
 * Supported item types: NULL / "" / "<Item>", "<Title>", "<CheckItem>",
 * "<ToggleItem>", "<RadioItem>", "<Separator>", "<Tearoff>", "<Branch>",
 * "<LastBranch>", and the path of a radio item to join its group.
 */
typedef struct _MenuFactory		MenuFactory;
typedef struct _MenuFactoryEntry	MenuFactoryEntry;

/* Unprototyped on purpose, like GtkItemFactoryCallback, so that the
   existing entry tables need no casts. */
typedef void (*MenuFactoryCallback)	();

struct _MenuFactoryEntry
{
	gchar *path;
	gchar *accelerator;
	MenuFactoryCallback callback;
	guint callback_action;
	gchar *item_type;
	gconstpointer extra_data;
};

MenuFactory *menu_factory_new		(GType			 container_type,
					 const gchar		*path,
					 GtkAccelGroup		*accel_group);
void menu_factory_create_item		(MenuFactory		*factory,
					 MenuFactoryEntry	*entry,
					 gpointer		 callback_data,
					 guint			 callback_type);
void menu_factory_create_items		(MenuFactory		*factory,
					 guint			 n_entries,
					 MenuFactoryEntry	*entries,
					 gpointer		 callback_data);
MenuFactory *menu_factory_from_widget	(GtkWidget		*widget);
GtkWidget *menu_factory_get_widget	(MenuFactory		*factory,
					 const gchar		*path);
GtkWidget *menu_factory_get_item	(MenuFactory		*factory,
					 const gchar		*path);
GtkAccelGroup *menu_factory_get_accel_group
					(MenuFactory		*factory);
void menu_factory_destroy		(MenuFactory		*factory);

GtkWidget *menubar_create	(GtkWidget		*window,
				 MenuFactoryEntry	*entries,
				 guint			 n_entries,
				 const gchar		*path,
				 gpointer		 data);
GtkWidget *menu_create_items	(MenuFactoryEntry	*entries,
				 guint			 n_entries,
				 const gchar		*path,
				 MenuFactory	       **factory,
				 gpointer		 data);

void menu_set_sensitive		(MenuFactory		*ifactory,
				 const gchar		*path,
				 gboolean		 sensitive);
void menu_set_sensitive_all	(GtkMenuShell		*menu_shell,
				 gboolean		 sensitive);

void menu_set_active		(MenuFactory		*ifactory,
				 const gchar		*path,
				 gboolean		 is_active);

void menu_button_position	(GtkMenu		*menu,
				 gint			*x,
				 gint			*y,
				 gboolean		*push_in,
				 gpointer		 user_data);
void menu_widget_position	(GtkMenu		*menu,
				 gint			*x,
				 gint			*y,
				 gboolean		*push_in,
				 gpointer		 user_data);

/* Option combo box (replaces GtkOptionMenu); see menu.c */
#define COMBOBOX_ADD(combo, label, data) \
	menu_combo_append(GTK_COMBO_BOX(combo), label, GINT_TO_POINTER(data))

GtkWidget *menu_combo_new	(void);
gint menu_combo_append		(GtkComboBox		*combo,
				 const gchar		*label,
				 gpointer		 data);
gint menu_combo_insert		(GtkComboBox		*combo,
				 gint			 position,
				 const gchar		*label,
				 gpointer		 data);
void menu_combo_remove		(GtkComboBox		*combo,
				 gint			 index);
void menu_combo_set_nth_extra	(GtkComboBox		*combo,
				 gint			 index,
				 gpointer		 extra);
gpointer menu_combo_get_nth_extra
				(GtkComboBox		*combo,
				 gint			 index);
void menu_combo_clear		(GtkComboBox		*combo);
gint menu_combo_get_n_items	(GtkComboBox		*combo);
gpointer menu_combo_get_nth_data(GtkComboBox		*combo,
				 gint			 index);
gpointer menu_combo_get_active_data
				(GtkComboBox		*combo);
gint menu_combo_set_active_data	(GtkComboBox		*combo,
				 gpointer		 data);
void menu_combo_set_item_sensitive
				(GtkComboBox		*combo,
				 gint			 index,
				 gboolean		 sensitive);
void menu_combo_set_sensitive_all
				(GtkComboBox		*combo,
				 gboolean		 sensitive);

gint menu_find_option_menu_index(GtkComboBox		*combo,
				 gpointer		 data,
				 GCompareFunc		 func);

gint menu_get_option_menu_active_index
				(GtkComboBox		*combo);

#endif /* __MENU_H__ */
