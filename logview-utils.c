/* logview-utils.c
 *
 * Copyright (C) 1998  Cesar Miquel  <miquel@df.uba.ar>
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
 */

#define _XOPEN_SOURCE
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <glib.h>

#include "logview-utils.h"

static gint 
days_compare (gconstpointer a, gconstpointer b)
{
  const Day *day1 = a, *day2 = b;

  return g_date_compare (day1->date, day2->date);
}

static GDate *
string_get_date (const char *line, char **time_string)
{
  GDate *date;
  struct tm tp;
  char *cp;
  char tmp[50];
  size_t chars_read;
  
  /* it's safe to assume that if strptime returns NULL, it's
   * because of an error (format unmatched). being a log file, it's very
   * unlikely that there aren't any more characters after the date.
   */

  if (line == NULL || line[0] == '\0') {
    return NULL;
  }

  /* this parses the "MonthName DayNo" format */
  cp = strptime (line, "%b %d", &tp);
  if (cp) {
    chars_read = strftime (tmp, 50, "%b %e", &tp);
    goto out;
  }

  /* this parses the YYYY-MM-DD format */
  cp = strptime (line, "%F", &tp);
  if (cp) {
    chars_read = strftime (tmp, 50, "%F", &tp);
    goto out;
  }

out:
  /* the year doesn't matter to us now */
  date = g_date_new_dmy (tp.tm_mday, tp.tm_mon + 1, 1);
  *time_string = g_strndup (tmp, chars_read);

  return date;
}

/**
 * log_read_dates:
 *
 * @buffer_lines: an array of text lines.
 * @current: the current time.
 *
 * Reads all the dates inside the text buffer.
 * All dates are given with respect to the 1/1/1970
 * and are then corrected to the correct year once we
 * reach the end.
 *
 * Returns: a #GSList of #Day structures. 
 */

GSList *
log_read_dates (const char **buffer_lines, time_t current)
{
  int offsetyear = 0, current_year;
  GSList *days = NULL, *days_copy;
  GDate *date, *newdate;
  struct tm *tmptm;
  char *date_string;
  Day *day;
  gboolean done = FALSE;
  int i, n, rangemin, rangemax, idx;

  g_return_val_if_fail (buffer_lines != NULL, NULL);

  n = g_strv_length ((char **) buffer_lines);

  tmptm = localtime (&current);
  current_year = tmptm->tm_year + 1900;

  /* find the first line with a date we're able to parse */
  for (i = 0; buffer_lines[i]; i++) {
    if ((date = string_get_date (buffer_lines[i], &date_string)) != NULL)
      break;
  }

  if (!date) {
    /* no valid dates in the array, return NULL */
    return NULL;
  }

  if (!g_date_valid (date)) {
    g_date_free (date);
    g_free (date_string);
    return NULL;
  }

  g_date_set_year (date, current_year);

  day = g_slice_new0 (Day);
  days = g_slist_append (days, day);

  /* $i now contains the line number for the first good date */
  day->date = date;
  day->first_line = i;
  day->last_line = -1;

  /* now scan the logfile to get the last line of the day */
  rangemin = i;
  rangemax = n - 1;

  while (!done) {
    /* find out the last line of the day we're currently building */

    i = n - 1;

    while (day->last_line < 0) {
      if (strstr (buffer_lines[i], date_string)) {
        /* if we find the same string on the last line of the log, we're done */
        if (i == n - 1) {
          done = TRUE;
          day->last_line = i;
          break;
        }

        /* we're still in a section of lines with the same date;
         * - if the next one changes, then we're on the last.
         * - else we keep searching in the following.
         */

        if (!strstr (buffer_lines[i + 1], date_string)) {
            day->last_line = i;
            break;
        } else {
          rangemin = i;
          i = floor (((float) i + (float) rangemax) / 2.);
        }
      } else {
        /* we can't find the same date here; go back to a safer range. */
        rangemax = i;
        i = floor (((float) rangemin + (float) i) / 2.);               
      }
    }

    g_free (date_string);

    if (!done) {
      /* this means we finished the current day but we're not at the end
       * of the buffer: reset the parameters for the next day.
       */
      newdate = NULL;
      
      for (i = day->last_line + 1; buffer_lines[i]; i++) {
        if ((newdate = string_get_date (buffer_lines[i], &date_string)) != NULL)
        break;
      }

      if (date_string == NULL && i == n - 1) {
        done = TRUE;
      }

      /* this will set the last line of the "old" log to either:
       * - "n - 1" if we can't find another date
       * - the line before the new date else.
       */
      day->last_line = i - 1;

      if (newdate) {
        /* append a new day to the list */

        g_date_set_year (newdate, current_year + offsetyear);

        if (g_date_compare (newdate, date) < 1) {
          /* this isn't possible, as we're reading the log forward.
           * so it means that newdate is the next year.
           */
          g_date_add_years (newdate, 1);
        }

        date = newdate;
        day = g_slice_new0 (Day);
        days = g_slist_prepend (days, day);

        day->date = date;
        day->first_line = i;
        day->last_line = -1;
        rangemin = i;
        rangemax = n - 1;
      }
    }
  }

  /* sort the days in chronological order */
  days = g_slist_sort (days, days_compare);

  return days;
}