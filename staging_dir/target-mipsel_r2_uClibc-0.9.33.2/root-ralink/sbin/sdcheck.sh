#!/bin/sh

size=$(fdisk -l /dev/mmcblk0 | grep "mmcblk0:" | awk '{print $5}')
size=${size:-0}
size=$((size/1000/1000))

echo "sdsize=$size"
echo "minsize=3000"

