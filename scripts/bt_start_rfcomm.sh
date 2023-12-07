BD_ADDR=$1

if [ -z $BD_ADDR ];
then
	BD_ADDR="a4:cf:12:43:55:16"
	echo "Using default bt address $BD_ADDR"
	echo "Please define your own BT address as argument by running:"
	echo "./bt_start_rfcomm.sh <bt address>"
fi

sudo rfcomm connect /dev/rfcomm $BD_ADDR
