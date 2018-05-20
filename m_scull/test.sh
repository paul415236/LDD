#!/bin/bash

sudo rmmod scull
sudo insmod scull.ko
sudo ./nod.sh

cd ../test/scull/
./test

cd -

