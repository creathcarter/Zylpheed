/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 2001 Match Grun
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

/*
 * Add address to address book dialog.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gtkutils.h"
#include "stock_pixmap.h"
#include "prefs_common.h"
#include "addressadd.h"
#include "addritem.h"
#include "addrbook.h"
#include "addrindex.h"
#include "editaddress.h"
#include "manage_window.h"
#include "utils.h"

typedef struct {
	AddressBookFile	*book;
	ItemFolder	*folder;
} FolderInfo;

static struct _AddressAdd_dlg {
	GtkWidget *window;
	GtkWidget *label_name;
	GtkWidget *label_address;
	GtkWidget *label_remarks;
	GtkWidget *tree_folder;
	GtkWidget *ok_btn;
	GtkWidget *cancel_btn;
	FolderInfo *fiSelected;
} addressadd_dlg;

enum {
	ADDRESSADD_COL_NAME,
	ADDRESSADD_COL_DATA,
	ADDRESSADD_N_COLS
};

static gboolean addressadd_cancelled;

static FolderInfo *addressadd_create_folderinfo( AddressBookFile *abf, ItemFolder *folder )
{
	FolderInfo *fi = g_new0( FolderInfo, 1 );
	fi->book   = abf;
	fi->folder = folder;
	return fi;
}

static void addressadd_free_folderinfo( FolderInfo *fi ) {
	fi->book   = NULL;
	fi->folder = NULL;
	g_free( fi );
}

static gint addressadd_delete_event( GtkWidget *widget, GdkEventAny *event, gboolean *cancelled ) {
	addressadd_cancelled = TRUE;
	gtk_main_quit();
	return TRUE;
}

static gboolean addressadd_key_pressed( GtkWidget *widget, GdkEventKey *event, gboolean *cancelled ) {
	if (event && event->keyval == GDK_KEY_Escape) {
		addressadd_cancelled = TRUE;
		gtk_main_quit();
	}
	return FALSE;
}

static void addressadd_ok( GtkWidget *widget, gboolean *cancelled ) {
	addressadd_cancelled = FALSE;
	gtk_main_quit();
}

static void addressadd_cancel( GtkWidget *widget, gboolean *cancelled ) {
	addressadd_cancelled = TRUE;
	gtk_main_quit();
}

static void addressadd_folder_select( GtkTreeSelection *selection, gpointer data )
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if( ! gtk_tree_selection_get_selected( selection, &model, &iter ) ) return;
	gtk_tree_model_get( model, &iter, ADDRESSADD_COL_DATA,
			    &addressadd_dlg.fiSelected, -1 );
}

/* GtkCTree auto sort: strcmp() on the node text, ascending */
static gint addressadd_folder_compare( GtkTreeModel *model, GtkTreeIter *a,
				       GtkTreeIter *b, gpointer data )
{
	gchar *name_a = NULL, *name_b = NULL;
	gint ret;

	gtk_tree_model_get( model, a, ADDRESSADD_COL_NAME, &name_a, -1 );
	gtk_tree_model_get( model, b, ADDRESSADD_COL_NAME, &name_b, -1 );
	if( ! name_a || ! name_b )
		ret = ( name_a != NULL ) - ( name_b != NULL );
	else
		ret = strcmp( name_a, name_b );
	g_free( name_b );
	g_free( name_a );

	return ret;
}

static gboolean addressadd_free_folderinfo_func( GtkTreeModel *model,
						 GtkTreePath *path,
						 GtkTreeIter *iter,
						 gpointer data )
{
	FolderInfo *fi = NULL;

	gtk_tree_model_get( model, iter, ADDRESSADD_COL_DATA, &fi, -1 );
	if( fi )
		addressadd_free_folderinfo( fi );

	return FALSE;
}

/* Clear the tree and free the row data (the GtkCTree destroy notify) */
static void addressadd_tree_clear( void )
{
	GtkTreeModel *model;

	model = gtk_tree_view_get_model( GTK_TREE_VIEW(addressadd_dlg.tree_folder) );
	gtk_tree_model_foreach( model, addressadd_free_folderinfo_func, NULL );
	gtk_tree_store_clear( GTK_TREE_STORE(model) );
}

static gboolean addressadd_tree_button( GtkWidget *widget, GdkEventButton *event, gpointer data ) {
	if( ! event ) return FALSE;
	if( event->button == 1 ) {
		/* Handle double click */
		if( event->type == GDK_2BUTTON_PRESS ) {
			addressadd_cancelled = FALSE;
			gtk_main_quit();
		}
	}

	return FALSE;
}

static void addressadd_create( void ) {
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *label_name;
	GtkWidget *label_addr;
	GtkWidget *label_rems;
	GtkWidget *tree_folder;
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkWidget *vlbox;
	GtkWidget *tree_win;
	GtkWidget *hbbox;
	GtkWidget *hsep;
	GtkWidget *ok_btn;
	GtkWidget *cancel_btn;
	gint top;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(window,
				    300 * gtkut_get_dpi_multiplier(),
				    360 * gtkut_get_dpi_multiplier());
	gtk_container_set_border_width(GTK_CONTAINER(window), 0);
	gtk_window_set_title(GTK_WINDOW(window), _("Add Address to Book"));
	gtk_window_set_position(GTK_WINDOW(window),
				GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_widget_realize(window);
	g_signal_connect(G_OBJECT(window), "delete_event",
			 G_CALLBACK(addressadd_delete_event), NULL);
	g_signal_connect(G_OBJECT(window), "key_press_event",
			 G_CALLBACK(addressadd_key_pressed), NULL);

	vbox = gtk_vbox_new(FALSE, 4);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_container_set_border_width( GTK_CONTAINER(vbox), 0 );

	table = gtk_table_new(3, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
	gtk_container_set_border_width( GTK_CONTAINER(table), 8 );
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 4);

	/* First row */
	top = 0;
	label = gtk_label_new(_("Name"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, top, (top + 1), GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	label_name = gtk_label_new("");
	gtk_table_attach(GTK_TABLE(table), label_name, 1, 2, top, (top + 1), GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_name), 0, 0.5);

	/* Second row */
	top = 1;
	label = gtk_label_new(_("Address"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, top, (top + 1), GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);

	label_addr = gtk_label_new("");
	gtk_table_attach(GTK_TABLE(table), label_addr, 1, 2, top, (top + 1), GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_addr), 0, 0.5);

	/* Third row */
	top = 2;
	label = gtk_label_new(_("Remarks"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, top, (top + 1), GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	label_rems = gtk_label_new("");
	gtk_table_attach(GTK_TABLE(table), label_rems, 1, 2, top, (top + 1), GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_rems), 0, 0.5);

	/* Address book/folder tree */
	vlbox = gtk_vbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), vlbox, TRUE, TRUE, 0);
	gtk_container_set_border_width( GTK_CONTAINER(vlbox), 4 );

	tree_win = gtk_scrolled_window_new( NULL, NULL );
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(tree_win),
				        GTK_POLICY_AUTOMATIC,
				        GTK_POLICY_AUTOMATIC );
	gtk_box_pack_start( GTK_BOX(vlbox), tree_win, TRUE, TRUE, 0 );

	store = gtk_tree_store_new( ADDRESSADD_N_COLS, G_TYPE_STRING, G_TYPE_POINTER );
	gtk_tree_sortable_set_sort_func( GTK_TREE_SORTABLE(store), ADDRESSADD_COL_NAME,
					 addressadd_folder_compare, NULL, NULL );
	gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE(store), ADDRESSADD_COL_NAME,
					      GTK_SORT_ASCENDING );
	tree_folder = gtk_tree_view_new_with_model( GTK_TREE_MODEL(store) );
	g_object_unref( G_OBJECT(store) );
	gtk_container_add( GTK_CONTAINER(tree_win), tree_folder );
	column = gtk_tree_view_column_new_with_attributes(
		_( "Select Address Book Folder" ), gtk_cell_renderer_text_new(),
		"text", ADDRESSADD_COL_NAME, NULL );
	gtk_tree_view_append_column( GTK_TREE_VIEW(tree_folder), column );
	gtk_tree_view_set_expander_column( GTK_TREE_VIEW(tree_folder), column );
	gtk_tree_selection_set_mode(
		gtk_tree_view_get_selection( GTK_TREE_VIEW(tree_folder) ),
		GTK_SELECTION_BROWSE );

	/* Button panel */
	gtkut_stock_button_set_create(&hbbox, &ok_btn, GTK_STOCK_OK,
				      &cancel_btn, GTK_STOCK_CANCEL,
				      NULL, NULL);
	gtk_box_pack_end(GTK_BOX(vbox), hbbox, FALSE, FALSE, 0);
	gtk_container_set_border_width( GTK_CONTAINER(hbbox), 4 );
	gtk_widget_grab_default(ok_btn);

	hsep = gtk_hseparator_new();
	gtk_box_pack_end(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(ok_btn), "clicked",
			 G_CALLBACK(addressadd_ok), NULL);
	g_signal_connect(G_OBJECT(cancel_btn), "clicked",
			 G_CALLBACK(addressadd_cancel), NULL);
	g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_folder))),
			 "changed", G_CALLBACK(addressadd_folder_select), NULL);
	g_signal_connect(G_OBJECT(tree_folder), "button_press_event",
			 G_CALLBACK(addressadd_tree_button), NULL);

	addressadd_dlg.window        = window;
	addressadd_dlg.label_name    = label_name;
	addressadd_dlg.label_address = label_addr;
	addressadd_dlg.label_remarks = label_rems;
	addressadd_dlg.tree_folder   = tree_folder;
	addressadd_dlg.ok_btn        = ok_btn;
	addressadd_dlg.cancel_btn    = cancel_btn;

	gtk_widget_show_all(vbox);
}

static void addressadd_load_folder( GtkTreeIter *parentNode, ItemFolder *parentFolder,
					FolderInfo *fiParent )
{
	GtkTreeStore *store = GTK_TREE_STORE( gtk_tree_view_get_model(
		GTK_TREE_VIEW(addressadd_dlg.tree_folder) ) );
	GList *list;
	ItemFolder *folder;
	GtkTreeIter node;
	FolderInfo *fi;

	list = parentFolder->listFolder;
	while( list ) {
		folder = list->data;
		fi = addressadd_create_folderinfo( fiParent->book, folder );
		gtk_tree_store_append( store, &node, parentNode );
		gtk_tree_store_set( store, &node,
				    ADDRESSADD_COL_NAME, ADDRITEM_NAME(folder),
				    ADDRESSADD_COL_DATA, fi, -1 );
		addressadd_load_folder( &node, folder, fi );
		list = g_list_next( list );
	}
}

static void addressadd_load_data( AddressIndex *addrIndex ) {
	AddressDataSource *ds;
	GList *list, *nodeDS;
	gchar *dsName;
	ItemFolder *rootFolder;
	AddressBookFile *abf;
	FolderInfo *fi;
	GtkTreeStore *store = GTK_TREE_STORE( gtk_tree_view_get_model(
		GTK_TREE_VIEW(addressadd_dlg.tree_folder) ) );
	GtkTreeIter node;

	addressadd_tree_clear();
	list = addrindex_get_interface_list( addrIndex );
	while( list ) {
		AddressInterface *iface = list->data;
		if( iface->type == ADDR_IF_BOOK ) {
			nodeDS = iface->listSource;
			while( nodeDS ) {
				ds = nodeDS->data;
				if ( !strcmp( addrindex_ds_get_name( ds ), ADDR_DS_AUTOREG ) )
					dsName = g_strdup( _("Auto-registered address") );
				else
					dsName = g_strdup( addrindex_ds_get_name( ds ) );

				/* Read address book */
				if( ! addrindex_ds_get_read_flag( ds ) ) {
					addrindex_ds_read_data( ds );
				}

				/* Add node for address book */
				abf = ds->rawDataSource;
				fi = addressadd_create_folderinfo( abf, NULL );
				gtk_tree_store_append( store, &node, NULL );
				gtk_tree_store_set( store, &node,
						    ADDRESSADD_COL_NAME, dsName,
						    ADDRESSADD_COL_DATA, fi, -1 );
				g_free( dsName );

				rootFolder = addrindex_ds_get_root_folder( ds );
				addressadd_load_folder( &node, rootFolder, fi );

				nodeDS = g_list_next( nodeDS );
			}
		}
		list = g_list_next( list );
	}

	/* GtkCTree nodes were inserted expanded */
	gtk_tree_view_expand_all( GTK_TREE_VIEW(addressadd_dlg.tree_folder) );
}

gboolean addressadd_selection( AddressIndex *addrIndex, const gchar *name, const gchar *address, const gchar *remarks ) {
	gboolean retVal = FALSE;
	ItemPerson *person = NULL;

	addressadd_cancelled = FALSE;
	if( ! addressadd_dlg.window ) addressadd_create();
	gtk_widget_grab_focus(addressadd_dlg.ok_btn);
	manage_window_set_transient(GTK_WINDOW(addressadd_dlg.window));
	gtk_widget_show(addressadd_dlg.window);

	addressadd_dlg.fiSelected = NULL;
	addressadd_load_data( addrIndex );
	gtkut_list_view_select_row( GTK_TREE_VIEW( addressadd_dlg.tree_folder ), 0 );
	gtk_widget_show(addressadd_dlg.window);

	gtk_label_set_text( GTK_LABEL(addressadd_dlg.label_name ), "" );
	gtk_label_set_text( GTK_LABEL(addressadd_dlg.label_address ), "" );
	gtk_label_set_text( GTK_LABEL(addressadd_dlg.label_remarks ), "" );
	if( name )
		gtk_label_set_text( GTK_LABEL(addressadd_dlg.label_name ), name );
	if( address )
		gtk_label_set_text( GTK_LABEL(addressadd_dlg.label_address ), address );
	if( remarks )
		gtk_label_set_text( GTK_LABEL(addressadd_dlg.label_remarks ), remarks );

	if( ! name )
		name = "";

	gtk_main();
	gtk_widget_hide( addressadd_dlg.window );

	if( ! addressadd_cancelled ) {
		if( addressadd_dlg.fiSelected ) {
			FolderInfo *fi = addressadd_dlg.fiSelected;
			person = addrbook_add_contact( fi->book, fi->folder, name, address, remarks );
			if( person ) {
				if( addressbook_edit_person( fi->book, NULL, person, FALSE ) == NULL )
					addrbook_remove_person( fi->book, person );
				else
					retVal = TRUE;
			}
		}
	}

	addressadd_tree_clear();

	return retVal;
}

gboolean addressadd_autoreg(AddressIndex *addrIndex, const gchar *name,
			    const gchar *address, const gchar *remarks)
{
	ItemPerson *person = NULL;
	AddressInterface *iface = NULL;
	AddressDataSource *ds = NULL;
	AddressBookFile *abf = NULL;
	GList *node_ds;
	const gchar *ds_name;

	g_return_val_if_fail(address != NULL, FALSE);

	if (!name)
		name = "";

	iface = addrindex_get_interface(addrIndex, ADDR_IF_BOOK);
	if (!iface)
		return FALSE;

	for (node_ds = iface->listSource; node_ds != NULL;
	     node_ds = node_ds->next) {
		ds = node_ds->data;
		ds_name = addrindex_ds_get_name(ds);
		if (!ds_name)
			continue;
		if (strcmp(ds_name, ADDR_DS_AUTOREG) != 0)
			continue;
		debug_print("addressadd_autoreg: AddressDataSource: %s found\n", ds_name);

		if (!addrindex_ds_get_read_flag(ds))
			addrindex_ds_read_data(ds);
		abf = ds->rawDataSource;
	}

	if (!abf)
		return FALSE;

	person = addrbook_add_contact(abf, NULL, name, address, remarks);
	if (person) {
		debug_print("addressadd_autoreg: person added: %s <%s>\n",
			    name, address);
		return TRUE;
	}

	return FALSE;
}

/*
* End of Source.
*/

