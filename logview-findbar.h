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

#ifndef __LOG_FINDBAR_H__
#define __LOG_FINDBAR_H__

#include "logview.h"
#include <gtk/gtk.h>

typedef enum {
    YELP_WINDOW_FIND_PREV = 1,
    YELP_WINDOW_FIND_NEXT
} LogviewFindAction;

typedef struct _SearchIter SearchIter;

struct _SearchIter {
	const char *pattern;
	GtkTreePath *found_path;
	GtkTreePath *current_path;
	int comparison;
	gboolean forward;
};
void logview_findbar_populate (LogviewWindow *window, GtkWidget *widget);

#endif /* __LOG_FINDBAR_H__ */
