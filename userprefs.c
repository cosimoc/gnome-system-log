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


#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gconf/gconf-client.h>
#include "userprefs.h"

#define LOG_CANVAS_H             400
#define LOG_CANVAS_W             600

#define GCONF_WIDTH_KEY  "/apps/gnome-system-log/width"
#define GCONF_HEIGHT_KEY "/apps/gnome-system-log/height"
#define GCONF_LOGFILE "/apps/gnome-system-log/logfile"

void
prefs_create_defaults (UserPrefsStruct *prefs)
{
	int i;
	gchar *logfiles[] = {"/etc/syslog.conf", "/var/adm/messages",
			     "/var/log/messages","/var/log/sys.log"};

	/* For first time running, try parsing various logfiles */

	for (i=0; i<4; i++) {
		if (isLogFile (logfiles[i], FALSE)) {
			prefs->logfile = g_strdup (logfiles[i]);
			break;
		}
	}
}

UserPrefsStruct *
prefs_load (GConfClient *client)
{
	gchar *logfile = NULL;
	int width, height;
	UserPrefsStruct *prefs;

	prefs = g_new0 (UserPrefsStruct, 1);
	
	logfile = gconf_client_get_string (client, GCONF_LOGFILE, NULL);
	if (logfile != NULL && strcmp (logfile, "") && isLogFile(logfile, FALSE)) {
		prefs->logfile = g_strdup (logfile);
		g_free (logfile);
	}
	else
		prefs_create_defaults(prefs);

	width = gconf_client_get_int (client, GCONF_WIDTH_KEY, NULL);
	height = gconf_client_get_int (client, GCONF_HEIGHT_KEY, NULL);
	prefs->width = (width == 0 ? LOG_CANVAS_W : width);
	prefs->height = (height == 0 ? LOG_CANVAS_H : height);

	return prefs;
}

void
prefs_save (GConfClient *client, UserPrefsStruct *prefs)
{
	if (gconf_client_key_is_writable (client, GCONF_LOGFILE, NULL) &&
	    prefs->logfile != NULL)
		gconf_client_set_string (client, GCONF_LOGFILE, prefs->logfile, NULL);
	if (prefs->width > 0 && prefs->height > 0) {
		if (gconf_client_key_is_writable (client, GCONF_WIDTH_KEY, NULL))
			gconf_client_set_int (client, GCONF_WIDTH_KEY, prefs->width, NULL);
		if (gconf_client_key_is_writable (client, GCONF_HEIGHT_KEY, NULL))
			gconf_client_set_int (client, GCONF_HEIGHT_KEY, prefs->height, NULL);
	}
}

void
prefs_store_size (GtkWidget *window, UserPrefsStruct *prefs)
{
	int width, height;
	gtk_window_get_size (GTK_WINDOW(window), &width, &height);
	/* FIXME : we should check the state of the window, maximized or not */
	if (width > 0 && height > 0) {
		prefs->width = width;
		prefs->height = height;
	}
}
