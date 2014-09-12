#!/bin/sh

[ $# -ne 1 ] && {
	echo "Please input the size of space need to be reserved."
	return
}
reservedspace=$1

datadir=/tmp/data
[ -L $datadir -o ! -e $datadir ] && {
	echo "$datadir isn't a directory, none more space can be freed."
	return 
}

freespace=$(df | grep "/tmp$" | awk '{ print $4}')
while [ $freespace -lt $reservedspace ]; do 
	largefile=$(find $datadir -type f | xargs du | sort -n -r | awk '{if(NR==1) print $2}')
	if [ $largefile = '.' ]; then
		echo "no more file can be removed."
		return
	else
		rm -f $largefile
		sync
	fi
	freespace=$(df | grep "/tmp$" | awk '{ print $4}')
done

