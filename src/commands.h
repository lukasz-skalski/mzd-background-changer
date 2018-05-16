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

#ifndef __COMMANDS_UTILS_H_
#define __COMMANDS_UTILS_H_

#include <gio/gio.h>
#include <glib-unix.h>
#include "utils.h"

gboolean       cmd_utils_check_rev        (Client        *client_ctx,
                                           GError       **error);

gboolean       cmd_image_upload           (Client       *client_ctx,
                                           const gchar  *data,
                                           GError      **error);

#endif /* __COMMANDS_UTILS_H_ */
