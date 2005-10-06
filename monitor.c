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
#include "monitor.h"
#include <libgnomevfs/gnome-vfs-ops.h>

void
monitor_stop (LogviewWindow *window)
{
  Log *log;

	if (!window)
		return;
  log = window->curlog;
  if (!log)
    return;

	if (log->mon_handle != NULL) {
		GtkListStore *list;
		list = (GtkListStore *) gtk_tree_view_get_model (GTK_TREE_VIEW(window->mon_list_view));
		gtk_list_store_clear (list);

    log->monitored = FALSE;

    gnome_vfs_monitor_cancel (log->mon_handle);
    log->mon_handle = NULL;
		log->mon_offset = 0;

    gnome_vfs_close (log->mon_file_handle);
    log->mon_file_handle = NULL;
	}
}

void
monitor_callback (GnomeVFSMonitorHandle *handle, const gchar *monitor_uri,
                  const gchar *info_uri, GnomeVFSMonitorEventType event_type,
                  gpointer data)
{
  GnomeVFSResult result;
  gchar *buffer_lines;
  LogviewWindow *logview;
  Log *log = data;
  
  g_return_if_fail (log);
  buffer_lines = ReadNewLines (log);

  if (buffer_lines != NULL) {
    log = log_add_lines (log, buffer_lines);
    logview  = (LogviewWindow *) log->window;
    if (logview->curlog == log) 
      log_repaint (logview);
  }
  
  return;
}

void
monitor_start (LogviewWindow *window)
{
	Log *log = window->curlog;
	g_return_if_fail (log);
  GnomeVFSResult result;
  GnomeVFSFileSize size;
  gchar *main, *second;
	 
  result = gnome_vfs_open (&(log->mon_file_handle), log->name, 
                           GNOME_VFS_OPEN_READ);
  result = gnome_vfs_seek (log->mon_file_handle, GNOME_VFS_SEEK_END, 0L);
  result = gnome_vfs_tell (log->mon_file_handle, &size);
  log->mon_offset = size;

  result = gnome_vfs_monitor_add (&(log->mon_handle), log->name,
                         GNOME_VFS_MONITOR_FILE, monitor_callback,
                         log);

  if (result != GNOME_VFS_OK) {
    g_sprintf (main, _("This file cannot be monitored."));
    switch (result) {
    case GNOME_VFS_ERROR_NOT_SUPPORTED :
      g_sprintf(second, _("File monitoring is not supported on this file system.\n"));
    default:
      g_sprintf(second, _("Gnome-VFS Error.\n"));
    }
    ShowErrMessage (NULL, main, second);
    gnome_vfs_close (log->mon_file_handle);
    return;
  }

  log->monitored = TRUE;
}
