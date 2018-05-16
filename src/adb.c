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

#define _GNU_SOURCE
#include <gio/gio.h>
#include <glib-unix.h>
#include <sys/prctl.h>
#include "utils.h"
#include "adb.h"

static void
adb_setup (gpointer data)
{
  prctl (PR_SET_PDEATHSIG, SIGKILL);
}

static void
adb_exit (GPid       pid,
          gint       status,
          gpointer   user_data)
{
  Client *client_ctx;
  client_ctx = (Client *) user_data;

  g_spawn_close_pid (pid);
  write_logfile (client_ctx->logfile, LOG_STATUS, "Forwarding ports...\n");

  /* TODO: port forwarding (fixme!) */
  system ("/tmp/MZDBackgroundBridge forward tcp:53516 tcp:53516");

  /* try to connect with Android device */
  write_logfile (client_ctx->logfile, LOG_STATUS, "Connecting...\n");
  android_connect (client_ctx);
}

gboolean
adb_start (Client *client_ctx)
{
  GPid       adb_pid;
  GError    *adb_error;
  gchar     *adb_argv[3];
  gboolean   adb_ret;

  adb_argv[0] = "MZDBackgroundBridge";
  adb_argv[1] = "wait-for-device";
  adb_argv[2] = NULL;
  adb_error = NULL;

  /* spawn adb wait-for-device */
  adb_ret = g_spawn_async ("/tmp",        /* working directory */
                           adb_argv,      /* argv */
                           NULL,          /* envp */
                           G_SPAWN_DO_NOT_REAP_CHILD |
                           G_SPAWN_STDOUT_TO_DEV_NULL |
                           G_SPAWN_STDERR_TO_DEV_NULL,
                           adb_setup,     /* child setup */
                           NULL,          /* user data */
                           &adb_pid,      /* child process ID */
                           &adb_error);   /* error */
  if (adb_ret == FALSE)
    {
      write_logfile (client_ctx->logfile, LOG_ERROR, "Cannot start bridge:%s\n",
                     adb_error->message);
      g_error_free (adb_error);
      return FALSE;
    }

  /* add new source */
  g_child_watch_add (adb_pid, adb_exit, client_ctx);

  return TRUE;
}

gboolean
adb_restart (gpointer user_data)
{
  Client *client_ctx;
  client_ctx = (Client *) user_data;

  if (adb_start (client_ctx))
    write_logfile (client_ctx->logfile, LOG_STATUS, "ADB restarted...\n");
  else
    {
      write_logfile (client_ctx->logfile, LOG_ERROR, "Cannot restart ADB...\n");
      g_main_loop_quit (client_ctx->loop);
    }
  return FALSE;
}
