/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 2001-2010 Hiroyuki Yamamoto & The Sylpheed Claws Team
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

/* (alfons) - based on a contribution by Satoshi Nagayasu; revised for colorful 
 * menu and more Sylpheed integration. The idea to put the code in a separate
 * file is just that it make it easier to allow "user changeable" label colors.
 */

#include "defs.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "colorlabel.h"
#include "gtkutils.h"
#include "utils.h"
#include "prefs.h"

static gchar *labels[] = {
	N_("Orange"),
	N_("Red") ,
	N_("Pink"),
	N_("Sky blue"),
	N_("Blue"),
	N_("Green"),
	N_("Brown")
};

typedef enum LabelColorChangeFlags_ {
	LCCF_COLOR = 1 << 0,
	LCCF_LABEL = 1 << 1,
	LCCF_ALL   = LCCF_COLOR | LCCF_LABEL
} LabelColorChangeFlags;

/* XXX: if you add colors, make sure you also check the procmsg.h.
 * color indices are stored as 3 bits; that explains the max. of 7 colors */
static struct 
{
	LabelColorChangeFlags	changed; 
	GdkColor		color;

	/* XXX: note that the label member is supposed to be dynamically 
	 * allocated and fffreed */
	gchar			*label;
	GtkWidget		*widget;
	GtkWidget		*label_widget;
} label_colors[] = {
	{ LCCF_ALL, { 0, 0xffff, (0x99 << 8), 0x0 },		NULL, NULL, NULL },
	{ LCCF_ALL, { 0, 0xffff, 0, 0 },			NULL, NULL, NULL },
	{ LCCF_ALL, { 0, 0xffff, (0x66 << 8), 0xffff },		NULL, NULL, NULL },
	{ LCCF_ALL, { 0, 0x0, (0xcc << 8), 0xffff },		NULL, NULL, NULL },
	{ LCCF_ALL, { 0, 0x0, 0x0, 0xffff },			NULL, NULL, NULL },
	{ LCCF_ALL, { 0, 0x0, 0x99 << 8, 0x0 },			NULL, NULL, NULL },
	{ LCCF_ALL, { 0, 0x66 << 8, 0x33 << 8, 0x33 << 8 },	NULL, NULL, NULL }
};

#define LABEL_COLOR_WIDTH	28
#define LABEL_COLOR_HEIGHT	16

#define LABEL_COLORS_ELEMS (sizeof label_colors / sizeof label_colors[0])

#define G_RETURN_VAL_IF_INVALID_COLOR(color, val) \
	g_return_val_if_fail((color) >= 0 && (color) < LABEL_COLORS_ELEMS, (val))

static void colorlabel_recreate        (gint);
static void colorlabel_recreate_label  (gint);

gint colorlabel_get_color_count(void)
{
	return LABEL_COLORS_ELEMS;
}

GdkColor colorlabel_get_color(gint color_index)
{
	GdkColor invalid = { 0 };

	G_RETURN_VAL_IF_INVALID_COLOR(color_index, invalid);

	return label_colors[color_index].color;
}

const gchar *colorlabel_get_color_text(gint color_index)
{
	G_RETURN_VAL_IF_INVALID_COLOR(color_index, NULL);

	return label_colors[color_index].label ?
		label_colors[color_index].label : gettext(labels[color_index]);
}

const gchar *colorlabel_get_custom_color_text(gint color_index)
{
	G_RETURN_VAL_IF_INVALID_COLOR(color_index, NULL);

	return label_colors[color_index].label;
}

void colorlabel_set_color_text(gint color_index, const gchar *label)
{
	if (label_colors[color_index].label)
		g_free(label_colors[color_index].label);

	label_colors[color_index].label = g_strdup(label);
	label_colors[color_index].changed |= LCCF_LABEL;
}

static gboolean colorlabel_drawing_area_draw_cb
	(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	gulong c = (gulong) GPOINTER_TO_INT(data);
	GtkAllocation allocation;

	gtk_widget_get_allocation(widget, &allocation);

	/* filled color box ... */
	cairo_set_source_rgb(cr, ((c >> 16UL) & 0xFFUL) / 255.0,
			     ((c >> 8UL) & 0xFFUL) / 255.0,
			     (c & 0xFFUL) / 255.0);
	cairo_rectangle(cr, 0, 0, allocation.width - 1,
			allocation.height - 1);
	cairo_fill(cr);

	/* ... with a black frame (the style's black is #000000) */
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr, 1.0);
	cairo_rectangle(cr, 0.5, 0.5, allocation.width - 1,
			allocation.height - 1);
	cairo_stroke(cr);

	return FALSE;
}

GtkWidget *colorlabel_create_color_widget(GdkColor color)
{
	GtkWidget *widget;

	widget = gtk_drawing_area_new();
	gtk_widget_set_size_request(widget,
				    LABEL_COLOR_WIDTH - 2, LABEL_COLOR_HEIGHT - 4);

#define CL(x)		(((gulong) (x) >> (gulong) 8) & 0xFFUL)	
#define CR(r, g, b)	((CL(r) << (gulong) 16) | \
			 (CL(g) << (gulong)  8) | \
			 (CL(b)))

	g_signal_connect(G_OBJECT(widget), "draw",
			 G_CALLBACK(colorlabel_drawing_area_draw_cb),
			 GINT_TO_POINTER
				((gint)CR(color.red, color.green, color.blue)));

	return widget;
}

/* XXX: this function to check if menus with colors and labels should
 * be recreated */
gboolean colorlabel_changed(void)
{
	gint n;

	for (n = 0; n < LABEL_COLORS_ELEMS; n++) {
		if (label_colors[n].changed) 
			return TRUE;
	}

	return FALSE;
}

/* XXX: colorlabel_recreate_XXX are there to make sure everything
 * is initialized ok, without having to call a global _xxx_init_
 * function */
static void colorlabel_recreate_color(gint color)
{
	GtkWidget *widget;

	if (!(label_colors[color].changed & LCCF_COLOR))
		return;

	widget = colorlabel_create_color_widget(label_colors[color].color);
	g_return_if_fail(widget);

	if (label_colors[color].widget) 
		gtk_widget_destroy(label_colors[color].widget);

	label_colors[color].widget = widget;		
	label_colors[color].changed &= ~LCCF_COLOR;
}

static void colorlabel_recreate_label(gint color)
{
	const gchar *text;

	if (!(label_colors[color].changed & LCCF_LABEL))
		return;

	text = colorlabel_get_color_text(color);

	if (label_colors[color].label_widget)
		gtk_label_set_text(GTK_LABEL(label_colors[color].label_widget),
				   text);
	else
		label_colors[color].label_widget = gtk_label_new(text);

	label_colors[color].changed &= ~LCCF_LABEL;
}

/* XXX: call this function everytime when you're doing important
 * stuff with the label_colors[] array */
static void colorlabel_recreate(gint color)
{
	colorlabel_recreate_label(color);
	colorlabel_recreate_color(color);
}

static void colorlabel_recreate_all(void)
{
	gint n;

	for (n = 0; n < LABEL_COLORS_ELEMS; n++) 
		colorlabel_recreate(n);
}

/* colorlabel_create_check_color_menu_item() - creates a color
 * menu item with a check box */
GtkWidget *colorlabel_create_check_color_menu_item(gint color_index)
{
	GtkWidget *hbox; 
	GtkWidget *vbox; 
	GtkWidget *item;

	G_RETURN_VAL_IF_INVALID_COLOR(color_index, NULL);

	item = gtk_check_menu_item_new();

	colorlabel_recreate(color_index);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(item), hbox);

	vbox = gtk_vbox_new(TRUE, 0);
	gtk_widget_show(vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 1);

	gtk_container_add(GTK_CONTAINER(vbox),
			  label_colors[color_index].widget);
	gtk_widget_show(label_colors[color_index].widget);

	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label_colors[color_index].label_widget, FALSE, FALSE, 4);
	gtk_widget_show(label_colors[color_index].label_widget);

	return item;
}

/* a color swatch with a black frame, the size of the color widget */
static GdkPixbuf *colorlabel_create_color_pixbuf(GdkColor color)
{
	GdkPixbuf *pixbuf;
	guchar *pixels, *p;
	gint width = LABEL_COLOR_WIDTH - 2;
	gint height = LABEL_COLOR_HEIGHT - 4;
	gint rowstride, n_channels;
	gint x, y;

	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
	gdk_pixbuf_fill(pixbuf, 0x000000ff);
	pixels = gdk_pixbuf_get_pixels(pixbuf);
	rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	n_channels = gdk_pixbuf_get_n_channels(pixbuf);

	for (y = 1; y < height - 1; y++) {
		for (x = 1; x < width - 1; x++) {
			p = pixels + y * rowstride + x * n_channels;
			p[0] = color.red >> 8;
			p[1] = color.green >> 8;
			p[2] = color.blue >> 8;
		}
	}

	return pixbuf;
}

enum {
	COLOR_COMBO_COL_PIXBUF,
	COLOR_COMBO_COL_TEXT,
	COLOR_COMBO_COL_COLOR,
	COLOR_COMBO_N_COLS
};

/* colorlabel_create_color_combo() - creates a combo box with the color
 * labels (replaces the color menu that was used in option menus) */
GtkWidget *colorlabel_create_color_combo(void)
{
	GtkListStore *store;
	GtkWidget *combo;
	GtkCellRenderer *renderer;
	GtkTreeIter iter;
	gint i;

	colorlabel_recreate_all();

	store = gtk_list_store_new(COLOR_COMBO_N_COLS, GDK_TYPE_PIXBUF,
				   G_TYPE_STRING, G_TYPE_UINT);

	for (i = 0; i < LABEL_COLORS_ELEMS; i++) {
		GdkPixbuf *pixbuf;

		pixbuf = colorlabel_create_color_pixbuf(label_colors[i].color);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				   COLOR_COMBO_COL_PIXBUF, pixbuf,
				   COLOR_COMBO_COL_TEXT,
				   colorlabel_get_color_text(i),
				   COLOR_COMBO_COL_COLOR, (guint)(i + 1),
				   -1);
		g_object_unref(pixbuf);
	}

	combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer,
				       "pixbuf", COLOR_COMBO_COL_PIXBUF, NULL);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "xpad", 4, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer,
				       "text", COLOR_COMBO_COL_TEXT, NULL);

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	return combo;
}

guint colorlabel_get_color_combo_active_item(GtkWidget *combo)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	guint color = 0;

	g_return_val_if_fail(GTK_IS_COMBO_BOX(combo), 0);

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter)) {
		model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
		gtk_tree_model_get(model, &iter, COLOR_COMBO_COL_COLOR, &color,
				   -1);
	}

	return color;
}

void colorlabel_update_menu(void)
{
	gint i;

	for (i = 0; i < LABEL_COLORS_ELEMS; i++) {
		if (label_colors[i].widget && label_colors[i].changed) {
			colorlabel_recreate(i);
		}
	}
}

gint colorlabel_read_config(void)
{
	gchar *path;
	FILE *fp;
	gint i;
	gchar buf[PREFSBUFSIZE];

	path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, "colorlabelrc",
			   NULL);
	if ((fp = g_fopen(path, "rb")) == NULL) {
		g_free(path);
		return -1;
	}

	for (i = 0; i < LABEL_COLORS_ELEMS; i++) {
		if (fgets(buf, sizeof(buf), fp) == NULL)
			break;
		g_strstrip(buf);
		if (buf[0] != '\0') {
			colorlabel_set_color_text(i, buf);
		}
	}

	fclose(fp);
	g_free(path);

	return 0;
}

gint colorlabel_write_config(void)
{
	gchar *path;
	PrefFile *pfile;
	gint i;
	gint ret;
	const gchar *text;

	path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, "colorlabelrc",
			   NULL);
	if ((pfile = prefs_file_open(path)) == NULL) {
		g_warning("failed to write colorlabelrc");
		g_free(path);
		return -1;
	}

	for (i = 0; i < LABEL_COLORS_ELEMS; i++) {
		text = colorlabel_get_custom_color_text(i);
		ret = 0;
		if (text)
			ret = fputs(text, pfile->fp);

		if (ret == EOF || fputc('\n', pfile->fp) == EOF) {
			FILE_OP_ERROR(path, "fputs || fputc");
			prefs_file_close_revert(pfile);
			g_free(path);
			return -1;
		}
	}

	if (prefs_file_close(pfile) < 0) {
		g_warning("failed to write colorlabelrc");
		g_free(path);
		return -1;
	}

	g_free(path);

	return 0;
}
