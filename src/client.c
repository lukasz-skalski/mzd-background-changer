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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/prctl.h>
#include "utils.h"
#include "adb.h"

static gboolean opt_debug = FALSE;

GOptionEntry entries[] =
{
  { "debug", 'd', 0, G_OPTION_ARG_NONE, &opt_debug,
    "redirect logs to stdout", NULL},
  { NULL }
};

static gboolean
unix_signal_handler (gpointer user_data)
{
  Client *client_ctx;
  client_ctx = (Client *) user_data;

  write_logfile (client_ctx->logfile, LOG_STATUS, "Force app restart...\n");

  client_ctx->status = EXIT_SUCCESS;
  g_main_loop_quit (client_ctx->loop);
}

int
main (int argc, char **argv, char **envp)
{
  GOptionContext  *option_context;
  GMainLoop       *main_loop;
  Client          *client_ctx;
  GError          *error;
  FILE            *logfile;
  gint             status;

  client_ctx = NULL;
  error = NULL;
  logfile = NULL;

  /* parse commandline options */
  option_context = g_option_context_new ("");
  g_option_context_add_main_entries (option_context, entries, NULL);
  g_option_context_parse (option_context, &argc, &argv, &error);

  /* open logfile */
  logfile = open_logfile (opt_debug, argv[1]);

  /* write header to logfile */
  header_logfile (logfile);

  /* add first log to logfile */
  write_logfile (logfile, LOG_STATUS, "%s has been started...\n", APPNAME);
  write_logfile (logfile, LOG_STATUS, "Waiting for Android device\n");

  /* create a new client */
  client_ctx = g_new0 (Client, 1);
  if (client_ctx == NULL)
    {
      write_logfile (logfile, LOG_ERROR, "Cannot allocate memory for Client\n");

      g_option_context_free (option_context);
      fclose (logfile);

      return EXIT_FAILURE;
    }
  else
    {
      write_logfile (logfile, LOG_STATUS, "Creating new 'Client'\n");
      client_ctx->status = status = EXIT_FAILURE;
      client_ctx->logfile = logfile;

      /* initialize mainloop */
      client_ctx->loop = g_main_loop_new (NULL, FALSE);
    }

  /* signals handler */
  g_unix_signal_add (SIGINT, unix_signal_handler, client_ctx);

  /* start ADB BRIDGE */
  if (adb_start (client_ctx))
    write_logfile (client_ctx->logfile, LOG_STATUS, "ADB started...\n");
  else
    {
      write_logfile (client_ctx->logfile, LOG_ERROR, "Cannot start ADB...\n");
      goto exit;
    }

  /* enter mainloop */
  write_logfile (client_ctx->logfile, LOG_STATUS, "Entering to mainloop...\n");
  g_main_loop_run (client_ctx->loop);

exit:

  /* exit status */
  status = client_ctx->status;
  write_logfile (client_ctx->logfile, LOG_WARNING,
                 "%s has been stopped with status: %d\n", APPNAME, status);

  if (client_ctx->connection)
    g_object_unref (client_ctx->connection);

  if (client_ctx->client)
    g_object_unref (client_ctx->client);

  if (client_ctx->channel)
    g_io_channel_unref (client_ctx->channel);

  if (client_ctx->loop)
    g_main_loop_unref (client_ctx->loop);

  if (client_ctx->logfile)
    {
      fflush (client_ctx->logfile);
      fclose (client_ctx->logfile);
    }

  if (client_ctx)
    g_free (client_ctx);

  if (option_context)
    g_option_context_free (option_context);

  return status;
}
