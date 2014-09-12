#!/bin/sh

rm -rf /tmp/upgrade 
mkdir /tmp/upgrade
sync
diskspace_chk.sh 12000
