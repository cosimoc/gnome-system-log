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
#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomeui/gnome-ui-init.h>
#include <libgnomeui/gnome-client.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/goption.h>
#include "logview.h"
#include "log_repaint.h"
#include "logrtns.h"
#include "info.h"
#include "zoom.h"
#include "monitor.h"
#include "about.h"
#include "desc_db.h"
#include "misc.h"
#include "logview-findbar.h"
#include "userprefs.h"
#include "configdata.h"

static GObjectClass *parent_class;
static GSList *logview_windows = NULL;
static GConfClient *client = NULL;
static UserPrefsStruct *user_prefs;
static gboolean restoration_complete = FALSE;

#define APP_NAME _("System Log Viewer")

/*
 *    -------------------
 *    Function Prototypes
 *    -------------------
 */

void destroy (GObject *object, gpointer data);
static void CreateMainWin (LogviewWindow *window_new);
static void LoadLogMenu (GtkAction *action, GtkWidget *parent_window);
static void CloseLogMenu (GtkAction *action, GtkWidget *callback_data);
static void FileSelectResponse (GtkWidget * chooser, gint response, gpointer data);
static void open_databases (void);
GtkWidget *logview_create_monitor_widget (LogviewWindow *window);
static GtkWidget *logview_create_window (void);
static void logview_menu_item_set_state (LogviewWindow *logviewwindow, char *path, gboolean state);
static void toggle_calendar (GtkAction *action, GtkWidget *callback_data);
static void toggle_zoom (GtkAction *action, GtkWidget *callback_data);
static void toggle_collapse_rows (GtkAction *action, GtkWidget *callback_data);
static void toggle_monitor (GtkAction *action, GtkWidget *callback_data);
static void logview_menus_set_state (LogviewWindow *logviewwindow);
static void logview_search (GtkAction *action, GtkWidget *callback_data);
static void logview_help (GtkAction *action, GtkWidget *parent_window);
static void logview_copy (GtkAction *action, GtkWidget *callback_data);
static void logview_select_all (GtkAction *action, GtkWidget *callback_data);
static int logview_count_logs (void);
gboolean window_size_changed_cb (GtkWidget *widget, GdkEventConfigure *event, 
				 gpointer data);
GType logview_window_get_type (void);
static void loglist_add_log (LogviewWindow *window, Log *log);
Log *logview_find_log_from_name (LogviewWindow *logview, gchar *name);
static void loglist_select_log_from_name (LogviewWindow *logview, gchar *logname);


/*
 *    ,-------.
 *    | Menus |
 *    `-------'
 */

static GtkActionEntry entries[] = {
	{ "FileMenu", NULL, N_("_Log"), NULL, NULL, NULL },
	{ "EditMenu", NULL, N_("_Edit"), NULL, NULL, NULL },
	{ "ViewMenu", NULL, N_("_View"), NULL, NULL, NULL },
	{ "HelpMenu", NULL, N_("_Help"), NULL, NULL, NULL },

	{ "OpenLog", GTK_STOCK_OPEN, N_("_Open..."), "<control>O", N_("Open a log from file"), 
	  G_CALLBACK (LoadLogMenu) },
	{ "Properties", GTK_STOCK_PROPERTIES,  N_("_Properties"), "<control>P", N_("Show Log Properties"), 
	  G_CALLBACK (LogInfo) },
	{ "CloseLog", GTK_STOCK_CLOSE, N_("Close"), "<control>W", N_("Close this log"), 
	  G_CALLBACK (CloseLogMenu) },
	{ "Quit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q", N_("Quit the log viewer"), 
	  G_CALLBACK (gtk_main_quit) },

	{ "Copy", GTK_STOCK_COPY, N_("Copy"), "<control>C", N_("Copy the selection"),
	  G_CALLBACK (logview_copy) },
	{ "SelectAll", NULL, N_("Select All"), "<Control>A", N_("Select the entire log"),
	  G_CALLBACK (logview_select_all) },
	{ "Search", GTK_STOCK_FIND, N_("_Find..."), "<control>F", N_("Find pattern in logs"),
	  G_CALLBACK (logview_search) },

	{"CollapseAll", NULL, N_("Collapse _All"), NULL, N_("Collapse all the rows"),
	 G_CALLBACK (toggle_collapse_rows) },

       	{ "HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1", N_("Open the help contents for the log viewer"), 
	  G_CALLBACK (logview_help) },
	{ "AboutAction", GTK_STOCK_ABOUT, N_("_About"), NULL, N_("Show the about dialog for the log viewer"), 
	  G_CALLBACK (AboutShowWindow) },

};

static GtkToggleActionEntry toggle_entries[] = {
	{ "MonitorLogs", NULL, N_("_Monitor"), "<control>M", N_("Monitor Current Log"),
	  G_CALLBACK (toggle_monitor) },
	{"ShowCalendar", NULL,  N_("Ca_lendar"), "<control>L", N_("Show Calendar Log"), 
	 G_CALLBACK (toggle_calendar) },
	{"ShowDetails", NULL,  N_("_Entry Detail"), "<control>E", N_("Show Entry Detail"), 
	 G_CALLBACK (toggle_zoom) },
};

static const char *ui_description = 
	"<ui>"
	"	<menubar name='LogviewMenu'>"
	"		<menu action='FileMenu'>"
	"			<menuitem action='OpenLog'/>"
	"			<separator/>"
	"			<menuitem action='MonitorLogs'/>"
	"			<separator/>"
	"			<menuitem action='Properties'/>"
	"			<separator/>"
	"			<menuitem action='CloseLog'/>"
	"			<menuitem action='Quit'/>"
	"		</menu>"
	"		<menu action='EditMenu'>"
	"                       <menuitem action='Copy'/>"
	"                       <menuitem action='SelectAll'/>"
	"                       <separator/>"
	"			<menuitem action='Search'/>"
	"		</menu>"	
	"		<menu action='ViewMenu'>"
	"			<menuitem action='ShowCalendar'/>"
	"			<menuitem action='ShowDetails'/>"
	"                       <separator/>"
	"			<menuitem action='CollapseAll'/>"
	"		</menu>"
	"		<menu action='HelpMenu'>"
	"			<menuitem action='HelpContents'/>"
	"			<menuitem action='AboutAction'/>"
	"		</menu>"
	"	</menubar>"
	"</ui>";

/*
 *       ----------------
 *       Global variables
 *       ----------------
 */
	
GList *regexp_db = NULL, *descript_db = NULL, *actions_db = NULL;
ConfigData *cfg = NULL;
gchar *file_to_open;

GOptionContext *context;
GOptionEntry options[] = {
	{ NULL }
};

void
logview_save_prefs (LogviewWindow *logview)
{
  GList *list;
  Log *log;

	prefs_free_loglist(user_prefs);
  for (list = logview->logs; list != NULL; list = g_list_next (list)) {
      log = list->data;
      user_prefs->logs = g_slist_append (user_prefs->logs, log->name);
	}
  user_prefs->logfile = logview->curlog->name;

	if (restoration_complete)
		prefs_save (client, user_prefs);
}

/* ----------------------------------------------------------------------
   NAME:          destroy
   DESCRIPTION:   Exit program.
   ---------------------------------------------------------------------- */

void
destroy (GObject *object, gpointer data)
{
   LogviewWindow *window = data;
   logview_windows = g_slist_remove (logview_windows, window);
   if (window->curlog->monitored)
	   monitor_stop (window);
   if (logview_windows == NULL) {
	   if (window->curlog && !(window->curlog->display_name))
		   user_prefs->logfile = window->curlog->name;
	   else
		   user_prefs->logfile = NULL;
	   prefs_save (client, user_prefs);
	   gtk_main_quit ();
   }
}

static gint
save_session (GnomeClient *gnome_client, gint phase,
              GnomeRestartStyle save_style, gint shutdown,
              GnomeInteractStyle interact_style, gint fast,
              gpointer client_data)
{
   gchar **argv;
   gint numlogs, i=0;
   GSList *list;

   numlogs = logview_count_logs ();

   argv = g_malloc0 (sizeof (gchar *) * numlogs+1);
   argv[0] = g_get_prgname();

   for (list = logview_windows; list != NULL; list = g_slist_next (list)) {
	   LogviewWindow *w = list->data;
	   if (w->curlog) {
		   argv[i++] = g_strdup_printf ("%s", w->curlog->name);
	   }
   }
   
   gnome_client_set_clone_command (gnome_client, numlogs+1, argv);
   gnome_client_set_restart_command (gnome_client, numlogs+1, argv);

   g_free (argv);

   return TRUE;

}

static gint
die (GnomeClient *gnome_client, gpointer client_data)
{
    gtk_main_quit ();
}

void
logview_select_log (LogviewWindow *logview, Log *log)
{
   logview->curlog = log;
   logview_menus_set_state (logview);
   log_repaint (logview);
	 logview_save_prefs (logview);
} 

void
logview_add_log (LogviewWindow *logview, Log *log)
{
  logview->logs = g_list_append (logview->logs, log);
  loglist_add_log (logview, log);
  log->monitored = FALSE;
  log->first_time = TRUE;
} 

void
logview_add_log_from_name (LogviewWindow *logview, gchar *file)
{
	Log *log;
	log = OpenLogFile (file);
	if (log != NULL) {
		  logview_add_log (logview, log);
	}
}

void
logview_add_logs_from_names (LogviewWindow *logview, GSList *lognames)
{
	GSList *list;
	Log *log;
	
	g_print("OPENING SAVED LOGS : ");
	for (list = lognames; list != NULL; list = g_slist_next (list)) {
		g_printf("%s ", list->data);
		if (logview_find_log_from_name (logview, list->data) == NULL) {
			log = OpenLogFile (list->data);
			if (log != NULL) {
				logview_add_log (logview, log);
			}
		}
	}
	g_print(".\n");
}

logview_remove_log (LogviewWindow *logview, Log *log)
{
	CloseLog (log);
	logview->logs = g_list_remove (logview->logs, log);
}

GOptionContext *
logview_init_options ()
{
	GOptionContext *context;
	
	context = g_option_context_new (" - Browse and monitor logs");
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_set_help_enabled (context, TRUE);
	g_option_context_set_ignore_unknown_options (context, TRUE);
	return context;
}
	
/* ----------------------------------------------------------------------
   NAME:          main
   DESCRIPTION:   Program entry point.
   ---------------------------------------------------------------------- */

int
main (int argc, char *argv[])
{
   GnomeClient *gnome_client;
   GError *error;
   GnomeProgram *program;
	 LogviewWindow *logview;
   int i;

   bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
   bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
   textdomain(GETTEXT_PACKAGE);

   QueueErrMessages (TRUE);

   /*  Initialize gnome & gtk */
   program = gnome_program_init ("gnome-system-log",VERSION, LIBGNOMEUI_MODULE, argc, argv,
				 GNOME_PARAM_APP_DATADIR, DATADIR, 
				 NULL);

   gconf_init (argc, argv, NULL);
   client = gconf_client_get_default ();
   gnome_vfs_init ();

   gtk_window_set_default_icon_name ("logviewer");
   cfg = CreateConfig();
   open_databases ();
   user_prefs = prefs_load (client);
   
   context = logview_init_options ();
   g_option_context_parse (context, &argc, &argv, &error);

   /* Open regular logs and add each log passed as a parameter */

	 logview = LOGVIEW_WINDOW(logview_create_window ());
	 logview_add_logs_from_names (logview, user_prefs->logs);
	 loglist_select_log_from_name (logview, user_prefs->logfile);
   if (argc > 1) {
	   for (i=1; i<argc; i++) {
		   file_to_open = argv[i];
			 logview_add_log_from_name (logview, file_to_open);
	   }
   }
	 gtk_widget_show (GTK_WIDGET(logview));
	 restoration_complete = TRUE;

   gnome_client = gnome_master_client ();

   QueueErrMessages (FALSE);
   ShowQueuedErrMessages ();
   
   g_signal_connect (gnome_client, "save_yourself",
					 G_CALLBACK (save_session), NULL);
   g_signal_connect (gnome_client, "die", G_CALLBACK (die), NULL);

   /*  Loop application */
   gtk_main ();

   return 0;
}


/* ----------------------------------------------------------------------
   NAME:        logview_create_window
   DESCRIPTION: Main initialization routine.
   ---------------------------------------------------------------------- */

static GtkWidget *
logview_create_window ()
{
   LogviewWindow *logviewwindow;
   GtkWidget *window;

   regexp_db = NULL;

   /*  Display main window */
   window = g_object_new (LOGVIEW_TYPE_WINDOW, NULL);
   logviewwindow = LOGVIEW_WINDOW (window);

   logviewwindow->loginfovisible = FALSE;
   logviewwindow->zoom_visible = FALSE;
   logviewwindow->zoom_scrolled_window = NULL;
   logviewwindow->zoom_dialog = NULL;
   logviewwindow->logs = NULL;
   logviewwindow->clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (window),
							     GDK_SELECTION_CLIPBOARD);

   /* FIXME : we need to listen to this key, not just read it. */
   gtk_ui_manager_set_add_tearoffs (logviewwindow->ui_manager, 
				    gconf_client_get_bool 
				    (client, "/desktop/gnome/interface/menus_have_tearoff", NULL));

   g_signal_connect (GTK_OBJECT (window), "destroy",
		     G_CALLBACK (destroy), logviewwindow);

   logview_windows = g_slist_prepend (logview_windows, window);

   return window;
}

Log *
logview_find_log_from_name (LogviewWindow *logview, gchar *name)
{
  GList *list;
  Log *log;
  for (list = logview->logs; list != NULL; list = g_list_next (list)) {
    log = list->data;
    if (g_ascii_strncasecmp (log->name, name, 255) == 0) {
      return log;
    }
  }
  return NULL;
}

static void
loglist_selection_changed (GtkTreeSelection *selection, LogviewWindow *logview)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *name;
  Log *log;
  
  if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		/* there is no selected log right now */
		logview->curlog = NULL;
		logview_set_window_title (logview);
		logview_menus_set_state (logview);
		log_repaint (logview);
    return;
	}
  
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (logview->treeview));
  gtk_tree_model_get (model, &iter, 0, &name, -1);
  log = logview_find_log_from_name (logview, name);
  if (log != logview->curlog)
      logview_select_log (logview, log);
}

GtkTreePath *
loglist_find_logname (LogviewWindow *logview, gchar *logname)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
	gchar *name;

	model =  gtk_tree_view_get_model (GTK_TREE_VIEW(logview->treeview));
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return NULL;

	do {
		gtk_tree_model_get (model, &iter, 0, &name, -1);
		if (g_ascii_strncasecmp (logname, name, 255) == 0) {
			path = gtk_tree_model_get_path (model, &iter);
			return path;
		}
	} while (gtk_tree_model_iter_next (model, &iter));

	return NULL;
}

static void
loglist_select_log_from_name (LogviewWindow *logview, gchar *logname)
{
	GtkTreePath *path;
	path = loglist_find_logname (logview, logname);
	if (path) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (logview->treeview));
		gtk_tree_selection_select_path (selection, path);
	}
}

static void 
loglist_add_log (LogviewWindow *logview, Log *log)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (logview->treeview));
    
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, log->name, -1);

	if (restoration_complete) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (logview->treeview));
		gtk_tree_selection_select_iter (selection, &iter);
	}
}

static void
loglist_remove_selected_log (LogviewWindow *logview)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	gchar *name;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (logview->treeview));
	gtk_tree_selection_get_selected (selection, &model, &iter);

	gtk_tree_model_get (model, &iter, 0, &name, -1);
	g_print("Removing selection : %s\n", name);
	
	if (name) {
		Log *log;
		
		log = logview_find_log_from_name (logview, name);
		logview_remove_log (logview, log);

		if (gtk_list_store_remove (GTK_LIST_STORE (model), &iter))
			gtk_tree_selection_select_iter (selection, &iter);
	}
}

GtkWidget *
loglist_create (LogviewWindow *window)
{
  GtkWidget *label;
  GtkWidget *scrolled;
  GtkWidget *vbox;
  GtkWidget *treeview;
  GtkListStore *model;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkCellRenderer *cell;
    
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (vbox), 3);
  
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
				       GTK_SHADOW_ETCHED_IN);
  
  model = gtk_list_store_new (1, G_TYPE_STRING);
  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
  g_object_unref (G_OBJECT (model));
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (loglist_selection_changed), window);
  
  cell = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("words", cell, "text", 0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  
  gtk_container_add (GTK_CONTAINER (scrolled), treeview);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);

  window->treeview = treeview;
  gtk_widget_show (vbox);
  return vbox;
}


/* ----------------------------------------------------------------------
   NAME:        CreateMainWin
   DESCRIPTION: Creates the main window.
   ---------------------------------------------------------------------- */

static void
CreateMainWin (LogviewWindow *window)
{
   GtkWidget *vbox;
   GtkTreeStore *tree_store;
   GtkTreeSelection *selection;
   GtkTreeViewColumn *column;
   GtkCellRenderer *renderer;
   gint i;
   GtkActionGroup *action_group;
   GtkAccelGroup *accel_group;
   GError *error = NULL;
   GtkWidget *menubar;
   GtkWidget *loglist;
   GtkWidget *hpaned;
   const gchar *column_titles[] = { N_("Date"), N_("Host Name"),
                                    N_("Process"), N_("Message"), NULL };

   gtk_window_set_default_size (GTK_WINDOW (window), user_prefs->width, user_prefs->height);

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (window), vbox);

   /* Create menus */
   action_group = gtk_action_group_new ("LogviewMenuActions");
   gtk_action_group_set_translation_domain (action_group, NULL);
   gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), window);
   gtk_action_group_add_toggle_actions(action_group, toggle_entries, G_N_ELEMENTS (toggle_entries), window);

   window->ui_manager = gtk_ui_manager_new ();

   gtk_ui_manager_insert_action_group (window->ui_manager, action_group, 0);
   
   accel_group = gtk_ui_manager_get_accel_group (window->ui_manager);
   gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
   
   if (!gtk_ui_manager_add_ui_from_string (window->ui_manager, ui_description, -1, &error)) {
	   g_message ("Building menus failed: %s", error->message);
	   g_error_free (error);
	   exit (1);
   }
   
   menubar = gtk_ui_manager_get_widget (window->ui_manager, "/LogviewMenu");
   gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

   /* panes */
   hpaned = gtk_hpaned_new ();
   gtk_box_pack_start (GTK_BOX (vbox), hpaned, TRUE, TRUE, 0);

   /* First pane : list of logs */
   window->loglist = loglist_create (window);
   gtk_paned_pack1 (GTK_PANED (hpaned), GTK_WIDGET (window->loglist), FALSE, FALSE);

   /* Second pane : logs themselves */
   window->main_view = gtk_frame_new (NULL);
   gtk_paned_pack2 (GTK_PANED (hpaned), GTK_WIDGET (window->main_view), TRUE, TRUE);

   /* Scrolled windows for the main view and monitor view */
   window->log_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (window->log_scrolled_window),
               GTK_POLICY_AUTOMATIC,
               GTK_POLICY_AUTOMATIC);
   window->mon_scrolled_window = logview_create_monitor_widget (window);

   /* We ref the two scrolled_windows so they are not destroyed when we remove one from
    * the main_view to put the other */
   g_object_ref (window->log_scrolled_window);
   g_object_ref (window->mon_scrolled_window);

   gtk_container_add (GTK_CONTAINER (window->main_view), window->log_scrolled_window);

   /* Main Tree View */
   tree_store = gtk_tree_store_new (4, G_TYPE_STRING, G_TYPE_STRING,
				    G_TYPE_STRING, G_TYPE_STRING);

   window->view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (tree_store));
   gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (window->view), TRUE);
   g_object_unref (G_OBJECT (tree_store)); 
   
   for (i = 0; column_titles[i]; i++) {
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_(column_titles[i]),
                    renderer, "text", i, NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE); 
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (window->view), column);
   }

   gtk_container_add (GTK_CONTAINER (window->log_scrolled_window), GTK_WIDGET (window->view));
   gtk_widget_show_all (window->log_scrolled_window);

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->view));
   gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

   /* Add signal handlers */
   g_signal_connect (G_OBJECT (selection), "changed",
                     G_CALLBACK (handle_selection_changed_cb), window);
   g_signal_connect (G_OBJECT (window->view), "row_activated",
                     G_CALLBACK (handle_row_activation_cb), window);
   g_signal_connect (G_OBJECT (window), "configure_event",
		     G_CALLBACK (window_size_changed_cb), window);

   window->find_bar = gtk_toolbar_new ();
   logview_findbar_populate (window, window->find_bar);
   gtk_toolbar_set_style (GTK_TOOLBAR (window->find_bar), GTK_TOOLBAR_BOTH_HORIZ);
   gtk_box_pack_start (GTK_BOX (vbox), window->find_bar, FALSE, FALSE, 0);
   gtk_widget_show (window->find_bar);

   /* Status area at bottom */
   window->statusbar = gtk_statusbar_new ();
   gtk_box_pack_start (GTK_BOX (vbox), window->statusbar, FALSE, FALSE, 0);

	 gtk_widget_show_all (vbox);
   gtk_widget_hide (window->find_bar);
}

/* ----------------------------------------------------------------------
   NAME:          CloseLogMenu
   DESCRIPTION:   Close the current log.
   ---------------------------------------------------------------------- */

static void
CloseLogMenu (GtkAction *action, GtkWidget *callback_data)
{
   LogviewWindow *window = LOGVIEW_WINDOW (callback_data);

   g_return_if_fail (window->curlog);

   if (window->curlog->monitored) {
	   GtkAction *action = gtk_ui_manager_get_action (window->ui_manager, "/LogviewMenu/FileMenu/MonitorLogs");
	   gtk_action_activate (action);
   }

   if (window->calendar_visible) {
	   GtkAction *action = gtk_ui_manager_get_action (window->ui_manager, "/LogviewMenu/ViewMenu/ShowCalendar");
	   gtk_action_activate (action);
   }

   if (window->zoom_visible) {
	   GtkAction *action = gtk_ui_manager_get_action (window->ui_manager, "/LogviewMenu/ViewMenu/ShowDetails");
	   gtk_action_activate (action);
   }

   gtk_widget_hide (window->find_bar);
	 loglist_remove_selected_log (window);
}

/* ----------------------------------------------------------------------
   NAME:          FileSelectResponse
   DESCRIPTION:   User selected a file.
   ---------------------------------------------------------------------- */

static void
FileSelectResponse (GtkWidget * chooser, gint response, gpointer data)
{
   char *f;
   LogviewWindow *logview = data;

   gtk_widget_hide (GTK_WIDGET (chooser));
   if (response != GTK_RESPONSE_OK)
	   return;
   
   f = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

   if (f != NULL) {
	   /* Check if the log is not already opened */
	   GList *list;
		 Log *log;
	   for (list = logview->logs; list != NULL; list = g_list_next (list)) {
		   log = list->data;
		   if (g_ascii_strncasecmp (log->name, f, 255) == 0) {
				 loglist_select_log_from_name (logview, log->name);
			   return;
		   }
	   }

	   Log *tl;
	   if ((tl = OpenLogFile (f)) != NULL) {
	     logview_add_log (logview, tl);
	     if (logview->calendar_visible)
	       init_calendar_data(logview);
	   }
   }
   
   g_free (f);

}

/* ----------------------------------------------------------------------
   NAME:          LoadLogMenu
   DESCRIPTION:   Open a new log defined by the user.
   ---------------------------------------------------------------------- */

static void
LoadLogMenu (GtkAction *action, GtkWidget *parent_window)
{
   static GtkWidget *chooser = NULL;
   
   if (chooser == NULL) {
	   chooser = gtk_file_chooser_dialog_new (_("Open new logfile"),
						  GTK_WINDOW (parent_window),
						  GTK_FILE_CHOOSER_ACTION_OPEN,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						  NULL);
	   gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);
	   gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);
	   g_signal_connect (G_OBJECT (chooser), "response",
			     G_CALLBACK (FileSelectResponse), parent_window);
	   g_signal_connect (G_OBJECT (chooser), "destroy",
			     G_CALLBACK (gtk_widget_destroyed), &chooser);
	   if (user_prefs->logfile != NULL)
		   gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), 
						  user_prefs->logfile);
   }

   gtk_window_present (GTK_WINDOW (chooser));
}

GtkWidget *logview_create_monitor_widget (LogviewWindow *window)
{
	GtkWidget *clist_view, *swin;
	GtkListStore *clist;   
	GtkCellRenderer *clist_cellrenderer;
	GtkTreeViewColumn *clist_column;

	clist = gtk_list_store_new (1, G_TYPE_STRING);
	clist_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (clist));
	g_object_unref (clist);
	clist_cellrenderer = gtk_cell_renderer_text_new ();
	clist_column = gtk_tree_view_column_new_with_attributes
		(NULL, clist_cellrenderer, "markup", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (clist_view), clist_column);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (clist_view), FALSE);
	swin = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (swin), GTK_WIDGET (clist_view));
	gtk_tree_selection_set_mode
		( (GtkTreeSelection *)gtk_tree_view_get_selection
		  (GTK_TREE_VIEW (clist_view)),
		  GTK_SELECTION_MULTIPLE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show_all (swin);

	window->mon_list_view = (GtkWidget *) clist_view;

	return swin;
}

/* ----------------------------------------------------------------------
   NAME:          open_databases
   DESCRIPTION:   Try to locate regexp and descript databases and load
   	          them.
   ---------------------------------------------------------------------- */

static void
open_databases (void)
{
	char full_name[1024];
	gboolean found;

	/* Find regexp DB -----------------------------------------------------  */
	found = FALSE;
	if (cfg->regexp_db_path != NULL) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/gnome-system-log-regexp.db", cfg->regexp_db_path);
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
	if (access (full_name, R_OK) == 0) {
		found = TRUE;
		read_actions_db (full_name, &actions_db);
	}

	if ( ! found && cfg->action_db_path != NULL) {
		g_snprintf (full_name, sizeof (full_name),
			    "%s/gnome-system-log-actions.db", cfg->action_db_path);
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

}

static void 
toggle_calendar (GtkAction *action, GtkWidget *callback_data)
{
    LogviewWindow *window = LOGVIEW_WINDOW (callback_data);
    if (window->calendar_visible) {
	window->calendar_visible = FALSE;
	gtk_widget_hide (window->calendar_dialog);
    }
    else
	CalendarMenu (window);
}

static void
toggle_zoom (GtkAction *action, GtkWidget *callback_data)
{
    LogviewWindow *window = LOGVIEW_WINDOW (callback_data);
    if (window->zoom_visible) {
	    window->zoom_visible = FALSE;
	    gtk_widget_hide (window->zoom_dialog);
    } else
	    create_zoom_view (window);
}

static void 
toggle_collapse_rows (GtkAction *action, GtkWidget *callback_data)
{
    LogviewWindow *window = LOGVIEW_WINDOW (callback_data);
    gtk_tree_view_collapse_all (GTK_TREE_VIEW (window->view));
}

static void
toggle_monitor (GtkAction *action, GtkWidget *callback_data)
{
    LogviewWindow *window = LOGVIEW_WINDOW (callback_data);
    g_return_if_fail (window->curlog);
    if (!window->curlog->display_name) {
	    if (window->curlog->monitored) {
		    gtk_container_remove (GTK_CONTAINER (window->main_view), window->mon_scrolled_window);
		    gtk_container_add (GTK_CONTAINER (window->main_view), window->log_scrolled_window);
		    monitor_stop (window);
		    window->curlog->monitored = FALSE;
	    } else {
		    gtk_container_remove (GTK_CONTAINER(window->main_view), window->log_scrolled_window);
		    gtk_container_add (GTK_CONTAINER(window->main_view), window->mon_scrolled_window);
		    monitor_start (window);
		    window->curlog->monitored = TRUE;
	    }
	    logview_set_window_title (window);
	    logview_menus_set_state (window);
    }
}

static void
logview_search (GtkAction *action,GtkWidget *callback_data)
{
	LogviewWindow *window = LOGVIEW_WINDOW (callback_data);

	gtk_widget_show (window->find_bar);
	gtk_widget_grab_focus (window->find_entry);
}

static void
logview_copy (GtkAction *action, GtkWidget *callback_data)
{
	LogviewWindow *window = LOGVIEW_WINDOW (callback_data);
	LogLine *line;
	int nline, i;
	gchar *text, **lines;

	if (window->curlog->selected_line_first > -1 && window->curlog->selected_line_last > -1) {
		nline = window->curlog->selected_line_last - window->curlog->selected_line_first + 1;
		lines = g_new0(gchar *, (nline + 1));
		for (i=0; i<=nline; i++) {
			if (window->curlog->selected_line_first + i < window->curlog->total_lines) {
				line = (window->curlog->lines)[window->curlog->selected_line_first + i];
				if (line && line->hostname && line->process && line->message)
					lines[i] = g_strdup_printf ("%s %s %s", line->hostname, line->process, line->message);
			}
		}
		lines[nline] = NULL;
		text = g_strjoinv ("\n", lines);
		gtk_clipboard_set_text (window->clipboard, LocaleToUTF8(text), -1);
		g_free (text);
		g_strfreev (lines);
	}
}

static void
logview_select_all (GtkAction *action, GtkWidget *callback_data)
{
	GtkTreeSelection *selection;
	LogviewWindow *window = LOGVIEW_WINDOW (callback_data);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->view));
	gtk_tree_selection_select_all (selection);
}

static void
logview_menu_item_set_state (LogviewWindow *logviewwindow, char *path, gboolean state)
{
	GtkAction *action;
	g_return_if_fail (path);
	action = gtk_ui_manager_get_action (logviewwindow->ui_manager, path);
	gtk_action_set_sensitive (action, state);
}

static void
logview_menus_set_state (LogviewWindow *window)
{
	if (window->curlog->monitored) {
		logview_menu_item_set_state (window, "/LogviewMenu/EditMenu/Copy", FALSE);
		logview_menu_item_set_state (window, "/LogviewMenu/EditMenu/Search", FALSE);
		logview_menu_item_set_state (window, "/LogviewMenu/ViewMenu/ShowCalendar", FALSE);
		logview_menu_item_set_state (window, "/LogviewMenu/ViewMenu/ShowDetails", FALSE); 
		logview_menu_item_set_state (window, "/LogviewMenu/ViewMenu/CollapseAll", FALSE);
		logview_menu_item_set_state (window, "/LogviewMenu/EditMenu/SelectAll", FALSE);
	} else {
		if (window->curlog) {
			if (window->curlog->display_name)
				logview_menu_item_set_state (window, "/LogviewMenu/FileMenu/MonitorLogs", FALSE);
			else
				logview_menu_item_set_state (window, "/LogviewMenu/FileMenu/MonitorLogs", TRUE);
		} else
			logview_menu_item_set_state (window, "/LogviewMenu/FileMenu/MonitorLogs", FALSE);
		
		logview_menu_item_set_state (window, "/LogviewMenu/FileMenu/Properties", (window->curlog != NULL));
		logview_menu_item_set_state (window, "/LogviewMenu/FileMenu/CloseLog", (window->curlog != NULL));
		logview_menu_item_set_state (window, "/LogviewMenu/ViewMenu/ShowCalendar", (window->curlog != NULL));
		logview_menu_item_set_state (window, "/LogviewMenu/ViewMenu/ShowDetails", (window->curlog != NULL));
		logview_menu_item_set_state (window, "/LogviewMenu/ViewMenu/CollapseAll", (window->curlog != NULL));
		logview_menu_item_set_state (window, "/LogviewMenu/EditMenu/Search", (window->curlog != NULL));
		logview_menu_item_set_state (window, "/LogviewMenu/EditMenu/Copy", (window->curlog != NULL));
		logview_menu_item_set_state (window, "/LogviewMenu/EditMenu/SelectAll", (window->curlog != NULL));
	}
}

static int
logview_count_logs (void)
{
	GSList *list;
	LogviewWindow *window;
	int numlogs = 0;
	for (list = logview_windows; list != NULL; list = g_slist_next (list)) {
		window = list->data;
		if (window->curlog != NULL)
			numlogs ++;
	}
	return numlogs;
}

static void
logview_help (GtkAction *action, GtkWidget *parent_window)
{
        GError *error = NULL;                                                                                
        gnome_help_display_desktop_on_screen (NULL, "gnome-system-log", "gnome-system-log", NULL,
					      gtk_widget_get_screen (GTK_WIDGET(parent_window)), &error);
	if (error) {
		ShowErrMessage (GTK_WIDGET(parent_window), _("There was an error displaying help."), error->message);
		g_error_free (error);
	}
}

void
logview_set_window_title (LogviewWindow *window)
{
	gchar *window_title;
	gchar *logname;

	if ((window->curlog != NULL) && (window->curlog->name != NULL)) {
		if (window->curlog->display_name != NULL)
			logname = window->curlog->display_name;
		else
			logname = window->curlog->name;
		
		if (window->curlog->monitored) 
			window_title = g_strdup_printf (_("%s (monitored) - %s"), logname, APP_NAME);
		else
			window_title = g_strdup_printf ("%s - %s", logname, APP_NAME);
	}
	else
		window_title = g_strdup_printf (APP_NAME);
	gtk_window_set_title (GTK_WINDOW (window), window_title);
	g_free (window_title);
}

static void
logview_window_finalize (GObject *object)
{
        LogviewWindow *window = (LogviewWindow *) object;

	g_object_unref (window->ui_manager);
        parent_class->finalize (object);
}

static void
logview_window_class_init (LogviewWindowClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	object_class->finalize = logview_window_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

GType
logview_window_get_type (void)
{
	static GType object_type = 0;
	
	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (LogviewWindowClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) logview_window_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (LogviewWindow),
			0,              /* n_preallocs */
			(GInstanceInitFunc) CreateMainWin
		};

		object_type = g_type_register_static (GTK_TYPE_WINDOW, "LogviewWindow", &object_info, 0);
	}

	return object_type;
}

gboolean 
window_size_changed_cb (GtkWidget *widget, GdkEventConfigure *event, 
				 gpointer data)
{
	LogviewWindow *window = data;

	prefs_store_size (GTK_WIDGET(window), user_prefs);
	return FALSE;
}
