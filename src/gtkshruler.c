/* GtkSHRuler
 *
 * Copyright (C) 2000-2005 Alfons Hoogervorst & The Sylpheed Claws Team
 * Copyright (C) 2007 Hiroyuki Yamamoto
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
/* I derived this class from hruler. S in HRuler could be read as
 * Sylpheed (sylpheed.sraoss.jp), but also [S]ettable HRuler.
 * I basically ripped apart the draw_ticks member of HRuler; it
 * now draws the ticks at ruler->max_size. so gtk_ruler_set_range's
 * last parameter has the distance between two ticks (which is
 * the width of the fixed font character!
 *
 * -- Alfons
 */

/* Since the GTK+ 3 port preparation, GtkSHRuler no longer derives from
 * GtkHRuler (removed in GTK+ 3): it is a GtkDrawingArea that draws the
 * same frame, ticks and labels itself with cairo, and keeps the range
 * values that gtk_ruler_set_range() used to store.
 * With GTK+ 3 the frame comes from the theme (style class "frame") and
 * its border width takes the place of the GtkStyle x/ythickness. */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gtkshruler.h"

#define RULER_HEIGHT          14
#define MINIMUM_INCR          5
#define MAXIMUM_SUBDIVIDE     5
#define MAXIMUM_SCALES        10

#define ROUND(x) ((int) ((x) + 0.5))

static void gtk_shruler_class_init   	(GtkSHRulerClass *klass);
static void gtk_shruler_init         	(GtkSHRuler      *hruler);
static void gtk_shruler_get_preferred_width
					(GtkWidget	 *widget,
					 gint		 *minimum,
					 gint		 *natural);
static void gtk_shruler_get_preferred_height
					(GtkWidget	 *widget,
					 gint		 *minimum,
					 gint		 *natural);
static gboolean gtk_shruler_draw	(GtkWidget	 *widget,
					 cairo_t	 *cr);
static void gtk_shruler_draw_ticks 	(GtkSHRuler      *ruler,
					 cairo_t	 *cr);

G_DEFINE_TYPE(GtkSHRuler, gtk_shruler, GTK_TYPE_DRAWING_AREA)

static void
gtk_shruler_class_init(GtkSHRulerClass * klass)
{
 	GtkWidgetClass * widget_class;

  	widget_class = (GtkWidgetClass*) klass;

	/* motion notify events are neglected (no event mask is set) */
	widget_class->get_preferred_width = gtk_shruler_get_preferred_width;
	widget_class->get_preferred_height = gtk_shruler_get_preferred_height;
	widget_class->draw = gtk_shruler_draw;
}

static void
gtk_shruler_init (GtkSHRuler * shruler)
{
	shruler->start_pos = 0;
	shruler->lower = 0;
	shruler->upper = 0;
	shruler->position = 0;
	shruler->max_size = 0;

	gtk_style_context_add_class
		(gtk_widget_get_style_context(GTK_WIDGET(shruler)),
		 GTK_STYLE_CLASS_FRAME);
}

/* the frame width, formerly GtkStyle xthickness / ythickness */
static void
gtk_shruler_get_thickness(GtkWidget *widget, gint *xthickness,
			  gint *ythickness)
{
	GtkStyleContext *context;
	GtkBorder border;

	context = gtk_widget_get_style_context(widget);
	gtk_style_context_get_border(context,
				     gtk_style_context_get_state(context),
				     &border);
	*xthickness = border.left;
	*ythickness = border.top;
}

static void
gtk_shruler_get_preferred_width(GtkWidget *widget, gint *minimum,
				gint *natural)
{
	gint xthickness, ythickness;

	gtk_shruler_get_thickness(widget, &xthickness, &ythickness);
	*minimum = *natural = xthickness * 2 + 1;
}

static void
gtk_shruler_get_preferred_height(GtkWidget *widget, gint *minimum,
				 gint *natural)
{
	gint xthickness, ythickness;

	gtk_shruler_get_thickness(widget, &xthickness, &ythickness);
	*minimum = *natural = ythickness * 2 + RULER_HEIGHT;
}

GtkWidget*
gtk_shruler_new(void)
{
	return GTK_WIDGET( g_object_new( gtk_shruler_get_type(), NULL ) );
}

void
gtk_shruler_set_start_pos(GtkSHRuler *ruler, gint pos)
{
	g_return_if_fail (GTK_IS_SHRULER (ruler));

	ruler->start_pos = pos;
}

gint
gtk_shruler_get_start_pos(GtkSHRuler *ruler)
{
	g_return_val_if_fail (GTK_IS_SHRULER (ruler), 0);

	return ruler->start_pos;
}

/* replaces gtk_ruler_set_range(): max_size is the distance between two
   ticks (the character width) */
void
gtk_shruler_set_range(GtkSHRuler *ruler, gdouble lower, gdouble upper,
		      gdouble position, gdouble max_size)
{
	g_return_if_fail (GTK_IS_SHRULER (ruler));

	ruler->lower = lower;
	ruler->upper = upper;
	ruler->position = position;
	ruler->max_size = max_size;

	gtk_widget_queue_draw(GTK_WIDGET(ruler));
}

static gboolean
gtk_shruler_draw(GtkWidget *widget, cairo_t *cr)
{
	GtkSHRuler *ruler = GTK_SHRULER(widget);
	GtkStyleContext *context;
	gint width, height;

	context = gtk_widget_get_style_context(widget);
	width = gtk_widget_get_allocated_width(widget);
	height = gtk_widget_get_allocated_height(widget);

	gtk_render_background(context, cr, 0, 0, width, height);
	gtk_render_frame(context, cr, 0, 0, width, height);

	gtk_shruler_draw_ticks(ruler, cr);

	return FALSE;
}

static void
gtk_shruler_draw_ticks(GtkSHRuler *ruler, cairo_t *cr)
{
	GtkWidget *widget;
	GtkStyleContext *context;
	GdkRGBA color;
	GtkAllocation allocation;
	gint i;
	gint height;
	gint xthickness;
	gint ythickness;
	gint pos;
	gint step;

	g_return_if_fail (ruler != NULL);

	widget = GTK_WIDGET (ruler);
	context = gtk_widget_get_style_context(widget);
	gtk_widget_get_allocation(widget, &allocation);

	gtk_shruler_get_thickness(widget, &xthickness, &ythickness);

	height = allocation.height - ythickness * 2;

	/* GtkHRuler drew with the normal foreground color */
	gtk_style_context_get_color(context,
				    gtk_style_context_get_state(context),
				    &color);
	gdk_cairo_set_source_rgba(cr, &color);

	/* assume ruler->max_size has the char width */
	step = (gint)ruler->max_size;
	if (step < 1)
		return;

	/* i is increment of char_width,  pos is label number
	 * y position is based on height of widget itself */
	for ( pos = ruler->start_pos, i = 0; pos < allocation.width - xthickness; pos += step, i++ ) {
		gint length = height / 8;

		if ( i % 10 == 0 ) length = ( 2 * height / 3 );
		else if ( i % 5 == 0 ) length = ( height / 3 );

		/* the pixels of gdk_draw_line() from (pos, height +
		   ythickness) to (pos, height - length) */
		cairo_rectangle(cr, pos, height - length, 1,
				ythickness + length + 1);
		cairo_fill(cr);

		if ( i % 10 == 0 ) {
			gchar buf[8];
			PangoLayout *layout;

			/* draw label */
			g_snprintf(buf, sizeof buf, "%d", i);

			layout = gtk_widget_create_pango_layout
				(GTK_WIDGET(ruler), buf);

			cairo_move_to(cr, pos + 2, 0);
			pango_cairo_show_layout(cr, layout);

			g_object_unref(layout);
		}
	}
}

/* gtk_ruler_set_pos() - does not work yet, need to reimplement
 * gtk_ruler_draw_pos(). */
void
gtk_shruler_set_pos(GtkSHRuler * ruler, gfloat pos)
{
	g_return_if_fail( ruler != NULL );

	if ( pos < ruler->lower )
		pos = ruler->lower;
	if ( pos > ruler->upper )
		pos = ruler->upper;

	/* the position marker was never drawn (draw_pos was NULL) */
	ruler->position = pos;
}
