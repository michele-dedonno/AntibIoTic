#!/bin/bash

NAME="antibiotic"

SRC="src" # Source code directory
OUT="bin" # Output directory

# Compiler and strip path for this machines architecture
CC_CURRENT="gcc"
STRIP_CURRENT="strip"

# Compiler and strip path for cross compilation architecture
CC_MIPS="$HOME/buildroot-2018.02.2/output/host/bin/mips-linux-gcc"
STRIP_MIPS="$HOME/buildroot-2018.02.2/output/host/bin/mips-linux-strip"

function compile_release {
	# Compile release version
	# $1 = processor architecture name
	# $2 = compiler path
	# $3 = extra compiler flags
	# $4 = strip path
	filename=${NAME}_release_$1
	echo "Compiling $filename"

	$2 $SRC/*.c -o $OUT/$filename $3 -std=c99 -static -pthread -O3 -fomit-frame-pointer -fdata-sections -ffunction-sections -Wl,--gc-sections
	$4 $OUT/$filename -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag --remove-section=.jcr --remove-section=.got.plt --remove-section=.eh_frame --remove-section=.eh_frame_ptr --remove-section=.eh_frame_hdr
}

function compile_debug {
	# Compile debug version
	# $1 = processor architecture name
	# $2 = compiler path
	# $3 = extra compiler flags
	filename=${NAME}_debug_$1
	echo "Compiling $filename"

	$2 $SRC/*.c -o $OUT/${NAME}_debug_$1 $3 -std=c99 -pthread -g -DDEBUG
}

# Create output directory if it doesn't exist
mkdir -p $OUT

if [ $# == 0 ]; then
	echo "Usage: $0 <debug | release>"

elif [ "$1" == "release" ]; then
	compile_release "current" $CC_CURRENT "-Wall" $STRIP_CURRENT
	compile_release "mips" $CC_MIPS "" $STRIP_MIPS

elif [ "$1" == "debug" ]; then
	compile_debug "current" $CC_CURRENT "-Wall -fsanitize=address"
	# The "-DDEBUGFILE" flag will tell the program to redirect debugging output to "debug.txt"
	compile_debug "mips" $CC_MIPS "-static -DDEBUGFILE"

else
	echo "Unknown parameter $1: $0 <debug | release>"
fi
