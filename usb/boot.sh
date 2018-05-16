#!/bin/sh
#
# Copyright 2018 Skalski Embedded Technologies <contact@lukasz-skalski.com>
#
# This file is part of MZD Background Changer.
#
# MZD Background Changer is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# MZD Background Changer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with MZD Background Changer. If not, see <http://www.gnu.org/licenses/>.
#

bin_file="MZDBackgroundChanger"
boot_file="MZDBackgroundBoot"
storage_dir="/mnt/sda1"

log_head="\e[1;34m"
log_end="\e[0m"

find_path()
{
  dir=$(find /mnt/sd* -name cmu_dataretrieval.up)
  dir=$(dirname $dir)
  dir=$(basename $dir)
  storage_dir="/mnt/$dir"
  cd $storage_dir
}

collect_data()
{
  # check 'version'
  echo -en "\e[1;32m[$(date +"%T")]\e[0m $log_head version: $log_end"
  /jci/scripts/show_version.sh

  # check 'storage'
  echo -e "\e[1;32m[$(date +"%T")]\e[0m $log_head storage:$log_end $storage_dir"

  # check 'whoami'
  echo -en "\e[1;32m[$(date +"%T")]\e[0m $log_head whoami: $log_end"
  whoami

  # check 'home'
  echo -e "\e[1;32m[$(date +"%T")]\e[0m $log_head home:$log_end $HOME"

  # check 'passwd'
  echo -e "\e[1;32m[$(date +"%T")]\e[0m $log_head passwd: $log_end"
  cat /etc/passwd
  echo ""
}

# find path
find_path

# forward stdout and stderr
# rm -Rf $storage_dir/Log/$bin_file-stage1.log
# exec &> $storage_dir/Log/$bin_file-stage1.log

# let's start
echo -e "\e[1;32m[$(date +"%T")]\e[0m $log_head start:$log_end STAGE 1"

# collect data
# collect_data

# preparing data for second stage of boot process - 1
echo -e "\e[1;32m[$(date +"%T")]\e[0m $log_head preparing data for second stage:$log_end 1"
pkill -2 -f $bin_file
sleep 1
pkill -9 -f adb
sleep 1

# preparing data for second stage of boot process - 2
echo -e "\e[1;32m[$(date +"%T")]\e[0m $log_head preparing data for second stage:$log_end 2"
chmod +x Data/$boot_file

# start second stage
echo -e "\e[1;32m[$(date +"%T")]\e[0m $log_head preparing data for second stage:$log_end completed"
su $(whoami) -c "$storage_dir/Data/$boot_file"

# that's it
echo -e "\e[1;32m[$(date +"%T")]\e[0m $log_head end:$log_end STAGE 1"
exit 0
