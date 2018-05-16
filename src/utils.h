/*
 * Copyright 2018 Skalski Embedded Technologies <contact@lukasz-skalski.com>
 *
 * This file is part of MZD Background Changer.
 *
 * MZD Background Changer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MZD Background Changer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MZD Background Changer. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __UTILS_H_
#define __UTILS_H_

#include <stdio.h>
#include <gio/gio.h>
#include <glib-unix.h>

#define APPNAME       "MZDBackgroundChanger"
#define REVISION      "1.0"
#define PORT          53516

typedef enum
{
  LOG_STATUS,
  LOG_WARNING,
  LOG_ERROR
} LogLevel;

typedef struct
{
  /* socket connection */
  GSocketConnection  *connection;
  GSocketClient      *client;
  GIOChannel         *channel;
  GSocket            *socket;

  /* global */
  FILE               *logfile;
  GMainLoop          *loop;
  gint                status;

} Client;

/* prototypes */

void              android_connect      (Client        *client_ctx);

gboolean          read_socket          (GIOChannel    *gio,
                                        GIOCondition   condition,
                                        gpointer       data);

gboolean          write_socket         (Client        *client_ctx,
                                        const gchar   *message,
                                        GError       **error);

FILE *            open_logfile         (gboolean       opt_debug,
                                        const gchar   *path);

void              write_logfile        (FILE          *logfile,
                                        LogLevel       loglevel,
                                        const gchar   *fmt, ...);

void              header_logfile       (FILE          *logfile);

#endif /* __UTILS_H_ */
