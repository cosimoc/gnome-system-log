/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2004 Vincent Noel <vnoel@cox.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "logview.h"
#include "logview-findbar.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

static
gboolean
logview_tree_model_search_iter_foreach (GtkTreeModel *model, GtkTreePath *path,
					GtkTreeIter *iter, gpointer data)
{
	SearchIter *st = data;
	gchar *found[3] = {NULL, NULL, NULL};
	int i;
	gchar *utf8;
	gchar *fields[3] = {NULL, NULL, NULL}, *hostname, *process, *message;
	GtkTreePath *search_path = gtk_tree_model_get_path (model, iter);

	if (st->forward) {
		if (gtk_tree_path_compare (st->current_path, search_path) > 0) {
			gtk_tree_path_free (search_path);
			return FALSE;
		}
	} else {
		if (gtk_tree_path_compare (st->current_path, search_path) == 0) {
			/* If we search backward and we have reached the current position, stop */
			gtk_tree_path_free (search_path);
			return TRUE;
		}
	}
	
	gtk_tree_model_get (model, iter, 1, &(fields[0]), 2, &(fields[1]), 3, &(fields[2]), -1);

	/* Can someone explain to me why the following code doesn't work ? */
	/*
	for (i==0; i<3; i++) {
		if (fields[i] != NULL) {
			utf8 = g_utf8_casefold (fields[i], -1);
			g_free (fields[i]);
			found[i] = g_strrstr (utf8, st->pattern);
			g_free (utf8);
		}
	}
	*/

	if (fields[0] != NULL) {
		utf8 = g_utf8_casefold (fields[0], -1);
		g_free (fields[0]);
		found[0] = g_strrstr (utf8, st->pattern);
		g_free (utf8);
	}
	if (fields[1] != NULL) {
		utf8 = g_utf8_casefold (fields[1], -1);
		g_free (fields[1]);
		found[1] = g_strrstr (utf8, st->pattern);
		g_free (utf8);
	}
	if (fields[2] != NULL) {
		utf8 = g_utf8_casefold (fields[2], -1);
		g_free (fields[2]);
		found[2] = g_strrstr (utf8, st->pattern);
		g_free (utf8);
	}

	if ((found[0] != NULL) || (found[1] != NULL) || (found[2] != NULL)) {
		if (st->forward) {
			if (gtk_tree_path_compare (st->current_path, search_path) <= st->comparison) {
				/* if searching forward, stop at the first found item */
				st->found_path = search_path;
				return TRUE;
			}
		} else {
			if (gtk_tree_path_compare (search_path, st->current_path) <= st->comparison) {
				/* if we search backward, continue until we get to the current point.
				 * we have to free the previously-found found-path, if we have one. */
				if (st->found_path)
					gtk_tree_path_free (st->found_path);
				st->found_path = search_path;
				return FALSE;
			}
		}
	}

	gtk_tree_path_free (search_path);
	return FALSE;
}

GtkTreePath *
logview_tree_model_find_match (GtkTreeModel *tree_model, const char *pattern,
			       GtkTreePath *current, gboolean forward, gboolean keep_current)
{
	GtkTreeIter iter_root;
	GtkTreePath *path;
	SearchIter *st;
	
	st = g_new0 (SearchIter, 1);
	st->pattern = pattern;
	st->found_path = NULL;
	st->current_path = current;
	/* if we keep the current line, use a 0 in the gtk_tree_path_compare */
	st->comparison = (keep_current ? 0 : -1);
	st->forward = forward;

	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (tree_model), &iter_root))
		return 0;

	gtk_tree_model_foreach (GTK_TREE_MODEL (tree_model),
				logview_tree_model_search_iter_foreach, st);

	path = st->found_path;
	g_free (st);
	return path;
 }

static gboolean
logview_findbar_find (LogviewWindow *window, char *pattern, gboolean forward, gboolean keep_current)
{
	/* if forward = FALSE, search backward ! */
	GtkTreeModel *model;
	GtkTreePath *path;

	if (strlen(pattern) < 3 || !window->curlog)
		return;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW(window->view));
	path = logview_tree_model_find_match (model, pattern,
					      window->curlog->current_path, forward, keep_current);
	if (path) {
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (window->view), path);
		gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->view)),
						path);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->view),
					      path, NULL, FALSE, 0, 0);
		gtk_tree_path_free (path);
		return TRUE;
	}
	return FALSE;
}

static void
logview_findbar_buttons_set_sensitive (LogviewWindow *window,
				   gboolean    sensitive)
{
    gtk_widget_set_sensitive (GTK_WIDGET (window->find_next), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET (window->find_prev), sensitive);
}

static void
logview_findbar_save_settings (LogviewWindow *window)
{
    const gchar    *tmp;

    tmp = gtk_entry_get_text (GTK_ENTRY (window->find_entry));

    g_free (window->find_string);
    window->find_string = g_utf8_casefold (tmp, -1);
}

static gboolean
logview_findbar_action (LogviewWindow *window, LogviewFindAction action, gboolean keep_current)
{
	logview_findbar_save_settings (window);
	return (logview_findbar_find (window, window->find_string, 
				      (action == YELP_WINDOW_FIND_NEXT) ? TRUE: FALSE, keep_current));
}

static void
logview_findbar_entry_activate_cb (GtkEditable *editable, gpointer data)
{
	LogviewWindow     *window = data;

	if (!window->curlog)
		return;

	if (!logview_findbar_action (window, YELP_WINDOW_FIND_NEXT, FALSE)) {
		gtk_widget_set_sensitive (GTK_WIDGET (window->find_next), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (window->find_prev), TRUE);
	} else {
		logview_findbar_buttons_set_sensitive (window, TRUE);
	}
}

static void
logview_findbar_entry_changed_cb (GtkEditable *editable,
				  gpointer     data)
{
	LogviewWindow     *window = data;

	if (!window->curlog)
		return;
	
	if (!logview_findbar_action (window, YELP_WINDOW_FIND_NEXT, TRUE)) {
		gtk_widget_set_sensitive (GTK_WIDGET (window->find_next), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (window->find_prev), TRUE);
	} else {
		logview_findbar_buttons_set_sensitive (window, TRUE);
	}
}

static void
logview_findbar_clicked_cb (GtkWidget  *widget,
			    LogviewWindow *window)
{
    g_return_if_fail (GTK_IS_TOOL_ITEM (widget));

    if (GTK_TOOL_ITEM (widget) == window->find_next) {
	    if (!logview_findbar_action (window, YELP_WINDOW_FIND_NEXT, FALSE)) {
		    gtk_widget_set_sensitive (GTK_WIDGET (window->find_next), FALSE);
		    gtk_widget_set_sensitive (GTK_WIDGET (window->find_prev), TRUE);
	} else {
	    logview_findbar_buttons_set_sensitive (window, TRUE);
	}
    }
    else if (GTK_TOOL_ITEM (widget) == window->find_prev) {
	    if (!logview_findbar_action (window, YELP_WINDOW_FIND_PREV, FALSE)) {
		    gtk_widget_set_sensitive (GTK_WIDGET (window->find_next), TRUE);
		    gtk_widget_set_sensitive (GTK_WIDGET (window->find_prev), FALSE);
	} else {
	    logview_findbar_buttons_set_sensitive (window, TRUE);
	}
    }
    else
	g_assert_not_reached ();
}

void
logview_findbar_populate (LogviewWindow *window, GtkWidget *find_bar)
{
	GtkWidget *label;
	GtkToolItem *item;
	
	g_return_if_fail (GTK_IS_TOOLBAR (find_bar));
	
	label = gtk_label_new_with_mnemonic (_("Fin_d : "));
	item = gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER (item), label);
	gtk_container_set_border_width (GTK_CONTAINER (item), 5);
	gtk_toolbar_insert (GTK_TOOLBAR (find_bar), item, -1);
	
	window->find_entry = gtk_entry_new ();
	g_signal_connect (G_OBJECT (window->find_entry), "changed",
			  G_CALLBACK (logview_findbar_entry_changed_cb), window);
	g_signal_connect (G_OBJECT (window->find_entry), "activate",
			  G_CALLBACK (logview_findbar_entry_activate_cb), window);
	item = gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER (item), window->find_entry);
	gtk_toolbar_insert (GTK_TOOLBAR (find_bar), item, -1);
	
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), window->find_entry);
	
	window->find_next = gtk_tool_button_new_from_stock (GTK_STOCK_GO_DOWN);
	gtk_tool_button_set_label (GTK_TOOL_BUTTON(window->find_next), _("Find Next"));
	gtk_tool_item_set_is_important (window->find_next, TRUE);
	g_signal_connect (window->find_next,
			  "clicked",
			  G_CALLBACK (logview_findbar_clicked_cb),
			  window);
	gtk_toolbar_insert (GTK_TOOLBAR (find_bar), window->find_next, -1);
	
	window->find_prev = gtk_tool_button_new_from_stock (GTK_STOCK_GO_UP);
	gtk_tool_item_set_is_important (window->find_prev, TRUE);
	gtk_tool_button_set_label (GTK_TOOL_BUTTON(window->find_prev), _("Find Previous"));
	g_signal_connect (window->find_prev,
			  "clicked",
			  G_CALLBACK (logview_findbar_clicked_cb),
			  window);
	gtk_toolbar_insert (GTK_TOOLBAR (find_bar), window->find_prev, -1);

	item = gtk_tool_item_new ();
	gtk_toolbar_insert (GTK_TOOLBAR (find_bar), item, -1);
	gtk_tool_item_set_expand (item, TRUE);
	
	item = gtk_tool_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_tool_item_set_is_important (item, FALSE);
	g_signal_connect_swapped (item,
				  "clicked",
				  G_CALLBACK (gtk_widget_hide),
				  window->find_bar);
	gtk_toolbar_insert (GTK_TOOLBAR (find_bar), item, -1);
}