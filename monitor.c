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


#include <config.h>
#include <gnome.h>
#include <time.h>
#include "logview.h"
#include "logrtns.h"

#define MON_WINDOW_WIDTH           180
#define MON_WINDOW_HEIGHT          150

#define MON_MAX_NUM_LOGS         4   /* Max. num of logs to monitor       */
#define MON_MAX_NUM_LINES        100 /* Max. num of lines in window       */
#define MON_MODE_VISIBLE         1   /* Displays log as it is monitored   */
#define MON_MODE_INVISIBLE       2   /* Monitors log without output       */
#define MON_MODE_ICONIFIED       3   /* Monitors log showing only an icon */

/*
 *    -------------------
 *    Function Prototypes
 *    -------------------
 */

void MonitorMenu (GtkWidget * widget, gpointer user_data);
void close_monitor_options (GtkWidget * widget, gpointer client_data);
void go_monitor_log (GtkWidget * widget, gpointer client_data);
void mon_remove_log (GtkWidget *widget, GtkWidget *foo);
void mon_add_log (GtkWidget *widget, GtkWidget *foo);
void mon_read_last_page (Log *log);
void mon_read_new_lines (Log *log);
void mon_format_line (char *buffer, int bufsize, LogLine *line);
void mon_hide_app_checkbox (GtkWidget *widget, gpointer data);
void mon_actions_checkbox (GtkWidget *widget, gpointer data);
void mon_edit_actions (GtkWidget *widget, gpointer data);
gint mon_check_logs (gpointer);
int mon_repaint (GtkWidget * widget, GdkEventExpose * event);
void InitMonitorData (void);

/*
 *       ----------------
 *       Global variables
 *       ----------------
 */

extern GtkWidget *app;
extern ConfigData *cfg;
extern Log *curlog, *loglist[];
extern int numlogs, curlognum;

static GtkWidget *monoptions = NULL;
static GtkWidget *monwindow = NULL;
static GtkListStore *srclist, *destlist;
static GtkWidget *srclist_view, *destlist_view;

static int monitorcount = 0;
static gboolean mon_opts_visible = FALSE, mon_win_visible = FALSE;
static gboolean mon_exec_actions = FALSE, mon_hide_while_monitor = FALSE;
static gboolean main_app_hidden = FALSE;

/* ----------------------------------------------------------------------
   NAME:         MonitorMenu
   DESCRIPTION:  Opens up the monitor selection dialog to choose which
                 logs to monitor.
   ---------------------------------------------------------------------- */

void
MonitorMenu (GtkWidget * widget, gpointer user_data)
{
   GtkWidget *hbox, *hbox2;
   GtkWidget *label;
   GtkWidget *scrolled_win;
   GtkWidget *list_item;
   GtkWidget *button;       
   GtkWidget *vbox2;       
   GtkStyle *style;
   GtkBox *vbox;
   char buffer[10];
   int i;
   GtkCellRenderer *cell_renderer;
   GtkTreeViewColumn *column;
   GtkTreeIter newiter;

   if (curlog == NULL || mon_opts_visible)
      return;
     
   /* Clear loglist[i].mon_on flag */
   for (i=0; i<numlogs; i++)
     loglist[i]->mon_on = FALSE;

   if (monoptions == NULL)
     {
       /* monoptions = gtk_dialog_new ();   */
      monoptions = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_container_set_border_width (GTK_CONTAINER (monoptions), 5);
      gtk_window_set_title (GTK_WINDOW (monoptions), _("Monitor options"));
      gtk_widget_set_style (monoptions, cfg->main_style);
      gtk_signal_connect (GTK_OBJECT (monoptions), "destroy",
			  GTK_SIGNAL_FUNC (close_monitor_options),
			  NULL);

      vbox = (GtkBox *)gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
      gtk_container_add (GTK_CONTAINER (monoptions), GTK_WIDGET (vbox));
      gtk_widget_show (GTK_WIDGET (vbox)); 

      style = gtk_style_new ();
      memcpy (style, cfg->main_style, sizeof(GtkStyle));
      style->font_desc = pango_font_description_copy (cfg->heading);
      label = gtk_label_new (_("Choose logs to monitor"));
      gtk_misc_set_alignment(GTK_MISC (label), 0.0, 0.0);
      gtk_widget_set_style (label, style);
      gtk_box_pack_start (vbox, label, TRUE, TRUE, 0);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (vbox, hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      /* Source list */
      scrolled_win = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      gtk_widget_set_usize (scrolled_win, 160, 100);
      gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 3);
      gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
      gtk_widget_set_style (scrolled_win, cfg->main_style);
      gtk_widget_show (scrolled_win);

      srclist = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
      srclist_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (srclist));
      cell_renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes (NULL, cell_renderer,
                                                         "text", 0, NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (srclist_view), column);
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (srclist_view), FALSE);
      gtk_tree_selection_set_mode
                               ( (GtkTreeSelection *)gtk_tree_view_get_selection
                                (GTK_TREE_VIEW (srclist_view)),
                                 GTK_SELECTION_SINGLE);
      gtk_widget_set_style (srclist_view, cfg->main_style);
      gtk_container_add (GTK_CONTAINER (scrolled_win), srclist_view);
      gtk_widget_show (srclist_view);

      for (i = 0; i < numlogs; i++)
        {
          gtk_list_store_append (srclist, (GtkTreeIter *)&newiter);
          gtk_list_store_set (srclist, (GtkTreeIter *)&newiter, 0,
                              loglist[i]->name, 1, i, -1);
        }

      vbox2 = gtk_vbox_new (FALSE, 2);
      gtk_box_pack_start ( GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
      gtk_widget_show (vbox2);

      /* Arrowed buttons */
      button = gtk_button_new_with_label (_("Add >>"));
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          (GtkSignalFunc) mon_add_log,
                          NULL);

      /* Remove button */ 
      button = gtk_button_new_with_label (_("Remove <<"));
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          (GtkSignalFunc) mon_remove_log,
                          NULL);



      /* Destination list */
      scrolled_win = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      gtk_widget_set_usize( scrolled_win, 160, 100);
      gtk_widget_set_style( scrolled_win, cfg->main_style );
      gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
      gtk_widget_show (scrolled_win);

      destlist = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT); 
      destlist_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (destlist));
      cell_renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes (NULL, cell_renderer,
                                                         "text", 0, NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (destlist_view), column);
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (destlist_view), FALSE);
      gtk_tree_selection_set_mode
                               ( (GtkTreeSelection *)gtk_tree_view_get_selection
                                (GTK_TREE_VIEW (destlist_view)),
                                 GTK_SELECTION_SINGLE);
      gtk_container_add (GTK_CONTAINER (scrolled_win), destlist_view);
      gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 3);
      gtk_widget_show (destlist_view);

      /* Make bottom part of window  -------------------------------- */
      hbox2 = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (vbox, hbox2, TRUE, TRUE, 0);
      gtk_widget_show (hbox2);

      /* Check boxes */
      button = gtk_check_button_new_with_label (_("Hide app"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (mon_hide_app_checkbox),
                          srclist_view);
      gtk_widget_show (button);
      
      button = gtk_check_button_new_with_label (_("Exec actions"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (mon_actions_checkbox),
                          srclist_view);
      gtk_widget_show (button);

      mon_hide_while_monitor = FALSE;
      mon_exec_actions = TRUE;

      /* Actions button */
      button = gtk_button_new_with_label (_("Actions..."));
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (mon_edit_actions),
                          srclist_view);

      /* OK button */
      button = gtk_button_new_from_stock (GNOME_STOCK_BUTTON_OK);
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (go_monitor_log),
                          NULL);

      /* Cancel button */
      button = gtk_button_new_from_stock (GNOME_STOCK_BUTTON_CANCEL);
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (close_monitor_options),
                          NULL);
      gtk_widget_grab_focus (button);

   }
   mon_opts_visible = TRUE;
   monitorcount = 0;

   gtk_widget_show (monoptions);

}


/* ----------------------------------------------------------------------
   NAME:	mon_add_log
   DESCRIPTION:	Add a logfile to destination list.
   ---------------------------------------------------------------------- */

void
mon_add_log (GtkWidget *widget,
	   GtkWidget *list)
{
  GtkTreeSelection *selection = NULL;
  GtkTreeIter newiter;
  GtkTreeModel *model = NULL;
  const char *name;
  int sellognum;
  gboolean selected = TRUE;

  if (monitorcount >= MON_MAX_NUM_LOGS)
    {
      ShowErrMessage (_("Too many logs to monitor. Remove one and try again"));
      return;
    } 

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (srclist_view));
  selected = gtk_tree_selection_get_selected (selection, &model, &newiter);
  if (selected == FALSE)
    {
    DB (printf (_("tmp_list is NULL\n")));
    return;
    }
  gtk_tree_model_get (model, &newiter, 1, &sellognum, -1);
  
  gtk_list_store_append (destlist, &newiter); 
  gtk_list_store_set (destlist, &newiter, 0,
                      loglist[sellognum]->name, 1, sellognum, -1); 

  gtk_tree_selection_get_selected (selection, NULL, &newiter);
  gtk_list_store_remove (srclist, &newiter);

  monitorcount++;
}

/* ----------------------------------------------------------------------
   NAME:	mon_remove_log
   DESCRIPTION:	Remove the selected logfile from destination list.
   ---------------------------------------------------------------------- */

void
mon_remove_log (GtkWidget *widget,
		GtkWidget *foo)
{  
  GtkTreeModel *model = NULL;
  GtkTreeSelection *selection = NULL;
  GtkTreeIter newiter; 
  const char *name;
  int sellognum;
  static gboolean selected = TRUE;

  if (monitorcount == 0)
    return;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (destlist_view));
  selected = gtk_tree_selection_get_selected (selection, &model, &newiter);
  if (selected == FALSE)
    {
      DB (printf (_("tmp_list is NULL\n")));
      return;
    }
  gtk_tree_model_get (model, &newiter, 1, &sellognum, -1);
 
  gtk_list_store_append (srclist, &newiter);
  gtk_list_store_set (srclist, &newiter, 0,
                      loglist[sellognum]->name, 1, sellognum, -1);

  gtk_tree_selection_get_selected (selection, NULL, &newiter);
  gtk_list_store_remove (destlist, &newiter);

  monitorcount--;
}  


/* ----------------------------------------------------------------------
   NAME:          InitMonitorData
   DESCRIPTION:   
   ---------------------------------------------------------------------- */

void
InitMonitorData ()
{
}

/* ----------------------------------------------------------------------
   NAME:          mon_repaint
   DESCRIPTION:   
   ---------------------------------------------------------------------- */

int
mon_repaint (GtkWidget * widget, GdkEventExpose * event)
{
   return TRUE;
}


/* ----------------------------------------------------------------------
   NAME:          close_monitor_options
   DESCRIPTION:   Callback called when the log dialog is closed.
   ---------------------------------------------------------------------- */

void
close_monitor_options (GtkWidget * widget, gpointer client_data)
{
   if (mon_opts_visible)
      gtk_widget_hide (monoptions);
   monoptions = NULL;
   mon_opts_visible = FALSE;
   if (mon_hide_while_monitor && main_app_hidden) {
	   gtk_widget_show (app);
	   main_app_hidden = FALSE;
   }
}

/* ----------------------------------------------------------------------
   NAME:	go_monitor_log
   DESCRIPTION:	Start monitoring the logs.
   ---------------------------------------------------------------------- */

void
go_monitor_log (GtkWidget * widget, gpointer client_data)
{
   GtkWidget *label;
   GtkWidget *swin;
   GtkWidget *notebook;
   GtkWidget *frame;
   GtkWidget *vbox;
   GtkListStore *clist;
   char logfile[1024];
   int i,lnum,x,y,w,h;
   GtkWidget *clist_view;
   GtkCellRenderer *clist_cellrenderer;
   GtkTreeViewColumn *clist_column;
   GtkTreeIter newiter;
   gboolean flag = FALSE;

   /* Set flag in log structures to indicate that they should be
      monitored */
   flag = gtk_tree_model_get_iter_root (GTK_TREE_MODEL (destlist), &newiter);
   i = 0;
   while (flag == TRUE)
     {
     gtk_tree_model_get ( GTK_TREE_MODEL (destlist), &newiter, 1, &lnum, -1);
     if (loglist[lnum] != NULL)
       {
	 i++;
	 loglist[lnum]->mon_on = TRUE;
       }
     flag = gtk_tree_model_iter_next (GTK_TREE_MODEL (destlist), &newiter);
     }

   /* Close monitor dialog */
   close_monitor_options(NULL, NULL);

   /* Destroy previous monitor window if there was one */
   if (monwindow != NULL)
     {
       gtk_widget_destroy (monwindow);
       monwindow = NULL;
     }
   

   /* Open window only if at least one log is selected */
   if (i == 0)
     return;

   /* Setup timer to check log */
   gtk_timeout_add (1000, mon_check_logs, NULL);

   /* Create monitor window */
   /* setup size */
   w = 600; h = 100;
   x = gdk_screen_width () - w - 2;
   y = gdk_screen_height ()- h - 2;

   /* monwindow = gtk_window_new (GTK_WINDOW_POPUP);  */ /*  Window without borders! */
   monwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   gtk_container_set_border_width (GTK_CONTAINER (monwindow), 0);
   gtk_window_set_title (GTK_WINDOW (monwindow), _("Monitoring logs..."));
   gtk_widget_set_style (monwindow, cfg->main_style);

   gtk_widget_set_usize(monwindow, w, h);
   gtk_widget_set_uposition(monwindow, x, y);
   gtk_signal_connect (GTK_OBJECT (monwindow), "destroy",
		       GTK_SIGNAL_FUNC (close_monitor_options),
		       NULL);
   gtk_signal_connect (GTK_OBJECT (monwindow), "destroy",
		       GTK_SIGNAL_FUNC (gtk_widget_destroyed),
		       &monwindow);
   
   vbox = gtk_vbox_new (FALSE, 10);
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
   gtk_container_add( GTK_CONTAINER (monwindow), vbox);
   gtk_widget_show (vbox);
   
   notebook = gtk_notebook_new ();
   gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
   gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
   gtk_widget_show (notebook);
   
   for (i = 0; i < numlogs; i++)
     {
       if (loglist[i]->mon_on != TRUE)
	 continue;
       g_snprintf (logfile, sizeof (logfile), "%s", loglist[i]->name );
       
       frame = gtk_frame_new (NULL);
       gtk_container_set_border_width (GTK_CONTAINER (frame), 1);
       gtk_widget_set_usize (frame, 200, 150);
       gtk_widget_show (frame);
       
       /* Create destination list */
       clist = gtk_list_store_new (1, G_TYPE_STRING); 
       clist_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (clist));
       clist_cellrenderer = gtk_cell_renderer_text_new ();
       clist_column = gtk_tree_view_column_new_with_attributes
                              (NULL, clist_cellrenderer, "text", 0, NULL);
       gtk_tree_view_append_column (GTK_TREE_VIEW (clist_view), clist_column);
       gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (clist_view), FALSE);
       swin = gtk_scrolled_window_new (NULL, NULL);
       gtk_container_add (GTK_CONTAINER (swin), GTK_WIDGET (clist_view));
       gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN
                                             (clist_column), 300); 
       gtk_tree_selection_set_mode
                               ( (GtkTreeSelection *)gtk_tree_view_get_selection
                                (GTK_TREE_VIEW (clist_view)),
                                 GTK_SELECTION_BROWSE);
       gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

       gtk_tree_view_column_set_alignment (GTK_TREE_VIEW_COLUMN (clist_column),
                                          0.0);
       gtk_widget_set_style (GTK_WIDGET (clist_view), cfg->main_style);
       loglist[i]->mon_lines = (GtkWidget *) clist_view;
       gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (swin));
       gtk_widget_show_all (swin);
       
       mon_read_last_page (loglist[i]);
       label = gtk_label_new (logfile);
       gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);
     }

   mon_win_visible = TRUE;

   gtk_widget_show (monwindow);

   if (mon_hide_while_monitor) {
	   gtk_widget_hide (app);
	   main_app_hidden = TRUE;
   }
}

/* ----------------------------------------------------------------------
   NAME:	mon_read_last_page
   DESCRIPTION:	read last lines of the log into the monitor window.
   ---------------------------------------------------------------------- */

void
mon_read_last_page (Log *log)
{
  Page pg;
  char buffer[1024], buf[10];
  int i, j;
  GtkTreeIter iter;
  GtkListStore *list;
  GtkTreePath *path;

  log->mon_numlines = 0;
  fseek (log->fp, log->offset_end, SEEK_SET);
  
  /* Read pages into buffers --------------------------- */
  ReadPageUp (log, &pg);

  for (i=pg.ll-5;i<pg.ll;i++)
    {
      if (i<0)
	continue;
      mon_format_line (buffer, sizeof (buffer), &pg.line[i]);
      list = (GtkListStore *)
              gtk_tree_view_get_model (GTK_TREE_VIEW(log->mon_lines));
      gtk_list_store_append (list, &iter);
      gtk_list_store_set (list, &iter, 0, buffer, -1);
      log->mon_numlines++;
      if (log->mon_numlines > MON_MAX_NUM_LINES)
	{
	  for(j=0;j<LINES_P_PAGE;j++)
	     {
               g_snprintf (buf, sizeof (buf), "%d", j);
               gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL(list),
                                                    &iter, buf); 
               gtk_list_store_remove (list, &iter);
             }
	  log->mon_numlines -= LINES_P_PAGE;
	}
        g_snprintf (buf, sizeof (buf), "%d", log->mon_numlines);
        path = gtk_tree_path_new_from_string (buf);
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (log->mon_lines),  
                                  path, NULL, TRUE, 0, 0);
        gtk_tree_path_free (path);
    }
}

/* ----------------------------------------------------------------------
   NAME:	mon_read_new_lines
   DESCRIPTION:	Read last lines in the log file and add to the monitor
   		window.
   ---------------------------------------------------------------------- */

void
mon_read_new_lines (Log *log)
{
  Page pg;
  char buffer[1024], buf[10];
  int i,j;
  GtkTreeIter iter;
  GtkListStore *list;
  GtkTreePath *path;

  fseek (log->fp, log->offset_end, SEEK_SET);
  
  list = (GtkListStore *)
          gtk_tree_view_get_model (GTK_TREE_VIEW(log->mon_lines));

  /* Read pages into buffers --------------------------- */
  ReadPageDown (log, &pg, mon_exec_actions);

  while (TRUE) 
    {
      for (i=pg.fl;i<pg.ll;i++)
	{
	  mon_format_line (buffer, sizeof (buffer), &pg.line[i]);
	  gtk_list_store_append (list, &iter);
	  gtk_list_store_set (list, &iter, 0, buffer, -1);
	  log->mon_numlines++;
	} 
      if (log->mon_numlines > MON_MAX_NUM_LINES)
	{
	  for(j=0;j<LINES_P_PAGE;j++)
	    {
              g_snprintf (buf, sizeof (buf), "%d", j);
              gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL(list),
                                                   &iter, buf);
              gtk_list_store_remove (list, &iter);
            }
	  log->mon_numlines -= LINES_P_PAGE;
	}
      g_snprintf (buf, sizeof (buf), "%d", log->mon_numlines);
      path = gtk_tree_path_new_from_string (buf);
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (log->mon_lines),
                                    path, NULL, TRUE, 0, 0);
      gtk_tree_path_free (path);
      if (pg.islastpage)
	break;
      ReadPageDown (log, &pg, mon_exec_actions);
    }
}

/* ----------------------------------------------------------------------
   NAME:	mon_format_line
   DESCRIPTION:	format the output for a log line.
   ---------------------------------------------------------------------- */

void
mon_format_line (char *buffer, int bufsize, LogLine *line)
{
	/* FIXME: this should be translated I think */
	g_snprintf (buffer, bufsize, "%2d/%2d  %2d:%02d:%02d %s %s", 
		    (int)line->date, (int)line->month+1,
		    (int)line->hour, (int)line->min, (int)line->sec,
		    line->process, line->message);
}

/* ----------------------------------------------------------------------
   NAME:	mon_check_logs
   DESCRIPTION:	Routinly called to check wheter the logs have changed.
   ---------------------------------------------------------------------- */

gint
mon_check_logs (gpointer data)
{
  int i;

  for(i=0;i<numlogs;i++)
    {
    if (loglist[i]->mon_on != TRUE)
      continue;
    if (WasModified(loglist[i]) != TRUE)
      continue;
    mon_read_new_lines (loglist[i]);

    DB (fprintf (stderr, _("TOUCHED!!\n")));

    }
    return TRUE;
}


/* ----------------------------------------------------------------------
   NAME:	mon_hide_app_checkbox
   DESCRIPTION:	hide app checkbox toggled.
   ---------------------------------------------------------------------- */

void
mon_hide_app_checkbox (GtkWidget *widget, gpointer data)
{
	mon_hide_while_monitor =
		GTK_TOGGLE_BUTTON (widget)->active ? TRUE : FALSE;
}


/* ----------------------------------------------------------------------
   NAME:	mon_actions_checkbox
   DESCRIPTION:	actions checkbox toggled.
   ---------------------------------------------------------------------- */

void
mon_actions_checkbox (GtkWidget *widget, gpointer data)
{
	mon_exec_actions = GTK_TOGGLE_BUTTON (widget)->active ? TRUE : FALSE;
}
