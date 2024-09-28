#!/bin/bash

oftrans="/home/pi/Downloads/offline_transaction.zip"
oftransfolder="/home/pi/Downloads"
pendingtran="/home/pi/Downloads/pending_transactions.txt"
fitran="/home/pi/Downloads/finish_transactions.txt"
tranfolder="/home/pi/Downloads/tran/"

case $1 in
charging)
	case $2 in
    -e|-f|-fn)
    # clear up post trans from temporary folder first
    rm -r "$tranfolder"
    mkdir "$tranfolder";;
	-l)
    # list files
    unzip -l $oftrans |grep json| awk '{print $4}' ;;
    -m)
    # move a file into archive
    cd $oftransfolder
    zip -m $oftrans "offline_transaction_$3.json"
    echo $3 >> $pendingtran ;;
    -e)
    # extract a file
    unzip $oftrans "offline_transaction_$3.json" 
    mv "offline_transaction_$3.json" $tranfolder;;
    -r)
    # remove a file
    zip -d $oftrans "offline_transaction_$3.json" ;;
    -f)
    sedcmd="/""$3""/d"
    sed -i "$sedcmd" $pendingtran 
    echo $3 >> $fitran ;;
    -fn)
    # finish a offline transaction
    sedcmd="$3""d"
    sed -i "$sedcmd" $pendingtran ;;
    esac;;
rfid)
    case $2 in
    -local)
    cat /usr/local/etc/nfc/libnfc.conf | grep device.connstring| awk -F = '{print $2}'|sed 's/"//g';;
    -global)
    output=$(cat /etc/nfc/libnfc.conf | grep device.connstring| awk -F = '{print $2}'|sed 's/"//g')
    echo $output;;
    esac;;
init)
    mv ~/upfiles/setting.$ ~/upfiles/setting.ini;;
esac

