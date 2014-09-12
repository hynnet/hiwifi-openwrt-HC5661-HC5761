#!/bin/sh

infofile=/tmp/sdinfo.txt
testfile=/tmp/sdtest.txt

sync
sdpid=$$
sddev=$(df -m | awk '{if(/\/dev\/mmc/) print $1 }')
sdmount=$(df -m | awk '{if(/\/dev\/mmc/) print $6 }')
sdsize=$(df -m | awk '{if(/\/dev\/mmc/) print $2 }')
sdsize=${sdsize:-0}
[ "$sddev" = "/dev/mmcblk0p2" ] && sdsize=$((sdsize+1024))
echo "sdsize: ${sdsize}M"

sdfree=$(df -m | awk '{if(/\/dev\/mmc/) print $4 }')
sdfree=${sdfree:-0}
echo "sdfree: ${sdfree}M"

ID_FS_TYPE=""
eval `blkid -o udev $sddev`

fstype=${ID_FS_TYPE:-N/A}

mount | grep "$sddev" | grep -qs "ro,"

if [ $? -eq 0 ]; then
	sdstatus="ro"
else
	sdstatus="rw"
fi

echo "fstype: $fstype"

[ $# -eq 0 ] && {
	cat <<EOF >$infofile
sdsize=$sdsize
fstype=$fstype
sdstatus=$sdstatus
EOF

	echo "info:"
	cat $infofile
	return
}

memfree=$(cat /proc/meminfo | awk '{ if(/MemFree:/) print $2}')
ddcount=$((memfree/2))
ddbsk=4
ddmem=$((ddbsk*ddcount/1024))
echo "memfree: ${memfree}K, ddmem: ${ddmem}M"

readspeed=N/A
[ $ddmem -lt $sdsize ] && {
	txt=$(dd if=$sddev of=/dev/null bs=${ddbsk}K count=$ddcount 2>&1)
	readspeed=$(echo $txt | awk -F', ' '{print $3}')
	echo "read: "
	echo "$txt"
	echo "readspeed: $readspeed"
}

writespeed=N/A
[ $ddmem -lt $sdfree ] && {
	if [ "$sdstatus" == "rw" ]; then
		txt=$(dd if=/dev/zero of=$sdmount/sd_test_$sdpid bs=${ddbsk}K count=$ddcount 2>&1)
		writespeed=$(echo $txt | awk -F', ' '{print $3}')
		rm -f $sdmount/sd_test_$sdpid
		echo "write: "
		echo "$txt"
		echo "writespeed: $writespeed"
	fi
}

cat <<EOF >$testfile
sdsize=$sdsize
fstype=$fstype
readspeed=$readspeed
writespeed=$writespeed
EOF

echo "test result:"
cat $testfile

