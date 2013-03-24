#!/bin/sh

#add by juguofeng	2013-01-28

cd jni/liveTV/

ANDROID="android"
LINUX="linux"

COLOR_RS="\033[31;49;1m"
COLOR_BS="\033[34;49;1m"
COLOR_E="\033[39;49;0m"

usage() {
	echo $COLOR_RS"please input the correct <os-platform>"$COLOR_E
	echo "Usage: sh $0 <os-platform>"
	echo "=========>"$COLOR_BS"android"$COLOR_E "or" $COLOR_BS"linux"$COLOR_E
	exit 1
}

if [ $# -ne 1 ]
then
	usage $*
fi

echo "/* auto create by compile.sh, do not modify */" > config_jni.h
echo "" >> config_jni.h

echo "#ifndef __CONFIG_JNI_H" >> config_jni.h
echo "#define __CONFIG_JNI_H" >> config_jni.h

if [ "$ANDROID" = "$1" ]
then
	echo "" >> config_jni.h
	echo "#define __ANDROID_" >> config_jni.h
	echo "" >> config_jni.h
	echo "#endif" >> config_jni.h
	echo ""
	echo $COLOR_BS"======>compiling for Android..."$COLOR_E
	echo ""
	ndk-build
	#rm config_jni.h
	exit
fi

if [ "$LINUX" = "$1" ]
then
	echo "" >> config_jni.h
	echo "//#define __ANDROID_" >> config_jni.h
	echo "" >> config_jni.h
	echo "#endif" >> config_jni.h
	echo ""
	echo $COLOR_BS"======>compiling for Linux..."$COLOR_E
	echo ""
	make
	#rm config_jni.h
	exit
fi

#rm config_jni.h
usage $*

