#ifndef GNOMELOCALEDIR
#define GNOMELOCALEDIR "/usr/share/locale"
#endif
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


#include <unistd.h>
#include <time.h>
#include <config.h>
#include <sys/stat.h>
#include "gtk/gtk.h"
#include "logview.h"
#include "gnome.h"
#include "log.xpm"

/*
 *    -------------------
 *    Function Prototypes
 *    -------------------
 */

void repaint (GtkWidget * canvas, GdkRectangle * area);
void CreateMainWin ();
void log_repaint (GtkWidget * canvas, GdkRectangle * area);
void PointerMoved (GtkWidget * canvas, GdkEventMotion * event);
void HandleLogKeyboard (GtkWidget * win, GdkEventKey * event_key);
void handle_log_mouse_button (GtkWidget * win, GdkEventButton *event);
void ExitProg (GtkWidget * widget, gpointer user_data);
void LoadLogMenu (GtkWidget * widget, gpointer user_data);
void CloseLogMenu (GtkWidget * widget, gpointer user_data);
void change_log_menu (GtkWidget * widget, gpointer user_data);
void CalendarMenu (GtkWidget * widget, gpointer user_data);
void MonitorMenu (GtkWidget* widget, gpointer user_data); 
void create_zoom_view (GtkWidget * widget, gpointer user_data);
void AboutShowWindow (GtkWidget* widget, gpointer user_data);
void CloseApp ();
void CloseLog (Log *);
void ShowErrMessage (char *);
void MainWinScrolled (GtkAdjustment *adjustment, GtkRange *);
void CanvasResized (GtkWidget *widget, GtkAllocation *allocation);
void ScrollWin (GtkRange *range, gpointer event);
void LogInfo (GtkWidget * widget, gpointer user_data);
void UpdateStatusArea ();
void set_scrollbar_size (int);
void change_log (int dir);
void open_databases ();
int InitApp ();
int InitPages ();
int RepaintLogInfo (GtkWidget * widget, GdkEventExpose * event);
int read_regexp_db (char *filename, GList **db);
int read_actions_db (char *filename, GList **db);
void print_db (GList *gb);
Log *OpenLogFile (char *);
GtkWidget *new_pixmap_from_data (char  **, GdkWindow *, GdkColor *);
GtkWidget *create_menu (char *item[], int n);

/*
 *    ,-------.
 *    | Menus |
 *    `-------'
 */


GnomeUIInfo file_menu[] = {
        {GNOME_APP_UI_ITEM, N_("Open log...            "), 
	 N_("Open log"), LoadLogMenu, NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN, 0, 0, NULL},
        {GNOME_APP_UI_ITEM, N_("Export log...          "), 
	 N_("Export log"), StubCall, NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE_AS, 0, 0, NULL},
        {GNOME_APP_UI_ITEM, N_("Close log              "), 
	 N_("Close log"), CloseLogMenu, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
        {GNOME_APP_UI_ITEM, N_("Switch log             "), 
	 N_("Switch log"), change_log_menu, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
        {GNOME_APP_UI_ITEM, N_("Monitor..              "), 
	 N_("Monitor log"), MonitorMenu, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
        {GNOME_APP_UI_ITEM, N_("Exit                   "), 
	 N_("Exit program"), ExitProg, NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'E', GDK_CONTROL_MASK, NULL},
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL}
};

GnomeUIInfo view_menu[] = {
        {GNOME_APP_UI_ITEM, N_("Calendar                "), 
	 N_("Show calendar log"), CalendarMenu, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 'C', GDK_CONTROL_MASK, NULL},
        {GNOME_APP_UI_ITEM, N_("Log stats               "), 
	 N_("Show log stats"), LogInfo, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 'I', GDK_CONTROL_MASK, NULL},
        {GNOME_APP_UI_ITEM, N_("Zoom                    "), 
	 N_("Show line info"), create_zoom_view, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 'Z', GDK_CONTROL_MASK, NULL},
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL}
};

GnomeUIInfo filter_menu[] = {
        {GNOME_APP_UI_ITEM, N_("Select...               "), 
	 N_("Select log events"), StubCall, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
        {GNOME_APP_UI_ITEM, N_("Filter..                "), 
	 N_("Filter log events"), StubCall, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL}
};

GnomeUIInfo help_menu[] = {
        {GNOME_APP_UI_ITEM, N_("About..                "), 
	 N_("Info about logview"), AboutShowWindow,
         NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL}
};

GnomeUIInfo main_menu[] = {
	GNOMEUIINFO_MENU_FILE_TREE(file_menu),
	GNOMEUIINFO_MENU_VIEW_TREE(view_menu),
        {GNOME_APP_UI_SUBTREE, N_("F_ilter"), NULL,  filter_menu, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	GNOMEUIINFO_MENU_HELP_TREE(help_menu),
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL}
};
                 

/*
 *       ----------------
 *       Global variables
 *       ----------------
 */


GtkWidget *app;
GtkWidget *main_win_scrollbar;
GtkLabel *filename_label, *date_label;

GList *regexp_db, *descript_db, *actions_db;
ConfigData *cfg;
int open_log_visible;

extern GdkGC *gc;
extern Log *curlog, *loglist[];
extern int numlogs, curlognum;
extern int loginfovisible, calendarvisible;
extern int cursor_visible;

static gint init_timer = -1;


/* ----------------------------------------------------------------------
   NAME:          destroy
   DESCRIPTION:   Exit program.
   ---------------------------------------------------------------------- */

void
destroy (void)
{
   CloseApp (0);
}

/* ----------------------------------------------------------------------
   NAME:          main
   DESCRIPTION:   Program entry point.
   ---------------------------------------------------------------------- */

int
main (int argc, char *argv[])
{
  /*  Initialize gnome & gtk */
  gnome_init ("logview", VERSION, argc, argv);

  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);
  
  /*  Load graphics config */
  cfg = CreateConfig();
  
  /*  Show about window */
  /* AboutShowWindow (NULL, NULL); */

  /*  Add a timer that will start initialization after */
  /*  after the title window appears. */
  init_timer = gtk_timeout_add (1000, (GtkFunction) InitApp, NULL);
   
  /*  Loop application */
  gtk_main ();
  
  return 0;
}

/* ----------------------------------------------------------------------
   NAME:        InitApp
   DESCRIPTION: Main initialization routine.
   ---------------------------------------------------------------------- */

int
InitApp ()
{
  /*  Initialize variables */
  loginfovisible = FALSE;
  regexp_db = NULL;

  /* Read databases */
  open_databases ();

  /*  Read files and init data. */
  if (InitPages () < 0)
    ShowErrMessage (_("No log files to open"));

  /*  Display main window */
  CreateMainWin ();

  /*  Remove timer by returning FALSE */
  return FALSE;

}

/* ----------------------------------------------------------------------
   NAME:        CreateMainWin
   DESCRIPTION: Creates the main window.
   ---------------------------------------------------------------------- */

void
CreateMainWin ()
{
   GtkWidget *canvas;
   GtkWidget *w, *box, *hbox, *hbox2;
   GtkWidget *frame, *padding;
   GtkLabel *label;
   GtkObject *adj;
   GdkPixmap *icon_pixmap;
   GdkBitmap *icon_bitmap;
   GtkAllocation req_size;

   /* Create App */

   app = gnome_app_new ("Logview", _("System Log Viewer"));

   gtk_container_set_border_width ( GTK_CONTAINER (app), 0);
   gtk_window_set_default_size ( GTK_WINDOW (app), LOG_CANVAS_W, LOG_CANVAS_H);
   req_size.x = req_size.y = 0;
   req_size.width = 400;
   req_size.height = 400;
   gtk_widget_size_allocate ( GTK_WIDGET (app), &req_size );
   gtk_signal_connect (GTK_OBJECT (app), "destroy",
		       GTK_SIGNAL_FUNC (destroy), NULL);
   gtk_signal_connect (GTK_OBJECT (app), "delete_event",
		       GTK_SIGNAL_FUNC (destroy), NULL);

   /* Create menus */
   gnome_app_create_menus (GNOME_APP (app), main_menu);

   box = gtk_vbox_new (FALSE, 0);
   gnome_app_set_contents (GNOME_APP (app), box);

   /* Deactivate unfinished items */
   gtk_widget_set_state (file_menu[1].widget, GTK_STATE_INSENSITIVE);
   if (numlogs < 2)
     gtk_widget_set_state (file_menu[3].widget, GTK_STATE_INSENSITIVE);
   gtk_widget_set_state (filter_menu[0].widget, GTK_STATE_INSENSITIVE);
   gtk_widget_set_state (filter_menu[1].widget, GTK_STATE_INSENSITIVE);


   /* Create main canvas and scroll bar */
   frame = gtk_frame_new (NULL);
   gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
   gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
   gtk_widget_set_style (frame, cfg->main_style);
   gtk_widget_show (frame);


   hbox = gtk_hbox_new (FALSE, 0);

   w = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w),
				   GTK_POLICY_AUTOMATIC, 
				   GTK_POLICY_AUTOMATIC);
   gtk_widget_show (w);
               
   canvas = gtk_drawing_area_new ();
   //gtk_drawing_area_size (GTK_DRAWING_AREA (canvas), 2*LOG_CANVAS_W, LOG_CANVAS_H); 
   gtk_drawing_area_size (GTK_DRAWING_AREA (canvas), 2*LOG_CANVAS_W, 10); 
   //gtk_widget_set_usize ( GTK_WIDGET (canvas), LOG_CANVAS_W, LOG_CANVAS_H);
   gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (w), canvas);
   gtk_box_pack_start (GTK_BOX(hbox), w, TRUE, TRUE, 0);

   if (curlog != NULL)
     adj = gtk_adjustment_new ( curlog->ln, 0.0, curlog->lstats.numlines+LINES_P_PAGE, 
				1.0, 10.0, (float) 1);
   else
     adj = gtk_adjustment_new (100.0, 0.0, 101.0, 1, 10, LINES_P_PAGE);

   main_win_scrollbar = (GtkWidget *)gtk_vscrollbar_new (GTK_ADJUSTMENT(adj));
   gtk_range_set_update_policy (GTK_RANGE (main_win_scrollbar), GTK_UPDATE_CONTINUOUS);

   gtk_box_pack_start (GTK_BOX(hbox), main_win_scrollbar, FALSE, FALSE, 0);
   gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		       (GtkSignalFunc) MainWinScrolled,
		       (gpointer) main_win_scrollbar);       
   gtk_signal_connect (GTK_OBJECT (main_win_scrollbar), "button_release_event", 
 		       (GtkSignalFunc) ScrollWin, 
 		       (gpointer) main_win_scrollbar);        
   gtk_widget_show (main_win_scrollbar);  


   gtk_container_add (GTK_CONTAINER (frame), hbox);


   
   /*  Install event handlers */
   gtk_signal_connect (GTK_OBJECT (canvas), "expose_event",
		       GTK_SIGNAL_FUNC (log_repaint), NULL);
   gtk_signal_connect (GTK_OBJECT (canvas), "motion_notify_event",
		       GTK_SIGNAL_FUNC (PointerMoved), NULL);
   gtk_signal_connect (GTK_OBJECT (app), "key_press_event",
		       GTK_SIGNAL_FUNC (HandleLogKeyboard), NULL);
   gtk_signal_connect (GTK_OBJECT (app), "button_press_event",
		       GTK_SIGNAL_FUNC (handle_log_mouse_button), NULL);
   gtk_signal_connect (GTK_OBJECT (canvas), "size_allocate",
		       GTK_SIGNAL_FUNC (CanvasResized), NULL);
   gtk_widget_set_events (canvas, GDK_EXPOSURE_MASK |
			  GDK_BUTTON_PRESS_MASK |
			  GDK_POINTER_MOTION_MASK );

   gtk_widget_set_events (app, GDK_KEY_PRESS_MASK);


   /*gtk_widget_set_style (canvas, cfg->black_bg_style); */
   gtk_widget_set_style (canvas, cfg->white_bg_style);
   gtk_widget_show (canvas);


   /* Create status area at bottom */
   hbox2 = gtk_hbox_new (FALSE, 2);
   gtk_container_set_border_width ( GTK_CONTAINER (hbox2), 3);

   label = (GtkLabel *)gtk_label_new (_("Filename: "));
   gtk_label_set_justify (label, GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET (label), FALSE, FALSE, 0);
   gtk_widget_show (GTK_WIDGET (label));  

   filename_label = (GtkLabel *)gtk_label_new ("");
   gtk_widget_show ( GTK_WIDGET (filename_label));  
   gtk_label_set_justify (label, GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET (filename_label), 
		       FALSE, FALSE, 0);
   
   /* Add padding to right justify */
   padding = gtk_label_new (" ");
   gtk_widget_show (padding);
   gtk_box_pack_start (GTK_BOX (hbox2), padding, TRUE, TRUE, 0);

   label = (GtkLabel *)gtk_label_new (_("Date: "));
   gtk_label_set_justify (label, GTK_JUSTIFY_RIGHT);
   gtk_widget_set_usize (GTK_WIDGET(label), 40, -1);
   gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET (label), FALSE, FALSE, 0);
   gtk_widget_show (GTK_WIDGET (label));  

   date_label = (GtkLabel *)gtk_label_new ("");
   gtk_widget_show (GTK_WIDGET (date_label));  
   gtk_widget_set_usize (GTK_WIDGET (label), 60, -1);
   gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET (date_label), FALSE, FALSE, 0);

   gtk_widget_show (hbox2);

   gtk_box_pack_start (GTK_BOX (box), hbox2, FALSE, FALSE, 0);

   gtk_widget_show (box);
   gtk_widget_show (hbox);
   gtk_widget_show (app);

   // Create application icon using tigerts' log
   icon_pixmap = gdk_pixmap_create_from_xpm_d (frame->window, &icon_bitmap, 
					       NULL, log_xpm);
   if (icon_pixmap)
     {
       gdk_window_set_icon (GTK_WIDGET(app)->window, NULL, icon_pixmap, icon_bitmap);
       gdk_window_set_icon_name (GTK_WIDGET(app)->window, "Logview");
     }
   else
     fprintf (stderr, "Couldn't create icon pixmap!\n");
   

}

/* ----------------------------------------------------------------------
   NAME:          MainScreenResized
   DESCRIPTION:   The main screen was resized.
   ---------------------------------------------------------------------- */

void
CanvasResized (GtkWidget *widget, GtkAllocation *allocation)
{
  if (allocation)
    printf ("Main screen resized!\n New size = (%d,%d)\n", 
	    allocation->width, allocation->height);
  else
    printf ("Main screen resized!\nNo allocation :(\n");

}

/* ----------------------------------------------------------------------
   NAME:          ScrollWin
   DESCRIPTION:   When the mouse button is released we scroll the window.
   ---------------------------------------------------------------------- */

void
ScrollWin (GtkRange *range, gpointer event)
{
  int newln;
  
  newln = (int) range->adjustment->value;
  if (newln >= curlog->lstats.numlines || newln == 0)
    return;
  
  /* Goto mark */
  MoveToMark (curlog);
  curlog->firstline = 0;
  
  ScrollDown (newln - curlog->curmark->ln);
  
  /* Repaint screen */
  log_repaint(NULL, NULL);
     
  return;
}

/* ----------------------------------------------------------------------
   NAME:          MainWinScrolled
   DESCRIPTION:   main window scrolled
   ---------------------------------------------------------------------- */

void
MainWinScrolled (GtkAdjustment *adjustment, GtkRange *range)
{
  int newln, howmuch;
  DateMark *mark;

  newln = (int) range->adjustment->value;

 if (newln == 0)
   return;

 if (newln >= curlog->lstats.numlines)
   newln == curlog->lstats.numlines - 1;

  /* Find mark which has this line */
  mark = curlog->curmark;
  if (mark == NULL)
    return;

  if (newln >= mark->ln)
    {
      while( mark->next != NULL)
	{
	  if (newln <= mark->next->ln)
	    break;
	  mark = mark->next;
	}
    }
  else
    {
      while( mark->prev != NULL)
	{
	  if (newln >= mark->ln)
	    break;
	  mark = mark->prev;
	}
    }

  /* Now lets make it the current mark */
  cursor_visible = FALSE;
  howmuch = newln - curlog->ln;
  curlog->ln = newln;
  if (mark != curlog->curmark)
    {
      curlog->curmark = mark;
      MoveToMark (curlog);
      curlog->firstline = 0;
      howmuch = newln - mark->ln;
    }

  /* Update status area */
  UpdateStatusArea ();

  /* Only scroll when the scrollbar is released */
  if (howmuch > 0)
      ScrollDown (howmuch);
  else
      ScrollUp (-1*howmuch);

  if (howmuch != 0)
    log_repaint(NULL, NULL);
  
  return;
}


/* ----------------------------------------------------------------------
   NAME:          set_scrollbar_size
   DESCRIPTION:   Set size of scrollbar acording to file.
   ---------------------------------------------------------------------- */

void set_scrollbar_size (int num_lines)
{
  GtkObject *adj;


  /*adj = gtk_adjustment_new ( curlog->ln, 0.0, num_lines+LINES_P_PAGE, 1.0, 10.0, (float) LINES_P_PAGE);*/
  adj = gtk_adjustment_new ( curlog->ln, 0.0, num_lines+LINES_P_PAGE, 1.0, 10.0, (float) 1);
  gtk_range_set_adjustment ( GTK_RANGE (main_win_scrollbar), GTK_ADJUSTMENT (adj) );
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      (GtkSignalFunc) MainWinScrolled,
		      (gpointer) main_win_scrollbar);       
  gtk_adjustment_set_value (GTK_ADJUSTMENT(adj), curlog->ln);
  gtk_widget_realize (main_win_scrollbar);
}

/* ----------------------------------------------------------------------
   NAME:          CloseLogMenu
   DESCRIPTION:   Close the current log.
   ---------------------------------------------------------------------- */

void
CloseLogMenu (GtkWidget * widget, gpointer user_data)
{
   int i;

   if (numlogs == 0)
      return;

   CloseLog (curlog);
   numlogs--;
   if (numlogs == 0)
   {
      curlog = NULL;
      loglist[0] = NULL;
      curlognum = 0;
      log_repaint (NULL, NULL);
      if (loginfovisible)
	 RepaintLogInfo (NULL, NULL);
      return;
   }
   for (i = curlognum; i < numlogs; i++)
      loglist[i] = loglist[i + 1];
   loglist[i] = NULL;

   if (curlognum > 0)
      curlognum--;
   curlog = loglist[curlognum];
   log_repaint (NULL, NULL);

   if (loginfovisible)
      RepaintLogInfo (NULL, NULL);

   /* Change menu entry if there is only one log */
   if (numlogs < 2)
     gtk_widget_set_state (file_menu[3].widget, GTK_STATE_INSENSITIVE);

   set_scrollbar_size (curlog->lstats.numlines);
}

/* ----------------------------------------------------------------------
   NAME:          change_log_menu
   DESCRIPTION:   Switch log
   ---------------------------------------------------------------------- */

void
change_log_menu (GtkWidget * widget, gpointer user_data)
{
  change_log (1);
}

/* ----------------------------------------------------------------------
   NAME:          FileSelectCancel
   DESCRIPTION:   User selected a file.
   ---------------------------------------------------------------------- */

void
FileSelectCancel (GtkWidget * w, GtkFileSelection * fs)
{
   gtk_widget_destroy (GTK_WIDGET (fs));
   open_log_visible = FALSE;
}

/* ----------------------------------------------------------------------
   NAME:          FileSelectOk
   DESCRIPTION:   User selected a file.
   ---------------------------------------------------------------------- */

void
FileSelectOk (GtkWidget * w, GtkFileSelection * fs)
{
   char f[255];
   Log *tl;

   /* Check that we haven't opened all logfiles allowed    */
   if (numlogs >= MAX_NUM_LOGS)
     {
       ShowErrMessage (_("Too many open logs. Close one and try again"));
       return;
     }

   strncpy(f, gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs)), 254);
   f[254] = '\0';
   gtk_widget_destroy (GTK_WIDGET (fs));

   open_log_visible = FALSE;
   if (f != NULL)
      if ((tl = OpenLogFile (f)) != NULL)
      {
	 curlog = tl;
	 loglist[numlogs] = tl;
	 numlogs++;
	 curlognum = numlogs - 1;
	 /* Set main scrollbar */
	 set_scrollbar_size (curlog->lstats.numlines);

	 /* Clear window */
	 log_repaint (NULL, NULL);
	 if (loginfovisible)
	   RepaintLogInfo (NULL, NULL);
	 if (calendarvisible)
	   init_calendar_data();
	 UpdateStatusArea();
	 set_scrollbar_size (curlog->lstats.numlines);
	 if (numlogs >= 2)
	   gtk_widget_set_sensitive (file_menu[3].widget, TRUE);
      }

}

/* ----------------------------------------------------------------------
   NAME:          LoadLogMenu
   DESCRIPTION:   Open a new log defined by the user.
   ---------------------------------------------------------------------- */

void
LoadLogMenu (GtkWidget * widget, gpointer user_data)
{
   GtkWidget *filesel = NULL;

   /*  Cannot open more than MAX_NUM_LOGS */
   if (numlogs == MAX_NUM_LOGS)
      return;
   
   /*  Cannot have more than one fileselect window */
   /*  at one time. */
   if (open_log_visible)
     return;


   filesel = gtk_file_selection_new (_("Open new logfile"));

   /* Make window modal */
   gtk_window_set_modal (GTK_WINDOW (filesel), TRUE);

   gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), 
   				    PATH_MESSAGES);
   gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
   gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		       "clicked", (GtkSignalFunc) FileSelectOk,
		       filesel);
   gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			      "clicked", (GtkSignalFunc) FileSelectCancel,
			      GTK_OBJECT (filesel));

   if (!GTK_WIDGET_VISIBLE (filesel))
      gtk_widget_show (filesel);
   else
      gtk_widget_destroy (filesel);

   open_log_visible = TRUE;
}





/* ----------------------------------------------------------------------
   NAME:          ExitProg
   DESCRIPTION:   Callback to call when program exits.
   ---------------------------------------------------------------------- */

void 
ExitProg (GtkWidget * widget, gpointer user_data)
{
   CloseApp ();
}

/* ----------------------------------------------------------------------
   NAME:          CloseApp
   DESCRIPTION:   Close everything and exit.
   ---------------------------------------------------------------------- */

void 
CloseApp ()
{
   int i;

   for (i = 0; i < numlogs; i++)
      CloseLog (loglist[i]);

   gtk_exit (0);
}

/* ----------------------------------------------------------------------
   NAME:          open_databases
   DESCRIPTION:   Try to locate regexp and descript databases and load
   	          them.
   ---------------------------------------------------------------------- */

void
open_databases ()
{
  char full_name[255];
  int found;

  /* Find regexp DB -----------------------------------------------------  */
  found = FALSE;
  if (cfg->regexp_db_path != NULL)
    {
      sprintf (full_name, "%s/logview-regexp.db", cfg->regexp_db_path);
      fprintf (stderr, "Looking for database in [%s]\n", cfg->regexp_db_path);
      if (access (full_name, R_OK) == 0) 
	found = TRUE;
    }

  strncpy (full_name, LOGVIEWINSTALLPREFIX, 200);
  strncat (full_name, "share/logview/logview-regexp.db", 40);
  if (access (full_name, R_OK) == 0)
     {
         found = TRUE;
	 cfg->regexp_db_path = g_strdup (full_name);
         read_regexp_db (full_name, &regexp_db);
     }
  else
    regexp_db = NULL;

  /* Find description DB ------------------------------------------------  */
  found = FALSE;
  if (cfg->descript_db_path != NULL)
    {
      sprintf (full_name, "%s/logview-descript.db", cfg->descript_db_path);
      fprintf (stderr, "Looking for database in [%s]\n", cfg->descript_db_path);
      if (access (full_name, R_OK) == 0) 
	found = TRUE;
    }

  strncpy (full_name, LOGVIEWINSTALLPREFIX, 200);
  strncat (full_name, "share/logview/logview-descript.db", 40);
  if (access (full_name, R_OK) == 0)
     {
         found = TRUE;
	 cfg->descript_db_path = g_strdup (full_name);
         read_descript_db (full_name, &descript_db);
     }
  else
    descript_db = NULL;


  /* Find action DB ------------------------------------------------  */
  found = FALSE;
  if (cfg->action_db_path != NULL)
    {
      sprintf (full_name, "%s/logview-actions.db", cfg->action_db_path);
      fprintf (stderr, "Looking for database in [%s]\n", cfg->action_db_path);
      if (access (full_name, R_OK) == 0) 
	found = TRUE;
    }


  strncpy (full_name, LOGVIEWINSTALLPREFIX, 200);
  strncat (full_name, "share/logview/logview-actions.db", 40);
  if (access (full_name, R_OK) == 0)
     {
         found = TRUE;
	 cfg->action_db_path = g_strdup (full_name);
         read_actions_db (full_name, &actions_db);
     }
  else
    actions_db = NULL;


  /* If debugging then print DB */
  print_db (regexp_db);

  return;
}

/* ----------------------------------------------------------------------
   NAME:          IsLeapYear
   DESCRIPTION:   Return TRUE if year is a leap year.
   ---------------------------------------------------------------------- */
int
IsLeapYear (int year)
{
   if ((1900 + year) % 4 == 0)
      return TRUE;
   else
      return FALSE;
}
