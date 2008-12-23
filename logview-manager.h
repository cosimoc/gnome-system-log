/* logview-manager.h
 *
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@gnome.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 551 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

/* logview-manager.h */

#ifndef __LOGVIEW_MANAGER_H__
#define __LOGVIEW_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define LOGVIEW_TYPE_MANAGER logview_manager_get_type()
#define LOGVIEW_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_MANAGER, LogviewManager))
#define LOGVIEW_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_MANAGER, LogviewManagerClass))
#define LOGVIEW_IS_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_MANAGER))
#define LOGVIEW_IS_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), LOGVIEW_TYPE_MANAGER))
#define LOGVIEW_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_MANAGER, LogviewManagerClass))

typedef struct _LogviewManager LogviewManager;
typedef struct _LogviewManagerClass LogviewManagerClass;
typedef struct _LogviewManagerPrivate LogviewManagerPrivate;

struct _LogviewManager {
  GObject parent;
  LogviewManagerPrivate *priv;
};

struct _LogviewManagerClass {
  GObjectClass parent_class;

  void (* log_added) (LogviewManager *manager,
                      LogviewLog *log);
  void (* log_closed) (LogviewManager *manager,
                       LogviewLog *log);
  void (* log_add_error) (LogviewManager *manager,
                          char *filename);
  void (* active_changed) (LogviewManager *manager,
                           LogviewLog *log);
};

GType logview_manager_get_type (void);

/* public methods */
LogviewManager* logview_manager_get                 (void);
void            logview_manager_add_log_from_name   (LogviewManager *manager,
                                                     const char *filename);
void            logview_manager_add_logs_from_names (LogviewManager *manager,
                                                     GSList *names);
void            logview_manager_set_active_log      (LogviewManager *manager,
                                                     LogviewLog *log);
LogviewLog *    logview_manager_get_active_log      (LogviewManager *manager);
int             logview_manager_get_log_count       (LogviewManager *manager);
LogviewLog *    logview_manager_get_if_loaded       (LogviewManager *manager,
                                                     char *filename);
void            logview_manager_close_active_log    (LogviewManager *manager);

G_END_DECLS

#endif /* __LOGVIEW_MANAGER_H__ */