time=$1

iptables -t mangle -N powerport &> /dev/null
iptables -t mangle -F powerport
iptables -t mangle -nvL PREROUTING | grep 'powerport' &> /dev/null
if [ $? -ne 0 ] ;then
	iptables -t mangle -I PREROUTING 1 -i br-lan -p tcp  -j powerport
fi

shift
for i in $*
do
	iptables -t mangle -A powerport -i br-lan -p tcp --dport $i -j ACCEPT
done

iptables -t mangle -A powerport -i br-lan -j DROP
sleep $time

iptables -t mangle -F powerport
