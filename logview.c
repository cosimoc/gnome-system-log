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


#include <config.h>
#include <gnome.h>
#include <gconf/gconf-client.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "logview.h"
#include "logview-search.h"
#include "logview-search-dialog.h"
#include <libgnomeui/gnome-window-icon.h>
#include "gedit-output-window.h"
#include <ctype.h>

/*
 *    -------------------
 *    Function Prototypes
 *    -------------------
 */

void CreateMainWin (void);
gboolean log_repaint (void);
gboolean handle_log_mouse_button (GtkWidget *view, GdkEventButton *event);
void ExitProg (GtkWidget * widget, gpointer user_data);
void LoadLogMenu (GtkWidget * widget, gpointer user_data);
void CloseLogMenu (GtkWidget * widget, gpointer user_data);
void change_log_menu (GtkWidget * widget, gpointer user_data);
void CalendarMenu (GtkWidget * widget, gpointer user_data);
void MonitorMenu (GtkWidget* widget, gpointer user_data); 
void create_zoom_view (void);
void AboutShowWindow (GtkWidget* widget, gpointer user_data);
void CloseApp (void);
void CloseLog (Log *);
void FileSelectResponse (GtkWidget * chooser, gint response, gpointer data);
void LogInfo (GtkWidget * widget, gpointer user_data);
void UpdateStatusArea (void);
void change_log (int dir);
void open_databases (void);
void destroy (void);
void InitApp (void);
int InitPages (void);
int RepaintLogInfo (void);
int read_regexp_db (char *filename, GList **db);
int read_actions_db (char *filename, GList **db);
void print_db (GList *gb);
Log *OpenLogFile (char *);
void SaveUserPrefs(UserPrefsStruct *prefs);
char * parse_syslog(gchar * syslog_file);

void close_zoom_view (GtkWidget *widget, gpointer client_data);
void handle_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
void handle_row_activation_cb (GtkTreeView *treeview, GtkTreePath *path, 
     GtkTreeViewColumn *arg2, gpointer user_data);
void save_rows_to_expand (Log *log);
void CloseAllLogs (void);

static void toggle_calendar (void);
static void toggle_zoom (void);
static void toggle_collapse_rows (void);
static void logview_menus_set_state (int numlogs);
static void logview_search (GtkWidget *widget, gpointer data);
static void logview_close_output_window (GtkWidget *widget, gpointer user_data);
static void logview_output_window_changed (GtkWidget *widget, gchar *key, gpointer user_data);


/*
 *    ,-------.
 *    | Menus |
 *    `-------'
 */

enum {
	MENU_FILE_OPEN = 0,
	MENU_SWITCH_LOG = 2,
	MENU_MONITOR = 3,
	MENU_PROPERTIES = 5,
	MENU_CLOSE = 7,
	MENU_CLOSE_ALL = 8,
	MENU_QUIT = 9,
	MENU_FIND = 0
};

GnomeUIInfo log_menu[] = {
	GNOMEUIINFO_MENU_OPEN_ITEM(LoadLogMenu, NULL),
	GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("S_witch Log"),
			 N_("Switch between already opened logs"),
			 change_log_menu,
			 NULL, NULL,
			 GNOME_APP_PIXMAP_NONE, NULL,
			 'N', GDK_CONTROL_MASK, NULL },

    { GNOME_APP_UI_ITEM, N_("_Monitor..."),
			 N_("Monitor Logs"),
		         MonitorMenu,
			 NULL,
			 NULL,
			 GNOME_APP_PIXMAP_NONE, NULL,
			 'M', GDK_CONTROL_MASK, NULL },
	GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("_Properties"), 
				N_("Show Log Properties"), 
				LogInfo, 
				NULL, 
				NULL,
				GNOME_APP_PIXMAP_NONE, NULL, 
				'I', GDK_CONTROL_MASK, NULL },
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_CLOSE_ITEM (CloseLogMenu, NULL),
    { GNOME_APP_UI_ITEM, N_("Clos_e All"),
				N_("Close all Log files"), 
				CloseAllLogs, 
				NULL, 
				NULL,
				GNOME_APP_PIXMAP_STOCK, 
				GNOME_STOCK_MENU_CLOSE, 
				'W', GDK_CONTROL_MASK | GDK_SHIFT_MASK, NULL },
	GNOMEUIINFO_MENU_QUIT_ITEM (ExitProg, NULL),
    GNOMEUIINFO_END
};

GnomeUIInfo edit_menu[] = {
	{ GNOME_APP_UI_ITEM, N_("_Find"),
	  N_("Find pattern in logs"),
	  logview_search,
	  NULL,
	  NULL,
	  GNOME_APP_PIXMAP_STOCK,
	  GTK_STOCK_FIND,
	  'F', GDK_CONTROL_MASK, NULL },
	GNOMEUIINFO_END
};

GnomeUIInfo view_menu[] = {
	{ GNOME_APP_UI_TOGGLEITEM, N_("_Calendar"),
				N_("Show Calendar Log"), 
				toggle_calendar, 
				NULL, 
				NULL,
				GNOME_APP_PIXMAP_NONE, NULL, 
				'L', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_TOGGLEITEM, N_("_Entry Detail"), 
				N_("Show Entry Detail"), 
				toggle_zoom, 
				NULL, 
				NULL, 
				GNOME_APP_PIXMAP_NONE, NULL, 
				'D', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("_Collapse All"),
				N_("Collapse all the rows"),
				toggle_collapse_rows,
				NULL,
				NULL,
				GNOME_APP_PIXMAP_NONE, NULL, 
				'C', GDK_CONTROL_MASK, NULL },
	GNOMEUIINFO_END
};

GnomeUIInfo help_menu[] = {
      GNOMEUIINFO_HELP ("gnome-system-log"),
      GNOMEUIINFO_MENU_ABOUT_ITEM (AboutShowWindow, NULL),
      GNOMEUIINFO_END
};

GnomeUIInfo main_menu[] = {
	GNOMEUIINFO_MENU_FILE_TREE (log_menu),
	GNOMEUIINFO_MENU_EDIT_TREE (edit_menu),
	GNOMEUIINFO_MENU_VIEW_TREE (view_menu),
	GNOMEUIINFO_MENU_HELP_TREE (help_menu),
	GNOMEUIINFO_END
};
                 

/*
 *       ----------------
 *       Global variables
 *       ----------------
 */
	
GtkWidget *app = NULL;
GtkWidget *statusbar = NULL;
GtkWidget *output_window = NULL;

GList *regexp_db = NULL, *descript_db = NULL, *actions_db = NULL;
UserPrefsStruct *user_prefs = NULL;
UserPrefsStruct user_prefs_struct = {0};
ConfigData *cfg = NULL;
GtkWidget *open_file_dialog = NULL;
GtkWidget *view = NULL;

GConfClient *client;
poptContext poptCon;
gint next_opt;
int output_window_type;

struct poptOption options[] = { {
	"file",
	'f',
	POPT_ARG_STRING,
	NULL,
	1,
	NULL,
	NULL,
} };

extern GtkWidget *CalendarDialog;
extern GtkWidget *zoom_dialog;
extern Log *curlog, *loglist[];
extern int numlogs, curlognum;
extern int loginfovisible, calendarvisible;
extern int cursor_visible;
extern int zoom_visible;	


/* ----------------------------------------------------------------------
   NAME:          destroy
   DESCRIPTION:   Exit program.
   ---------------------------------------------------------------------- */

void
destroy (void)
{
   CloseApp ();
}

static gint
save_session (GnomeClient *gclient, gint phase,
              GnomeRestartStyle save_style, gint shutdown,
              GnomeInteractStyle interact_style, gint fast,
              gpointer client_data)
{
   gchar **argv;
   gint i;

   argv = g_malloc0 (sizeof (gchar *) * (numlogs+1));
   argv[0] = (gchar *) client_data;
   for ( i = 1; i <= numlogs; i++) 
       argv[i] = g_strconcat ("--file=", loglist[i-1]->name, NULL);
   
   gnome_client_set_clone_command (gclient, numlogs+1, argv);
   gnome_client_set_restart_command (gclient, numlogs+1, argv);

   g_free (argv);

   return TRUE;

}

static gboolean
restore_session (void)
{
   Log *tl;

   /* closing the log file opened by default */
   CloseLog (loglist[0]);
   curlog = NULL;
   loglist[0] = NULL;
   curlognum = 0;
   log_repaint ();
   if (loginfovisible)
       RepaintLogInfo ();
   numlogs = 0;

   next_opt = poptGetNextOpt (poptCon);

   do {
      if ( next_opt == 1) {
         gchar *f = (gchar *) poptGetOptArg (poptCon);
         if (f != NULL) {
            if ((tl = OpenLogFile (f)) != NULL) {
               curlog = tl;
               loglist[numlogs] = tl;
               numlogs++;
               curlognum = numlogs - 1;
            }
         }
         if (f)
            g_free (f);
      }
   } while ((next_opt = poptGetNextOpt (poptCon)) != -1);
   return TRUE;
}

static gint
die (GnomeClient *gclient, gpointer client_data)
{
    gtk_main_quit ();
}

/* ----------------------------------------------------------------------
   NAME:          main
   DESCRIPTION:   Program entry point.
   ---------------------------------------------------------------------- */

int
main (int argc, char *argv[])
{
   GnomeClient *gclient;
   GtkIconInfo *icon_info;

   bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
   bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
   textdomain(GETTEXT_PACKAGE);

   QueueErrMessages (TRUE);

   /*  Initialize gnome & gtk */
   gnome_program_init ("gnome-system-log",VERSION, LIBGNOMEUI_MODULE, argc, argv,
			   GNOME_PARAM_APP_DATADIR, DATADIR, NULL);

   icon_info = gtk_icon_theme_lookup_icon (gtk_icon_theme_get_default (), "logviewer", 48, 0);
   if (icon_info) {
       gnome_window_icon_set_default_from_file (gtk_icon_info_get_filename (icon_info));
       gtk_icon_info_free (icon_info);
   }
   
   poptCon = poptGetContext ("gnome-system-log", argc, (const gchar **) argv, 
							 options, 0);  
   gclient = gnome_master_client ();
   g_signal_connect (gclient, "save_yourself",
					 G_CALLBACK (save_session), (gpointer)argv[0]);
   g_signal_connect (gclient, "die", G_CALLBACK (die), NULL);

   gconf_init (argc, argv, NULL);
   client = gconf_client_get_default ();
   
   /*  Load graphics config */
   cfg = CreateConfig();
   
   InitApp ();

   log_repaint (); 

   QueueErrMessages (FALSE);
   ShowQueuedErrMessages ();
   
   if (gnome_client_get_flags (gclient) & GNOME_CLIENT_RESTORED) {
	  restore_session ();
   }

   /*  Loop application */
   gtk_main ();
   
   SaveUserPrefs(user_prefs);

   return 0;

}

/* ----------------------------------------------------------------------
   NAME:        InitApp
   DESCRIPTION: Main initialization routine.
   ---------------------------------------------------------------------- */

void
InitApp ()
{
   loginfovisible = FALSE;
   regexp_db = NULL;
   user_prefs = &user_prefs_struct;
   SetDefaultUserPrefs(user_prefs);

   /*  Display main window */
   CreateMainWin ();

   /* Read databases */
   open_databases ();

   /*  Read files and init data. */
   if (InitPages () < 0)
	 ShowErrMessage (_("No log files to open"));

}

/* ----------------------------------------------------------------------
   NAME:        CreateMainWin
   DESCRIPTION: Creates the main window.
   ---------------------------------------------------------------------- */

void
CreateMainWin ()
{
   GtkWidget *vbox, *hbox, *hbox_date;
   GtkLabel *label;
   GtkTreeStore *tree_store;
   GtkTreeSelection *selection;
   GtkTreeViewColumn *column;
   GtkCellRenderer *renderer;
   GtkWidget *scrolled_window = NULL;
   gint i;
   gchar *window_title;
   const gchar *column_titles[] = { N_("Date"), N_("Host Name"),
                                    N_("Process"), N_("Message"), NULL };

   /* Create App */

   if ((curlog != NULL) && (curlog->name != NULL) && (numlogs))
       window_title = g_strdup_printf ("%s - %s", curlog->name, APP_NAME);
   else
       window_title = g_strdup_printf (APP_NAME);
   app = gnome_app_new ("gnome-system-log", window_title);
   g_free (window_title);

   gtk_window_set_default_size ( GTK_WINDOW (app), LOG_CANVAS_W, LOG_CANVAS_H);
   gtk_signal_connect (GTK_OBJECT (app), "destroy",
		       GTK_SIGNAL_FUNC (destroy), NULL);

   /* Create menus */
   gnome_app_create_menus (GNOME_APP (app), main_menu);

   vbox = gtk_vbox_new (FALSE, 6);
   gnome_app_set_contents (GNOME_APP (app), vbox);

   logview_menus_set_state (numlogs);

   /* Create scrolled window and tree view */
   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_widget_set_sensitive (scrolled_window, TRUE);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
               GTK_POLICY_AUTOMATIC,
               GTK_POLICY_AUTOMATIC);
   gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

   /* Create Tree View */
   tree_store = gtk_tree_store_new (4,
                G_TYPE_STRING, G_TYPE_STRING,
                G_TYPE_STRING, G_TYPE_STRING);

   view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (tree_store));
   gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), TRUE);
   g_object_unref (G_OBJECT (tree_store)); 
   
   /* Add Tree View Columns */
   for (i = 0; column_titles[i]; i++) {
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_(column_titles[i]),
                    renderer, "text", i, NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE); 
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_column_set_spacing (column, 6);
        gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
   }

   gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (view));
   gtk_widget_show_all (scrolled_window);

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
   gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

   /* Add signal handlers */
   g_signal_connect (G_OBJECT (selection), "changed",
                     G_CALLBACK (handle_selection_changed_cb), NULL);

   g_signal_connect (G_OBJECT (view), "row_activated",
                     G_CALLBACK (handle_row_activation_cb), NULL);

   /* Create ouput window */
   output_window = gedit_output_window_new ();
   output_window_type = LOGVIEW_WINDOW_OUTPUT_WINDOW_NONE;
   gedit_output_window_set_select_multiple (GEDIT_OUTPUT_WINDOW (output_window),
					    GTK_SELECTION_SINGLE);
   gtk_box_pack_start (GTK_BOX (vbox), output_window, FALSE, FALSE, 0);
   g_signal_connect (G_OBJECT (output_window), "close_requested",
		     G_CALLBACK (logview_close_output_window), app);
   
   g_signal_connect (G_OBJECT (output_window), "selection_changed",
		     G_CALLBACK (logview_output_window_changed), app);

   /* Create status area at bottom */
   statusbar = gtk_statusbar_new ();
   gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

   gtk_widget_show_all (vbox);
   gtk_widget_hide (output_window);

   gtk_widget_show_now (app);

}

/* ----------------------------------------------------------------------
   NAME:          CloseLogMenu
   DESCRIPTION:   Close the current log.
   ---------------------------------------------------------------------- */

void
CloseLogMenu (GtkWidget *widget, gpointer user_data)
{
   int i;

   if (numlogs == 0)
	   return;

   CloseLog (curlog);

   numlogs--;

   logview_menus_set_state (numlogs);

   if (numlogs == 0)
   {
      curlog = NULL;
      loglist[0] = NULL;
      curlognum = 0;
      log_repaint ();
      if (loginfovisible)
	      RepaintLogInfo ();
      return;
   }
   for (i = curlognum; i < numlogs; i++)
      loglist[i] = loglist[i + 1];
   loglist[i] = NULL;

   if (curlognum > 0)
      curlognum--;
   curlog = loglist[curlognum];
   log_repaint ();

   if (loginfovisible)
      RepaintLogInfo ();

}

/* ----------------------------------------------------------------------
   NAME:          change_log_menu
   DESCRIPTION:   Switch log
   ---------------------------------------------------------------------- */

void
change_log_menu (GtkWidget * widget, gpointer user_data)
{
  gtk_widget_hide (output_window);
  change_log (1);

}

/* ----------------------------------------------------------------------
   NAME:          FileSelectResponse
   DESCRIPTION:   User selected a file.
   ---------------------------------------------------------------------- */

void
FileSelectResponse (GtkWidget * chooser, gint response, gpointer data)
{
   char *f;
   Log *tl;
   gint i;

   if (response != GTK_RESPONSE_OK) {
	   gtk_widget_destroy (chooser);
	   return;
   }

   /* Check that we haven't opened all logfiles allowed    */
   if (numlogs >= MAX_NUM_LOGS)
     {
       ShowErrMessage (_("Too many open logs. Close one and try again"));
       return;
     }

   f = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
   gtk_widget_destroy (GTK_WIDGET (chooser));

   /* Check whether we are opening the already opened log file */ 
   for ( i = 0; i < numlogs; i++)
   {
      if (strcmp (f, loglist[i]->name) == 0)
      {
         ShowErrMessage (_("File already opened"));
         g_free (f);
         return;
      }
   }      

   if (f != NULL) {
       if ((tl = OpenLogFile (f)) != NULL) {
           if (numlogs > 0) {
               memset (curlog->expand_paths, 0, 
                       sizeof(curlog->expand_paths));
               save_rows_to_expand (curlog); 
           }
           
	   curlog = tl;
	   curlog->first_time = TRUE; 
	   curlog->mon_on = FALSE;
	   loglist[numlogs] = tl;
	   numlogs++;
	   curlognum = numlogs - 1;

	   /* Clear window */
	   log_repaint ();
	   if (loginfovisible)
		   RepaintLogInfo ();
	   if (calendarvisible)
		   init_calendar_data();
	   
	   UpdateStatusArea();
	   logview_menus_set_state (numlogs);
	   
       }
   }

   g_free (f);

}

/* ----------------------------------------------------------------------
   NAME:          LoadLogMenu
   DESCRIPTION:   Open a new log defined by the user.
   ---------------------------------------------------------------------- */

void
LoadLogMenu (GtkWidget * widget, gpointer user_data)
{
   GtkWidget *chooser = NULL;

   /*  Cannot open more than MAX_NUM_LOGS */
   if (numlogs == MAX_NUM_LOGS)
     { 
       ShowErrMessage (_("Too many open logs. Close one and try again")); 
       return;
     }
   
   /*  Cannot have more than one file chooser window */
   /*  at one time. */
   if (open_file_dialog != NULL) {
	   gtk_widget_show_now (open_file_dialog);
	   gdk_window_raise (open_file_dialog->window);
	   return;
   }

   chooser = gtk_file_chooser_dialog_new (_("Open new logfile"),
					  GTK_WINDOW (app),
					  GTK_FILE_CHOOSER_ACTION_OPEN,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					  NULL);
   gtk_dialog_set_default_response (GTK_DIALOG (chooser),
		   		    GTK_RESPONSE_OK);
   gtk_window_set_default_size (GTK_WINDOW (chooser), 600, 400);

   /* Make window modal */
   gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);

   if (user_prefs->logfile != NULL)
   	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), 
				       user_prefs->logfile);

   gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_MOUSE);
   g_signal_connect (G_OBJECT (chooser), "response",
		     G_CALLBACK (FileSelectResponse), NULL);

   g_signal_connect (G_OBJECT (chooser), "destroy",
		     G_CALLBACK (gtk_widget_destroyed), &open_file_dialog);

   gtk_widget_show (chooser);

   open_file_dialog = chooser;

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
   NAME:          CloseAllLogs 
   DESCRIPTION:   Close all log files
   ---------------------------------------------------------------------- */

void 
CloseAllLogs (void)
{
   int i;
   
   if (numlogs == 0)
      return;

   for (i = 0; i < numlogs; ++i) 
       CloseLog (loglist[i]);

   numlogs = 0;
   curlognum = 0;
   curlog = NULL;
   loglist[0] = NULL;

   log_repaint ();
   if (loginfovisible)
	  RepaintLogInfo ();

  logview_menus_set_state (numlogs);

}

/* ----------------------------------------------------------------------
   NAME:          CloseApp
   DESCRIPTION:   Close everything and exit.
   ---------------------------------------------------------------------- */

void 
CloseApp (void)
{
   int i;

   for (i = 0; i < numlogs; i++)
      CloseLog (loglist[i]);

   numlogs = 0;

   gtk_main_quit ();   

}

/* ----------------------------------------------------------------------
   NAME:          open_databases
   DESCRIPTION:   Try to locate regexp and descript databases and load
   	          them.
   ---------------------------------------------------------------------- */

void
open_databases (void)
{
	char full_name[1024];
	gboolean found;

	/* Find regexp DB -----------------------------------------------------  */
	found = FALSE;
	if (cfg->regexp_db_path != NULL) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/gnome-system-log-regexp.db", cfg->regexp_db_path);
		DB (fprintf (stderr, "Looking for database in [%s]\n", cfg->regexp_db_path));
		if (access (full_name, R_OK) == 0)  {
			found = TRUE;
			read_regexp_db (full_name, &regexp_db);
		}
	}

	if ( ! found) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/share/gnome-system-log/gnome-system-log-regexp.db", LOGVIEWINSTALLPREFIX);
		if (access (full_name, R_OK) == 0) {
			found = TRUE;
			g_free (cfg->regexp_db_path);
			cfg->regexp_db_path = g_strdup (full_name);
			read_regexp_db (full_name, &regexp_db);
		}
	}

	/* Find description DB ------------------------------------------------  */
	found = FALSE;
	if (cfg->descript_db_path != NULL) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/gnome-system-log-descript.db", cfg->descript_db_path);
		DB (fprintf (stderr, "Looking for database in [%s]\n", cfg->descript_db_path));
		if (access (full_name, R_OK) == 0) {
			read_descript_db (full_name, &descript_db);
			found = TRUE;
		}
	}

	if ( ! found) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/share/gnome-system-log/gnome-system-log-descript.db", LOGVIEWINSTALLPREFIX);
		if (access (full_name, R_OK) == 0) {
			found = TRUE;
			g_free (cfg->descript_db_path);
			cfg->descript_db_path = g_strdup (full_name);
			read_descript_db (full_name, &descript_db);
		}
	}


	/* Find action DB ------------------------------------------------  */
	found = FALSE;
	g_snprintf (full_name, sizeof (full_name),
		    "%s/.gnome/gnome-system-log-actions.db", g_get_home_dir ());
	DB (fprintf (stderr, "Looking for database in [%s/.gnome]\n",
		     g_get_home_dir ()));
	if (access (full_name, R_OK) == 0) {
		found = TRUE;
		read_actions_db (full_name, &actions_db);
	}

	if ( ! found && cfg->action_db_path != NULL) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/gnome-system-log-actions.db", cfg->action_db_path);
		DB (fprintf (stderr, "Looking for database in [%s]\n", cfg->action_db_path));
		if (access (full_name, R_OK) == 0) {
			found = TRUE;
			read_actions_db (full_name, &actions_db);
		}
	}


	if ( ! found) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/share/gnome-system-log/gnome-system-log-actions.db", LOGVIEWINSTALLPREFIX);
		if (access (full_name, R_OK) == 0) {
			found = TRUE;
			g_free (cfg->action_db_path);
			cfg->action_db_path = g_strdup (full_name);
			read_actions_db (full_name, &actions_db);
		}
	}

	/* If debugging then print DB */
	DB (print_db (regexp_db));

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

void SetDefaultUserPrefs(UserPrefsStruct *prefs)
{
	/* Make defaults configurable later */
	/* Will have to save prefs. eventually too*/
	gchar *logfile = NULL;
	struct stat filestat;
	
	logfile = gconf_client_get_string (client, "/apps/gnome-system-log/logfile", NULL);
	if (logfile != NULL && strcmp (logfile, "")) {
		prefs->logfile = g_strdup (logfile);
		g_free (logfile);
	}
	else {

		/* For first time running, try parsing /etc/syslog.conf */
		if (lstat("/etc/syslog.conf", &filestat) == 0) {
			if ((logfile = parse_syslog("/etc/syslog.conf")) == NULL);
			prefs->logfile = g_strdup (logfile);
		}
		else if (lstat("/var/adm/messages", &filestat) == 0) 
			prefs->logfile = g_strdup ("/var/adm/messages");
		else if (lstat("/var/log/messages", &filestat) == 0) 
			prefs->logfile = g_strdup ("/var/log/messages");
		else if (lstat("/var/log/sys.log", &filestat) == 0) 
			prefs->logfile = g_strdup ("/var/log/sys.log");
		else
			prefs->logfile = NULL;
	}
}

char * parse_syslog(gchar * syslog_file) {
/* Most of this stolen from sysklogd sources */
    char * logfile = NULL;
    char cbuf[BUFSIZ];
    char *cline;
    char *p;
    FILE * cf;
    if ((cf = fopen(syslog_file, "r")) == NULL) {
        fprintf(stderr, "Could not open file: (%s)\n", syslog_file);
        return NULL;
    }
    cline = cbuf;
    while (fgets(cline, sizeof(cbuf) - (cline - cbuf), cf) != NULL) {
        for (p = cline; isspace(*p); ++p);
        if (*p == '\0' || *p == '#')
            continue;
        for (;*p && !strchr("\t ", *p); ++p);
        while (*p == '\t' || *p == ' ')
            p++;
        if (*p == '-')
            p++;
        if (*p == '/') {
            logfile = g_strdup (p);
            /* remove trailing newline */
            if (logfile[strlen(logfile)-1] == '\n')
                logfile[strlen(logfile)-1] = '\0';
            fprintf(stderr, "Found a logfile: (%s)\n", logfile);
            return logfile;
        }
        /* I don't totally understand this code
           it came from syslogd.c and is supposed
           to read run-on lines that end with \
           FIXME?? */
        /*
        if (*p == '\\') {
            if ((p - cbuf) > BUFSIZ - 30) {
                cline = cbuf;
            } else {
                *p = 0;
                cline = p;
                continue;
            }
        }  else
            cline = cbuf;
        *++p = '\0';
        */
        
    }
    return logfile; 
}


 
void SaveUserPrefs(UserPrefsStruct *prefs)
{
    if (gconf_client_key_is_writable (client, "/apps/gnome-system-log/logfile", NULL) &&
	prefs->logfile != NULL)
	    gconf_client_set_string (client, "/apps/gnome-system-log/logfile", prefs->logfile, NULL);
}

static void 
toggle_calendar (void)
{
    if (calendarvisible) {
	calendarvisible = FALSE;
	gtk_widget_hide (CalendarDialog);
    }
    else
	CalendarMenu (app, NULL);
}

static void
toggle_zoom (void)
{
    if (zoom_visible) {
	close_zoom_view (app, NULL);
    }
    else
	create_zoom_view ();

}

static void 
toggle_collapse_rows (void)
{
    gtk_tree_view_collapse_all (GTK_TREE_VIEW (view));
}

static void
logview_search (GtkWidget *widget, gpointer data)
{
	static GtkWidget *dialog = NULL;

	if (dialog != NULL) {
		gtk_window_present (GTK_WINDOW (dialog));
		return;
	}

	dialog = logview_search_dialog_new (GTK_WINDOW (app));
	g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer)&dialog);
	gtk_widget_show (dialog);
}

static void
logview_close_output_window (GtkWidget *widget, gpointer user_data)
{
	gedit_output_window_clear (GEDIT_OUTPUT_WINDOW (widget));
	gtk_widget_hide (widget);
}

static void
logview_output_window_changed (GtkWidget *widget, gchar *key, gpointer user_data)
{
	GtkTreePath *child_path, *path;
	GtkTreeModel *model;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	child_path = logview_tree_path_from_key (model, key);
	if (child_path) {
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (view), child_path);
		gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
						child_path);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view),
					      child_path, NULL, FALSE, 0, 0);
	}
	return;
}

static void
logview_menus_set_state (int numlogs)
{
	int i;
	gtk_widget_set_sensitive (log_menu[MENU_PROPERTIES].widget, (numlogs > 0)); 
	gtk_widget_set_sensitive (log_menu[MENU_CLOSE].widget, (numlogs > 0)); 
	gtk_widget_set_sensitive (log_menu[MENU_CLOSE_ALL].widget, (numlogs > 0)); 
	gtk_widget_set_sensitive (log_menu[MENU_MONITOR].widget, (numlogs > 0)); 
	for ( i = 0; i < 3; i++) 
		gtk_widget_set_sensitive (view_menu[i].widget, (numlogs > 0)); 

	gtk_widget_set_sensitive (log_menu[MENU_SWITCH_LOG].widget, (numlogs > 1));

}
