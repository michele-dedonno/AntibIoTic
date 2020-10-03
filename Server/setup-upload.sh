#!/bin/bash
# Helper script that copies required files for the AntibIoTic gateway to run

# ------------- Variables to be edited ------------- #
DST="./.upload" # path to the gateway upload folder
SRC="../Agent" # path to the Agent folder
# ------------- ------------- ------------- -------- #

# Generating and copying files required for NETGEAR Router
mkdir -p $DST/netgear/
cp $SRC/netgear-mips/upload.py $DST/netgear/upload.py
CDIR=$(pwd)
cd ./$SRC/netgear-mips/
./build.sh debug
sleep 2
cd $CDIR
cp $SRC/netgear-mips/bin/antibiotic_debug_mips $DST/netgear/antibiotic_debug_mips

# Generating and copying files required for UDOO x86
mkdir -p $DST/udoo/
cp -r $SRC/src/ $DST/udoo/
cp $SRC/udoo-x86/upload.py $DST/udoo/

# Generating and copying files required for Raspberry Pi
mkdir -p $DST/raspberry/
cp -r $SRC/src/ $DST/raspberry/
cp $SRC/raspberry-arm/upload.py $DST/raspberry/
