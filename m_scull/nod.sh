#!/bin/bash

MODULE=scull
MODE=644



major=$(cat /proc/devices | grep $MODULE | awk '{print $1}')
echo $major
