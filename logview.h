
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

#ifndef __LOGVIEW_H__
#define __LOGVIEW_H__

#include <time.h>
#include "gtk/gtk.h"
#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs.h>

#define LINES_P_PAGE             10
#define NUM_PAGES                5 
#define MAX_WIDTH                240
#define MAX_HOSTNAME_WIDTH       257	/* Need authoritative answer on this value. */
#define MAX_PROC_WIDTH           60
#define NUM_LOGS                 2
#define R_BUF_SIZE               1024	/* Size of read buffer */
#define MAX_NUM_LOGS             10

#define LOG_CANVAS_H             400
#define LOG_CANVAS_W             600

#define APP_NAME                 _("System Log Viewer")

/* the following is a simple hashing function that will convert a
 * given date into an integer value that can be used as a key for
 * the date_headers hash table */
#define DATEHASH(month, day)     GUINT_TO_POINTER (month * 31 + day)

/*
 *    ,----------.
 *    | Typedefs |
 *    `----------'
 */

/* for the search results */
enum {
	LOGVIEW_WINDOW_OUTPUT_WINDOW_NONE,
	LOGVIEW_WINDOW_OUTPUT_WINDOW_SEARCH,
};

typedef struct
{
  /* Paths ----------------------------------------------------- */
  char *regexp_db_path, *descript_db_path, *action_db_path;

} ConfigData;

struct __datemark
{
  time_t time;		/* date           */
  struct tm fulldate;
  long offset;		/* offset in file */
  char year;		/* to correct for logfiles with many years */
  long ln;		/* Line # from begining of file */
  struct __datemark *next, *prev;
};

typedef struct __datemark DateMark;

typedef struct
{
  time_t startdate, enddate;
  time_t mtime;
  long numlines;
  long size;
  DateMark *firstmark, *lastmark;
}
LogStats;

typedef struct {
  GtkWidget *calendar;
  DateMark *curmonthmark;
} CalendarData;

typedef struct
{
  char tag[50];
  char *regexp;
  char *description;
  int level;
} Description;

typedef struct
{
  char *regexp;
  GList *matching;
} ProcessDB;

typedef struct
{
  char *text;
  char tag[50];
} DescriptionEntry;
  
typedef struct
{
  char tag[50];
  char *log_name;
  char *process;
  char *message;
  char *description;
  char *action;
} Action;

typedef struct
{
  char message[MAX_WIDTH];
  char process[MAX_PROC_WIDTH];
  char hostname[MAX_HOSTNAME_WIDTH];
  signed char month;
  signed char date;
  signed char hour;
  signed char min;
  signed char sec;
  Description *description;
} LogLine;

typedef void (*MonActions)();

typedef struct
{
	/* FIXME: This should perhaps be a glist of logfiles to
	** open on startup. Also one should be able to set the value in
	** the prefs dialog
	*/
	gchar *logfile;
} UserPrefsStruct;

typedef struct
{
	GnomeVFSHandle *handle;
	GnomeVFSFileSize filesize;

	DateMark *curmark;
	char name[255];
	CalendarData *caldata;
	LogStats lstats;
	gint current_line_no; /* indicates the line that is selected */
	gint total_lines; /* no of lines in the file */
	LogLine **lines; /* actual lines */
	gboolean first_time;
	GtkTreePath *current_path;
	GtkTreePath *expand_paths[32];
	GHashTable *date_headers; /* stores paths to date headers */
	
	/* Monitor info */
	MonActions alert;
	GnomeVFSFileOffset mon_offset;
	GnomeVFSMonitorHandle *mon_handle;
}
Log;


/*
 *    ,---------------------.
 *    | Function Prototypes |
 *    `---------------------'
 */

ConfigData *CreateConfig(void);
int IsLeapYear (int year);
void SetDefaultUserPrefs(UserPrefsStruct *prefs, GConfClient *client);
int exec_action_in_db (Log *log, LogLine *line, GList *db);

#define LOGVIEW_TYPE_WINDOW		  (logview_window_get_type ())
#define LOGVIEW_WINDOW(obj)		  (GTK_CHECK_CAST ((obj), LOGVIEW_TYPE_WINDOW, LogviewWindow))
#define LOGVIEW_WINDOW_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_WINDOW, LogviewWindowClass))
#define LOGVIEW_IS_WINDOW(obj)	  (GTK_CHECK_TYPE ((obj), LOGVIEW_TYPE_WINDOW))
#define LOGVIEW_IS_WINDOW_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), LOGVIEW_TYPE_WINDOW))
#define LOGVIEW_WINDOW_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), LOGVIEW_TYPE_WINDOW, LogviewWindowClass))

typedef struct _LogviewWindow LogviewWindow;
typedef struct _LogviewWindowClass LogviewWindowClass;

struct _LogviewWindow {
	GtkWindow parent_instance;

	GConfClient *client;

	gchar *program_name;

	GtkWidget *view;
	GtkWidget *mon_list_view;
	GtkWidget *main_view;
	GtkWidget *log_scrolled_window;
	GtkWidget *mon_scrolled_window;
		
	GtkWidget *statusbar;
	GtkUIManager *ui_manager;

	GtkWidget *zoom_dialog;
	GtkWidget *zoom_scrolled_window;
	GtkListStore *zoom_store;
	gboolean zoom_visible;
	
	GtkWidget *info_dialog;
	gboolean loginfovisible;

	GtkWidget *calendar_dialog;
	GtkWidget *calendar;
	gboolean calendar_visible;

	GtkWidget *find_bar;
	GtkWidget *find_entry;
	GtkToolItem *find_next;
	GtkToolItem *find_prev;
	gchar *find_string;

	int numlogs, curlognum;
	Log *curlog;	

	gboolean monitored;
	gint timer_tag;
};

struct _LogviewWindowClass {
	GtkWindowClass parent_class;
};

GType logview_window_get_type (void);
GtkWidget *logview_window_new (void);
void logview_set_window_title (LogviewWindow *window);

#endif /* __LOGVIEW_H__ */
