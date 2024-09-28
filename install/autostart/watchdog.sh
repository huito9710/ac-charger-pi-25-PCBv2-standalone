#!/bin/bash

retry=3

if [ $retry -eq -1 ]
then
	echo "exit watchdog program"
	exit
fi
i=0
while [ $i -lt $retry ]
do
	sleep 30

	pgrep ChargerV2
	if [ $? -eq 1 ]
	then
		echo "ChargerV2 not running."
		i=$(($i+1))
		echo $i
	else
		echo "ChargerV2 is running, exit now"
		exit
	fi
done
echo "reboot now"
reboot