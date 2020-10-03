#!/bin/bash

# 
# Helper script for building AntibIoTic Bot binaries.
#	1- release: compile the release binary of the Agent 
#	2- debug: compile the debug binary of the Agent
#	3- test <subfolder>: compile the tests
#

# %---- To verify before execution ----%
NAME="antibiotic"

SRC="../src" # Source code directory
OUT="bin" # Output directory
TST="tests" # Tests source code subdirectory 	 

# Compiler and strip path for this machines architecture
CC_CURRENT="gcc"
STRIP_CURRENT="strip"

# Compiler and strip path for cross compilation architecture
CC_MIPS="$HOME/buildroot-2018.02.7/output/host/bin/mips-linux-gcc"
STRIP_MIPS="$HOME/buildroot-2018.02.7/output/host/bin/mips-linux-strip"
# % -------------------------------------%

function compile_release {
	# Compile release version
	# $1 = processor architecture name
	# $2 = compiler path
	# $3 = extra compiler flags
	# $4 = strip path
	filename=${NAME}_release_$1
	echo "[BUILD:release]> Compiling $filename with the following commands:"
	echo -e "\t '$2 $SRC/*.c -o $OUT/$filename $3 -std=c99 -static -pthread -O3 -fomit-frame-pointer -fdata-sections -ffunction-sections -Wl,--gc-sections'"
	echo -e "\t '$4 $OUT/$filename -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag --remove-section=.jcr --remove-section=.got.plt --remove-section=.eh_frame --remove-section=.eh_frame_ptr --remove-section=.eh_frame_hdr'"
	$2 $SRC/*.c -o $OUT/$filename $3 -std=c99 -static -pthread -O3 -fomit-frame-pointer -fdata-sections -ffunction-sections -Wl,--gc-sections
	$4 $OUT/$filename -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag --remove-section=.jcr --remove-section=.got.plt --remove-section=.eh_frame --remove-section=.eh_frame_ptr --remove-section=.eh_frame_hdr

	echo "[BUILD:release]> Output folder: '.$OUT/'"
}


function compile_debug {
	# Compile debug version
	# $1 = processor architecture name
	# $2 = compiler path
	# $3 = extra compiler flags
	filename=${NAME}_debug_$1
	echo "[BUILD:debug]> Compiling $filename with the following command:"
	echo -e "\t '$2 $SRC/*.c -o $OUT/$filename $3 -std=c99 -pthread -g -DDEBUG'"

	$2 $SRC/*.c -o $OUT/$filename $3 -std=c99 -pthread -g -DDEBUG

	echo "[BUILD:debug]> Output folder: '.$OUT/'"
}

function compile_test {
	# Compile test
	# $1 = processor architecture name
	# $2 = compiler path
	# $3 = extra compiler flags
	# $4 = test subfolder
	filename=${NAME}_$4_test_$1

	echo "[BUILD:test]> Compiling $filename with the following command:"
	echo -e "\t '$2 $SRC/$TST/$4/*.c -o $OUT/$TST/$filename $3 -std=c99 -pthread -g -DDEBUG'"

	$2 $SRC/$TST/$4/*.c -o $OUT/$TST/$filename $3 -std=c99 -pthread -g -DDEBUG

	echo "[BUILD:test]> Output folder: '.$OUT/$TST'"
}


# Create output directory if it doesn't exist
mkdir -p $OUT

if [ $# == 0 ]; then
	echo "[build.sh]> Usage: $0 debug | release | test <source-subfolder>"

elif [ "$1" == "release" ]; then
	compile_release "local" $CC_CURRENT "-Wall" $STRIP_CURRENT
	compile_release "mips" $CC_MIPS "" $STRIP_MIPS

elif [ "$1" == "debug" ]; then
	compile_debug "local" $CC_CURRENT "-Wall -fsanitize=address"
	# The "-DDEBUGFILE" flag will tell the program to redirect debugging output to "debug.txt"
	compile_debug "mips" $CC_MIPS "-static -DDEBUGFILE"

elif [ $# == 2 ] && [ "$1" == "test" ]; then
	mkdir -p $OUT/$TST
	# Compile tests in the 'mem' folder
	compile_test "local" $CC_CURRENT "-Wall -fsanitize=address" $2 
	compile_test "mips" $CC_MIPS "-static -DDEBUGFILE" $2
else
	echo "[build.sh]> Unknown or wrong parameter(s). Usage: $0 debug | release | test <source-subfolder>"
fi
