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

#include <stdio.h>
#include <glib/gprintf.h>
#include "utils.h"
#include "adb.h"
#include "commands.h"

static void
next_iteration_after_timeout (Client *client_ctx)
{
  /* let's do some cleanup */
  if (client_ctx->connection)
    {
      g_object_unref (client_ctx->connection);
      client_ctx->connection = NULL;
    }

  if (client_ctx->client)
    {
      g_object_unref (client_ctx->client);
      client_ctx->client = NULL;
    }

  if (client_ctx->channel)
    {
      g_io_channel_unref (client_ctx->channel);
      client_ctx->channel = NULL;
    }

  /* jump to next iteration */
  g_timeout_add_seconds (4, adb_restart, client_ctx);
}

void
android_connect (Client *client_ctx)
{
  GError *error;

  error = NULL;

  /* create a new connection */
  client_ctx->client = g_socket_client_new();
  if (client_ctx->client == NULL)
    {
      write_logfile (client_ctx->logfile, LOG_ERROR,
                     "Cannot create new connection\n");
      goto exit;
    }
  else
    write_logfile (client_ctx->logfile, LOG_STATUS,
                   "Creating new connection\n");

  /* connect to the host */
  client_ctx->connection = g_socket_client_connect_to_host (client_ctx->client,
                                                            "127.0.0.1",
                                                            PORT, NULL, &error);
  if (error != NULL)
    {
      write_logfile (client_ctx->logfile, LOG_WARNING,
                     "Connection bridge is not ready: %s\n", error->message);
      g_error_free (error);

      /* jump to next iteration */
      next_iteration_after_timeout (client_ctx);
    }
  else
    {
      write_logfile (client_ctx->logfile, LOG_STATUS,
                     "Connection bridge is ready to use\n");

      client_ctx->socket = g_socket_connection_get_socket (client_ctx->connection);
      if (client_ctx->socket == NULL)
        {
          write_logfile (client_ctx->logfile, LOG_ERROR,
                         "Failed to initialize connection subsystems\n");
          goto exit;
        }
      else
        {
          write_logfile (client_ctx->logfile, LOG_STATUS,
                         "Initialize connection subsystems\n");
          client_ctx->channel = g_io_channel_unix_new (g_socket_get_fd (client_ctx->socket));

          if (!g_io_add_watch (client_ctx->channel, G_IO_IN | G_IO_HUP,
                               read_socket, client_ctx))
            {
              write_logfile (client_ctx->logfile, LOG_ERROR,
                             "Cannot create 'watch' channel\n");
              goto exit;
            }
        }
    }

  return;

exit:
        client_ctx->status = EXIT_FAILURE;
        g_main_loop_quit (client_ctx->loop);
}


static gboolean
handle_command (Client       *client_ctx,
                const gchar  *msg,
                GError      **error)
{
  write_logfile (client_ctx->logfile, LOG_STATUS, "Data: %s\n", msg);

  /*
   * commands-check-revision
   */
  if (g_str_has_prefix (msg, "CMD_CHECK_REVISION"))
    {
      return cmd_utils_check_rev (client_ctx, error);
    }

  /*
   * commands-image-upload
   */
  else if (g_str_has_prefix (msg, "CMD_IMAGE_UPLOAD"))
    {
      return cmd_image_upload (client_ctx, (gchar*) msg+17, error);
    }

  g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
               "Not supported command: %s", msg);
  return FALSE;
}


gboolean
read_socket (GIOChannel    *gio,
             GIOCondition   condition,
             gpointer       data)
{
  Client    *client_ctx;
  GError    *error;
  gchar     *msg;
  gchar     *tmp;
  GIOStatus  ret;
  gsize      len;

  client_ctx = (Client*) data;
  error = NULL;

  /* read data from socket */
  ret = g_io_channel_read_line (gio, &msg, &len, NULL, &error);
  if (ret == G_IO_STATUS_ERROR)
    {
      write_logfile (client_ctx->logfile, LOG_WARNING,
                     "Cannot read data from Android App: %s\n", error->message);
      g_error_free (error);

      return TRUE;
    }

  /* android app is not ready yet -> next iteration */
  if (msg == 0)
    {
      write_logfile (client_ctx->logfile, LOG_WARNING,
                     "Android App is not ready yet\n");

      next_iteration_after_timeout (client_ctx);
      return FALSE;
    }

  /* handle received command */
  if (!handle_command (client_ctx, msg, &error))
    {
      write_logfile (client_ctx->logfile, LOG_WARNING, "%s\n", error->message);
      g_error_free (error);
    }

  g_free (msg);
  return TRUE;
}


gboolean
write_socket (Client       *client_ctx,
              const gchar  *message,
              GError      **error)
{
  GIOStatus ret;

  write_logfile (client_ctx->logfile, LOG_STATUS, "Data 'to': %s", message);

  if (client_ctx->channel != NULL)
    {
      ret = g_io_channel_write_chars (client_ctx->channel,
                                      message, -1, NULL, error);
      if (ret == G_IO_STATUS_ERROR)
        return FALSE;

      ret = g_io_channel_flush (client_ctx->channel, error);
      if (ret == G_IO_STATUS_ERROR)
        return FALSE;
    }
  else
    {
      write_logfile (client_ctx->logfile, LOG_WARNING,
                     "Channel to Android device is not available\n");
    }

  return TRUE;
}


FILE *
open_logfile (gboolean      opt_debug,
              const gchar  *path)
{
  if (opt_debug)
    return stdout;
  else
    return fopen (path, "w");
}


void
write_logfile (FILE         *logfile,
               LogLevel      loglevel,
               const gchar  *fmt, ...)
{
  time_t tm_time;
  struct tm tms;
  va_list args;

  if (logfile != NULL)
    {
      /* get current time */
      tm_time = time (NULL);
      tms = *localtime (&tm_time);

      /* set color for loglevel */
      switch (loglevel)
        {
          case (LOG_STATUS):  g_fprintf (logfile, "\x1b[32m"); break;
          case (LOG_WARNING): g_fprintf (logfile, "\x1b[33m"); break;
          case (LOG_ERROR):   g_fprintf (logfile, "\x1b[31m"); break;
          default: break;
        }

      /* write timestamp */
      g_fprintf (logfile, "[%04d/%02d/%02d %02d:%02d:%02d] ",
                 tms.tm_year + 1900, tms.tm_mon + 1,
                 tms.tm_mday, tms.tm_hour,
                 tms.tm_min, tms.tm_sec);

      /* write log message */
      va_start (args, fmt);
      g_vfprintf (logfile, fmt, args);
      va_end (args);

      /* reset color */
      g_fprintf (logfile, "\x1b[0m");

      /* flush */
      fflush (logfile);
    }
}


void
header_logfile (FILE *logfile)
{
  if (logfile != NULL)
    {
      g_fprintf (logfile, "\x1b[36m");
      g_fprintf (logfile, "\n+----------------------------+\n");
      g_fprintf (logfile, "|       %s       |\n", APPNAME);
      g_fprintf (logfile, "| date: %s %s |\n", __DATE__, __TIME__);
      g_fprintf (logfile, "| rev.: %s                  |\n", REVISION);
      g_fprintf (logfile, "+----------------------------+\n\n");
      g_fprintf (logfile, "\x1b[0m");

      fflush (logfile);
    }
}
