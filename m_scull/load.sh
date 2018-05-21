#!/bin/bash

MODULE=scull
MODE=644

sudo insmod ${MODULE}.ko

# retrieve device major
major=$(cat /proc/devices | grep $MODULE | awk '{print $1}')
echo "scull major: $major"

rm -f /dev/${MODULE}[0-3]
for i in {0..3};
do
	mknod /dev/${MODULE}$i c $major $i
	chown paul:paul /dev/${MODULE}$i
	chmod 777 /dev/${MODULE}$i
done
