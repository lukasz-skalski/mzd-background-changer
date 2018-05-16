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
#include <string.h>
#include "commands.h"

gboolean
cmd_utils_check_rev (Client  *client_ctx,
                     GError **error)
{
  GString   *revision;
  gboolean   ret;

  revision = g_string_new (NULL);
  g_string_printf (revision, "RET_REVISION%%%s\n", REVISION);

  ret = write_socket (client_ctx, revision->str, error);

  g_string_free (revision, TRUE);
  return ret;
}

gboolean
cmd_image_upload (Client       *client_ctx,
                  const gchar  *data,
                  GError      **error)
{
  gchar   **imageinfo;
  gchar    *cmd_md, *cmd_cp;
  gchar    *xstdout;
  gboolean  ret;

  imageinfo = g_strsplit (data, "  ", -1);
  cmd_md = NULL;
  cmd_cp = NULL;
  xstdout = NULL;
  ret = TRUE;

  /* TODO: fixme! */
  system ("sleep 2");

  /*
   * [0] check for custom image
   */
  if (g_ascii_strncasecmp (imageinfo[0], "custom-image", strlen (imageinfo[0])))
    {
      cmd_md = g_strdup_printf("md5sum %s", imageinfo[1]);
      cmd_cp = g_strdup_printf("cp -f %s /jci/gui/common/images/background.png",
                               imageinfo[1]);
    }
  else
    {
      if (g_file_test ("/tmp/MZDBackgrounds/mzd_bg_custom.png",
                       G_FILE_TEST_EXISTS))
        {
          cmd_cp = g_strdup_printf ("cp -f "
                                    "/tmp/MZDBackgrounds/mzd_bg_custom.png "
                                    "/jci/gui/common/images/background.png");
          goto upload;
        }
      else
        {
          write_logfile (client_ctx->logfile, LOG_ERROR,
                         "Custom image is not available!\n");
          write_socket (client_ctx,
                        "RET_IMAGE_UPLOAD_ERROR%%Custom image "
                        "is not available!\n",
                        NULL);
          goto exit;
        }
    }

  /*
   * [1] check md5sum
   */
  ret = g_spawn_command_line_sync (cmd_md, &xstdout, NULL, NULL, error);
  if (ret == FALSE)
    {
      write_logfile (client_ctx->logfile, LOG_ERROR,
                     "Unable to calculate MD5 hash!\n");
      write_socket (client_ctx,
                    "RET_IMAGE_UPLOAD_ERROR%%Unable to calculate MD5 hash!\n",
                    NULL);
      goto exit;
    }

  /*
   * [2] compare md5sum
   */
  if (g_ascii_strncasecmp (data, xstdout, strlen (data)))
    {

      write_logfile (client_ctx->logfile, LOG_ERROR,
                     "MD5 hash value is invalid!\n");
      write_socket (client_ctx,
                    "RET_IMAGE_UPLOAD_ERROR%%MD5 hash value is invalid!\n",
                    NULL);
      goto exit;
  }

upload:

  /*
   * [3] remount filesystem
   * TODO: fixme!
   */
  system ("echo 1 > /sys/class/gpio/Watchdog\\ Disable/value");
  system ("mount -o rw,remount /");

  /*
   * [4] copy background
   */
  ret = g_spawn_command_line_sync (cmd_cp, NULL, NULL, NULL, error);
  if (ret == FALSE)
    {
      write_logfile (client_ctx->logfile, LOG_ERROR,
                     "Unable to change background!\n");
      write_socket (client_ctx,
                    "RET_IMAGE_UPLOAD_ERROR%%Unable to change background!\n",
                    NULL);
      goto exit;
    }

  /* send 'RET_IMAGE_UPLOAD_OK' to Android App */
  write_socket (client_ctx, "RET_IMAGE_UPLOAD_OK\n", NULL);

  /* TODO: fixme! */
  system ("sync && sleep 2 && reboot");

exit:

  if (imageinfo)
    g_strfreev (imageinfo);

  if (xstdout)
    g_free (xstdout);

  if (cmd_md)
    g_free (cmd_md);

  if (cmd_cp)
    g_free (cmd_cp);

  return ret;
}
