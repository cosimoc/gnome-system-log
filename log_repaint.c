
/*  ----------------------------------------------------------------------

    Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    ---------------------------------------------------------------------- */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "logview.h"
#include "logrtns.h"
#include "log_repaint.h"
#include "misc.h"

char *month[12] =
{N_("January"), N_("February"), N_("March"), N_("April"), N_("May"),
 N_("June"), N_("July"), N_("August"), N_("September"), N_("October"),
 N_("November"), N_("December")};

enum {
   DATE = 0,
   HOSTNAME,
   PROCESS,
   MESSAGE
};

/**
 * Recursively called until the row specified by orig is found.
 *
 * *count will be set to the  row number of the child
 * relative to the row that was initially passed in as tree_path.
 *
 * *count will be -1 if orig is not found as a child.
 * If NULL is passed in as orig, *count will
 * be a count of the all the children (visible + collapsed (invisible)).
 *
 * NOTE: the value for depth must be 0 when this recursive function
 * is initially called, or it may not function as expected.
 **/
static void 
iterate_thru_children(GtkTreeView  *tree_view,
                      GtkTreeModel *tree_model,
                      GtkTreePath  *tree_path,
                      GtkTreePath  *orig,
                      gint         *count,
                      gint         depth)
{
  GtkTreeIter iter;

  gtk_tree_model_get_iter (tree_model, &iter, tree_path);

  if (orig != NULL && !gtk_tree_path_compare (tree_path, orig)) 
    /* Found it! */
    return;

  if (orig != NULL && gtk_tree_path_compare (tree_path, orig) > 0)
    {
      /* Past it, so return -1 */
      *count = -1;
      return;
    }
  else if (gtk_tree_model_iter_has_child (tree_model, &iter))
    {
      gtk_tree_path_append_index (tree_path, 0);
      iterate_thru_children (tree_view, tree_model, tree_path,
                             orig, count, (depth + 1));
      return;
    }
  else if (gtk_tree_model_iter_next (tree_model, &iter)) 
    {
      (*count)++;
      tree_path = gtk_tree_model_get_path (tree_model, &iter);
      iterate_thru_children (tree_view, tree_model, tree_path,
                             orig, count, depth); 
      gtk_tree_path_free (tree_path);
      return;
		}
  else if (gtk_tree_path_up (tree_path))
    {
      GtkTreeIter temp_iter;
      gboolean exit_loop = FALSE;
      gint new_depth = depth - 1;

      (*count)++;

     /*
      * Make sure that we back up until we find a row
      * where gtk_tree_path_next does not return NULL.
      */
      while (!exit_loop)
        {
          if (gtk_tree_path_get_depth (tree_path) == 0)
              /* depth is now zero so */
            return;
          gtk_tree_path_next (tree_path);	

          /* Verify that the next row is a valid row! */
          exit_loop = gtk_tree_model_get_iter (tree_model, &temp_iter, tree_path);

          if (!exit_loop)
            {
              /* Keep going up until we find a row that has a valid next */
              if (gtk_tree_path_get_depth(tree_path) > 1)
                {
                  new_depth--;
                  gtk_tree_path_up (tree_path);
                }
              else
                {
                 /*
                  * If depth is 1 and gtk_tree_model_get_iter returns FALSE,
                  * then we are at the last row, so just return.
                  */ 
                  if (orig != NULL)
                    *count = -1;

                  return;

								}
        }

     /*
      * This guarantees that we will stop when we hit the end of the
      * children.
      */
      if (new_depth < 0)
        return;

      iterate_thru_children (tree_view, tree_model, tree_path,
                            orig, count, new_depth);
      return;
				}
		}

 /*
  * If it gets here, then the path wasn't found.  Situations
  * that would cause this would be if the path passed in is
  * invalid or contained within the last row, but not visible
  * because the last row is not expanded.  If NULL was passed
  * in then a row count is desired, so only set count to -1
  * if orig is not NULL.
  */
  if (orig != NULL)
    *count = -1;

  return;
}
	
void
row_toggled_cb (GtkTreeView *treeview, GtkTreeIter *iter,
                GtkTreePath *path, gpointer user_data)
{
	LogviewWindow *logview = user_data;
	GtkTreeModel *model;
	gchar *date;
	int day;

    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
    g_return_if_fail (logview->curlog);

	model = gtk_tree_view_get_model (treeview);
	gtk_tree_model_get (model, iter, DATE, &date, -1);
	day = atoi (g_strrstr (date, " "));
	
	logview->curlog->expand[day-1] = gtk_tree_view_row_expanded (treeview, path);

	g_free (date);
}

/* ----------------------------------------------------------------------
   NAME:        handle_selection_changed_cb
   DESCRIPTION: User clicked on main window
   ---------------------------------------------------------------------- */

void
handle_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
	int selected_first = -1, selected_last = -1;
	LogviewWindow *logview = data;
    GtkTreePath *selected_path, *path;
	GtkTreeModel *model;
    GtkTreeIter iter;
	GList *selected_paths, *i;
    gint *indices, j, day;
	
    g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
    g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
    g_return_if_fail (logview->curlog);

	selected_paths = gtk_tree_selection_get_selected_rows (selection, &model);

	if (selected_paths) {	
        for (i = selected_paths; i != NULL; i = g_list_next (i)) {		    
            selected_path = i->data;
            indices = gtk_tree_path_get_indices (selected_path);
            if (logview->curlog->has_date) {
                int row = 0;
                
                for (j = 0; j < indices[0]; j++) {
                    path = gtk_tree_path_new_from_indices (j, -1);
                    gtk_tree_model_get_iter (model, &iter, path);
                    row += gtk_tree_model_iter_n_children (model, &iter) - 1;
                }
                if (gtk_tree_path_get_depth (selected_path) > 1)
                    row += indices[1];

                if (selected_last == -1 || row > selected_last)
                    selected_last = row;
                if (selected_first == -1 || row < selected_first)
                    selected_first = row;
                
                if (logview->curlog->caldata) {
                    day = logview->curlog->lines[row]->date;
                    logview->curlog->caldata->first_pass = TRUE;
                    gtk_calendar_select_day (GTK_CALENDAR (logview->calendar), day);
                }

            } else {
                if (selected_last == -1 || indices[0] > selected_last)
                    selected_last = indices[0];
                if (selected_first == -1 || indices[0] < selected_first)
                    selected_first = indices[0];
            }
        }

        logview->curlog->current_line_no = selected_last;
		logview->curlog->current_path = gtk_tree_path_copy (selected_path);
		logview->curlog->selected_line_first = selected_first;
		logview->curlog->selected_line_last = selected_last;
		g_list_foreach (selected_paths, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (selected_paths);
	}
}

static void
logview_update_statusbar (LogviewWindow *window)
{
   struct tm *tdm;
   char status_text[255];
   char *utf8;
   char *buffer;
   char *statusbar_text;
   /* Translators: Date only format, %x should well do really */
   const char *time_fmt = _("%x"); /* an evil way to avoid warning */

   g_return_if_fail (LOGVIEW_IS_WINDOW (window));

   if (window->curlog == NULL) { 
       gtk_statusbar_pop (GTK_STATUSBAR (window->statusbar), 0);
       return;
   }

   if (window->curlog->curmark != NULL) {
	   tdm = &(window->curlog)->curmark->fulldate;

       if (strftime (status_text, sizeof (status_text), time_fmt, tdm) <= 0) {
   	       /* as a backup print in US style */
  	       utf8 = g_strdup_printf ("%02d/%02d/%02d", tdm->tm_mday,
                    tdm->tm_mon, tdm->tm_year % 100);
       } else
           utf8 = LocaleToUTF8 (status_text);

       statusbar_text = g_strdup_printf (_("Last Modified: %s, %d lines"),
                                         utf8, window->curlog->total_lines);
       g_free (utf8);
   } else
       statusbar_text = g_strdup_printf (_("%d lines"), window->curlog->total_lines);

   if (statusbar_text) {
       gtk_statusbar_pop (GTK_STATUSBAR(window->statusbar), 0);
       gtk_statusbar_push (GTK_STATUSBAR(window->statusbar), 0, statusbar_text);
       g_free (statusbar_text);
   }
}

void
logview_update_version_bar (LogviewWindow *logview)
{
	Log *log;
	int i;
	gchar *label;

	log = logview->curlog;

	if (log->versions > 0 || log->parent_log != NULL) {
		Log *recent;

		gtk_widget_show (logview->version_bar);

		if (log->current_version > 0)
			recent = log->parent_log;
		else
			recent = log;
			
		for (i=5; i>-1; i--)
			gtk_combo_box_remove_text (GTK_COMBO_BOX (logview->version_selector), i);

		gtk_combo_box_append_text (GTK_COMBO_BOX (logview->version_selector), _("Current"));

		for (i=0; i<(recent->versions); i++) {
			label = g_strdup_printf ("Archive %d", i+1);
			gtk_combo_box_append_text (GTK_COMBO_BOX (logview->version_selector),
																 label);
			g_free (label);
		}

		gtk_combo_box_set_active (GTK_COMBO_BOX (logview->version_selector), 
															log->current_version);

	} else {
		gtk_widget_hide (logview->version_bar);
	}
}

/* ----------------------------------------------------------------------
   NAME:        GetDateHeader
   DESCRIPTION: Gets the Date to display 
   ---------------------------------------------------------------------- */

char *
GetDateHeader (LogLine *line)
{
   char buf[1024];
   char *utf8;

   if (line->month >= 0 && line->month < 12) {
       struct tm tm = {0};

       tm.tm_mday = line->date;
       tm.tm_year = 2000 /* bogus */;
       tm.tm_mon = line->month;
       /* Translators: Make sure this is only Month and Day format, year
        * will be bogus here */
       if (strftime (buf, sizeof (buf), _("%B %e"), &tm) <= 0) {
           /* If we fail just use the US format */
           utf8 = g_strdup_printf ("%s %d", _(month[(int) line->month]), 
                                   line->date);
       } else {
           utf8 = LocaleToUTF8 (buf);
       }
   } else {
       utf8 = g_strdup_printf ("?%d? %d", (int) line->month, line->date);
   }

   return utf8;

}

static void
tree_view_columns_set_visible (GtkWidget *view, gboolean visible)
{
	int i;
	GtkTreeViewColumn *column;
	for (i=0; i<3; i++) {
		column = gtk_tree_view_get_column (GTK_TREE_VIEW(view), i);
		gtk_tree_view_column_set_visible (column, visible);
	}
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), visible);
}

static void
model_fill_date_iter (GtkTreeStore *model, GtkTreeIter *iter, LogLine *line)
{
    gchar *utf8;
    
    utf8 = GetDateHeader (line);
    gtk_tree_store_set (model, iter, DATE, utf8, -1);
    g_free (utf8);
}

static void
model_fill_iter (GtkTreeStore *model, GtkTreeIter *iter, LogLine *line)
{
    struct tm date = {0};
    char *date_utf8, *hostname_utf8 = NULL, *process_utf8 = NULL, *message_utf8;
    char tmp[4096];
    
    if (line->hour >= 0 && line->min >= 0 && line->sec >= 0) {
        date.tm_mon = line->month;
        date.tm_year = 70 /* bogus */;
        date.tm_mday = line->date;
        date.tm_hour = line->hour;
        date.tm_min = line->min;
        date.tm_sec = line->sec;
        date.tm_isdst = 0;
        
        /* Translators: should be only the time, date could be bogus */
        if (strftime (tmp, sizeof (tmp), _("%X"), &date) <= 0) {
            /* as a backup print in 24 hours style */
            date_utf8 = g_strdup_printf ("%02d:%02d:%02d", line->hour, 
                                         line->min, line->sec);
        } else {
            date_utf8 = LocaleToUTF8 (tmp);
        }
    } else {
        date_utf8 = g_strdup (" ");
    }			 
    
    hostname_utf8 = LocaleToUTF8 (line->hostname);
    process_utf8 = LocaleToUTF8 (line->process);
    message_utf8 = LocaleToUTF8 (line->message);
    gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                        DATE, date_utf8, HOSTNAME, hostname_utf8,
                        PROCESS, process_utf8,
                        MESSAGE, message_utf8, -1);
    g_free (date_utf8);
    g_free (message_utf8);
    g_free (hostname_utf8);
    g_free (process_utf8);
}

static void
logview_add_new_log_lines (LogviewWindow *window, Log *log)
{
    GtkTreeIter iter, child_iter;
    GtkTreePath *path;
    int i, last_day_row = -1;
    LogLine *line;

    g_return_if_fail (LOGVIEW_IS_WINDOW (window));
    g_return_if_fail (log);
    
    /* Find the last expandable row (== day row) */
    for (i=0; i<32; i++)
        if (log->expand_paths[i] != NULL)
            last_day_row = i;
    
    if (last_day_row == -1)
        return;
    
    path = log->expand_paths[last_day_row];
    gtk_tree_model_get_iter (log->model, &iter, log->expand_paths[last_day_row]);
                
    for (i=log->displayed_lines; i<log->total_lines; i++) {
        line = (log->lines)[i];
        gtk_tree_store_append (GTK_TREE_STORE (log->model), &child_iter, &iter);
        model_fill_iter (GTK_TREE_STORE (log->model), &child_iter, line);
    }
    log->displayed_lines = log->total_lines;

    path = gtk_tree_model_get_path (GTK_TREE_MODEL (log->model), &child_iter);
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->view), path, NULL, TRUE, 1, 1);
}
    
static void
logview_show_model (LogviewWindow *window, Log *log)
{    
    int i;

    g_return_if_fail (LOGVIEW_IS_WINDOW (window));
    g_return_if_fail (log);

    gtk_tree_view_set_model (GTK_TREE_VIEW (window->view), log->model);
    if (log->has_date) {
        for (i = 0; i<32; ++i) {
            if (log->expand[i])
                gtk_tree_view_expand_row (GTK_TREE_VIEW (window->view),
                                          log->expand_paths[i], FALSE);
        }
    }
}

static void
log_scroll_and_focus_path (LogviewWindow *window, Log *log, GtkTreePath *path)
{
    g_return_if_fail (LOGVIEW_IS_WINDOW (window));
    g_return_if_fail (log);
    g_return_if_fail (path);

    gtk_tree_view_set_cursor (GTK_TREE_VIEW (window->view), path, NULL, FALSE); 
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->view), path, NULL, TRUE, 
                                  1, 1);
}

static void
logview_create_model_no_date (LogviewWindow *window, Log *log)
{
    int i;
    LogLine *line;
    GtkTreePath *last_path;
    GtkTreeIter iter;
    
    g_return_if_fail (LOGVIEW_IS_WINDOW (window));
    g_return_if_fail (log);
        
    log->model = GTK_TREE_MODEL(gtk_tree_store_new (4, G_TYPE_STRING, G_TYPE_STRING,
                                                    G_TYPE_STRING, G_TYPE_STRING));
    for (i=log->total_lines-1; i>0; i--) {
        line = (log->lines)[i];
        gtk_tree_store_prepend (GTK_TREE_STORE (log->model), &iter, NULL);
        gtk_tree_store_set (GTK_TREE_STORE (log->model), &iter,
                            MESSAGE, line->message, -1);
        if (i == log->total_lines-1)
            log->current_path = gtk_tree_model_get_path (log->model, &iter);
    }
}

void
logview_create_model (LogviewWindow *window, Log *log)
{
    GtkTreeIter iter, child_iter;
    GtkTreePath *path;
    LogLine *line;
    gint cm = -1, cd = -1;
    gint day_rows [32];
    int i, j;

    g_return_if_fail (LOGVIEW_IS_WINDOW (window));
    g_return_if_fail (log->total_lines > 0);

    log->model = GTK_TREE_MODEL(gtk_tree_store_new (4, G_TYPE_STRING, G_TYPE_STRING,
                                                    G_TYPE_STRING, G_TYPE_STRING));    
    while (gtk_events_pending ())
        gtk_main_iteration ();

    /* Parse the whole thing to find dates
       and create toplevel lines for them. */
    
    for (i = 0; i<32; i++) {
        day_rows[i] = -1;
        log->expand_paths[i] = NULL;
        log->expand[i] = FALSE;
    }
    
    for (i = 0; i < log->total_lines; i++) {
        line = (log->lines)[i];
        
        if ( (line->month != cm && line->month > 0) ||
             (line->date != cd && line->date > 0) ) {
            GtkTreePath *path;
            gchar *path_string;
            
            cm = line->month;
            cd = line->date;
            
            day_rows [cd-1] = i;
            
            gtk_tree_store_append (GTK_TREE_STORE (log->model), &iter, NULL);
            model_fill_date_iter (GTK_TREE_STORE (log->model), &iter, line);
            path = gtk_tree_model_get_path (log->model, &iter);
            log->expand_paths[cd-1] = gtk_tree_path_copy (path);
            path_string = gtk_tree_path_to_string (path);
            g_hash_table_insert (log->date_headers,
                                 DATEHASH (cm, cd), path_string);
            gtk_tree_path_free (path);
            path = NULL;
        }
    }
    log->expand [cd-1] = TRUE;
    day_rows [cd] = log->total_lines-1;
    
    /* now add the log lines. */
    
    for (i = 0; i < 32; i++) {
        if (log->expand_paths[i] != NULL) {
            
            gtk_tree_model_get_iter (log->model, &iter, log->expand_paths[i]);
            
            for (j = day_rows [i+1]; j >= day_rows [i]; j--) {                    
                line = (log->lines)[j];
                gtk_tree_store_prepend (GTK_TREE_STORE (log->model), &child_iter, &iter);
                model_fill_iter (GTK_TREE_STORE (log->model), &child_iter, line);
                if (j == day_rows[i+1])
                    log->current_path = gtk_tree_model_get_path (log->model, &child_iter);
            }                
        }
    }
        
    log->displayed_lines = log->total_lines;
}

void
log_repaint (LogviewWindow *window)
{
    Log *log;
    g_return_if_fail (LOGVIEW_IS_WINDOW (window));

    logview_update_statusbar (window);
    logview_set_window_title (window);
    
    if (window->curlog) {
        log = window->curlog;

        if ((log->displayed_lines > 0) &&
            (log->displayed_lines != log->total_lines))
            logview_add_new_log_lines (window, log);
        else {

            if (log->model == NULL) {
                if (log->has_date)
                    logview_create_model (window, log);
                else
                    logview_create_model_no_date (window, log);
            }
            
            logview_show_model (window, log);
            tree_view_columns_set_visible (window->view, log->has_date);
            log_scroll_and_focus_path (window, log, log->current_path);
        }
    } else
        gtk_tree_view_set_model (GTK_TREE_VIEW (window->view), NULL);    
}
