#!/bin/ash

source /usr/share/libubox/jshn.sh
down_list="http://speed.dtgt.org/speedtest/random350x350.jpg http://speed.dtgt.org/speedtest/random750x750.jpg"
up_list="http://speed.dtgt.org/speedtest/upload.php"
down_parallels=5
up_parallels=5
act_id=""
sqos_down_bw=""
sqos_up_bw=""
up_data_size=512
test_time=10
actid_last=0
debug=$2
result_uri='https://m.hiwifi.com/api/Router/saveSpeedTest'
down_uri_list=""

save_and_init_sqos_bw()
{
	sqos_down_bw=$(cat /proc/sys/net/smartqos/download_bw)
	sqos_up_bw=$(cat /proc/sys/net/smartqos/upload_bw)
	echo 0 > /proc/sys/net/smartqos/download_bw
	echo 0 > /proc/sys/net/smartqos/upload_bw
}

restore_sqos_bw()
{
	echo $sqos_down_bw > /proc/sys/net/smartqos/download_bw
	echo $sqos_up_bw > /proc/sys/net/smartqos/upload_bw
}

get_json_str_arry_fields()
{                
	local idx=1
	local uri=""
	while json_get_type _type  $idx && [ "$_type" = string ]; do
		json_get_var tmp_uri  "$((idx++))"                  
		uri=$uri$tmp_uri" "               
	done                       
	echo $uri
} 

valid_download_uri()
{
	local list=$1
	local uri=""
	local found=1

	for uri in $list; do

		wget -s "$uri" &> /dev/null
		if [ $? -eq 0 ];then
			down_uri_list="$down_uri_list$uri "
			found=0	
		fi
	done

	return $found	
}

send_curr_result()
{
	local dir=$1
	local speed=$2
	local status=$3
	local rv=0

	if [ "$debug" = "1" ]; then
		curl -k -X POST -d "actid=$act_id&dir=$dir&speed=$speed&status=$status&actid_last=$actid_last" $result_uri
		rv=$?
		echo ""
	else
		curl -k -X POST -d "actid=$act_id&dir=$dir&speed=$speed&status=$status&actid_last=$actid_last" $result_uri &> /dev/null
		rv=$?
	fi
	
	if [ $rv -ne 0 ]; then
		if [ "$debug" = "1" ]; then
			curl -k -X POST -d "actid=$act_id&dir=$dir&speed=$speed&status=$status&actid_last=$actid_last" $result_uri
			echo ""
		else 
			curl -k -X POST -d "actid=$act_id&dir=$dir&speed=$speed&status=$status&actid_last=$actid_last" $result_uri &> /dev/null
		fi
	fi
}

download_speed_test()
{
	local down_list=$1
	local parallel_num=$2
	local time_out=$(($(date +%s) + test_time))
	local curr_time=$(date +%s)
	local old_time=0
	local speed=0
	local duration=10

	local speed_sum=0
	local speed_count=0;
	
	while true;
	do
		curr_time=$(date +%s)
		speed=$(cat /proc/net/smartqos/stat | awk '{if(match($0,/^=*$/)){target= NR +1} if(NR == target) print $3}')
		if [ $curr_time -ge $time_out ]; then
			send_curr_result "down" $speed "finish"
			
			speed_count=$((speed_count + 1))
			speed_sum=$((speed_sum + speed))	
			
			speed_avg=$((speed_sum / speed_count))
			
			uci set speedtest.down=$speed_avg
			uci commit
			break
		fi
		if [ "$curr_time" -gt $old_time ]; then
			send_curr_result "down" $speed "running" &
			
			speed_count=$((speed_count + 1))
			speed_sum=$((speed_sum + speed))	
			old_time=$curr_time;
		fi

		duration=$(( time_out - curr_time))
		for uri in $down_list;do	
			job_num=$(ps www | grep [c]url |grep "$uri" | wc -l)
			while [ $job_num -lt $parallel_num ];
			do
				curl  -m $duration -o /dev/null $uri  &> /dev/null &
				job_num=$((job_num + 1))
			done
		done
	done 
}

upload_speed_test()
{
	local uris=$1
	local parallel_num=$2
	local up_data_size=$3
	local curr_time=$(date +%s)
	local time_out=$((curr_time + test_time))
	local old_time=0
	local speed=0

	local speed_sum=0
	local speed_count=0;

	for up_uri in $uris;do
		/sbin/hwf_speedtest_upload "$up_data_size" "$parallel_num" "$up_uri" &> /dev/null &
	done
	
	while true; 
	do
		curr_time=$(date +%s)
		speed=$(cat /proc/net/smartqos/stat | awk '{if(match($0,/^=*$/)){target= NR +1} if(NR == target) print $5}')
		if [ $curr_time -gt $time_out ]; then
			killall -KILL hwf_speedtest_upload &> /dev/null
			send_curr_result "up" $speed "finish"  &
			                                                                                                                           
			speed_count=$((speed_count + 1)) 
			speed_sum=$((speed_sum + speed))        
			
			speed_avg=$((speed_sum / speed_count))
			
			uci set speedtest.up=$speed_avg
			uci commit
			sleep 1
			break
		fi			
		
		if [ "$curr_time" -gt "$old_time" ]; then 
		
			speed_count=$((speed_count + 1)) 
			speed_sum=$((speed_sum + speed))        
			
			send_curr_result "up" $speed "running" &
			old_time=$curr_time
		fi
	done                                                                                                                                      

	killall -KILL hwf_speedtest_upload &> /dev/null
}

parse_json_str_args()
{
	json_load "$1"
	json_get_var act_id actid &> /dev/null
	if [ $? -ne 0 ]; then
		echo "false:act id"
		return 1
	fi	

	json_get_var up_data_size	up_data_size	&> /dev/null
	if [ $? -ne 0 ]; then
		up_data_size=512
	fi

	json_get_var down_parallels	down_parallels &> /dev/null
	if [ $? -ne 0 ]; then
		down_parallels=5
	elif [ "$down_parallels" != "" ]; then
		expr match	"$down_parallels" "[0-9][0-9]*$" &> /dev/null
		if [ $? -ne 0 ]; then
			echo "false: down parallels"
			return 1
		fi
	fi

	json_get_var up_parallels up_parallels &> /dev/null
	if [ $? -ne 0 ]; then
		up_parallels=5
	elif [ "$up_parallels" != "" ]; then
		expr match "$up_parallels" "[0-9][0-9]*$" &> /dev/null
		if [ $? -ne 0 ]; then
			echo "false:up parallels"
			return 1
		fi
	fi

	json_get_var actid_last actid_last &> /dev/null
	if [ $? -ne 0 ]; then
		actid_last=0
	fi
	
	json_get_var test_time test_time &> /dev/null
	if [ $? -ne 0 ]; then
		echo "false: test time"
		return 1
	fi
	
	json_select "res_list_down" &>/dev/null
	if [ $? -ne 0 ]; then
		echo "false:down list"
		return 1
	fi
	down_list=$(get_json_str_arry_fields)
	json_select ".."
	json_select "res_list_up" &>/dev/null
	
	if [ $? -ne 0 ]; then
		echo "false:up list"
		return 1
	fi

	up_list=$(get_json_str_arry_fields)
	echo $act_id
	return 0
}

if [ "$debug" != "1" ]; then
	debug=0;
fi


parse_json_str_args "$1"
if [ $? -ne 0 ]; then
	echo "false:parse parameters"
	return 1
fi

save_and_init_sqos_bw

valid_download_uri  "$down_list"
if [ "$?" -ne 0 ]; then
	curl -k -X POST -d "actid=$act_id&dir=up&status=fail" $result_uri &> /dev/null

	echo "false:no download uri"

	restore_sqos_bw
	return 1
fi

touch /etc/config/speedtest

upload_speed_test "$up_list" "$up_parallels" "$up_data_size" 

download_speed_test "$down_uri_list" "$down_parallels"

restore_sqos_bw
