#!/bin/bash
#####################################################################################
# @File: userbuild_cyg.sh
# @Desc: File to build U-Boot under cygwin environment. Performs some sanity checks
#        and builds U-Boot, for the selected platforms.
# @Ver:  Version 1.3 (09/July/2011).
#        Version 1.2 (11/March/2007).
#        Version 1.1 (13/Oct/2006).
#        Version 1.0 (25/Oct/2005).
#
# Copyright (c) 2005,2006, 2007, 2011 Digi International Inc.
#####################################################################################

set -e;

#######################################################
## Initialize some variables
#######################################################
declare -r SCRIPTNAME="`basename ${0}`"
declare -r BASEDIR="`cd \`dirname ${0}\`; pwd`"
declare -r AVAILABLE_PLATFORMS="
	ccimx51js
	ccimx51js_dbg
	ccimx51js_test
	ccimx53js
	ccimx53js_dbg
	ccimx53js_test
";
declare CONFIGURE_PRJ="false"
declare BUILD_PRJ="false"
declare PLATFORM
declare UBOOT_IMAGES_INSTALL_PATH

#######################################################
## Print usage information
#######################################################
function usage {
	echo -e "\nUsage: ${SCRIPTNAME} [options] <platform>\n
	-l          List available platforms
	-c          Configure Project
	-b          Build project
	-i <path>   Install path for the U-Boot images"

	echo -e "\nAvailable platforms:\n${AVAILABLE_PLATFORMS}"
}

#######################################################
## Check that the selected platform is in the available
## platforms.
#######################################################
function check_selected_platform {
	declare correct="false"
	for i in : ${AVAILABLE_PLATFORMS}; do
		if [ "x${i}" = "x:" ]; then continue; fi
		if [ "x${i}" = "x${PLATFORM}" ]; then correct="true"; break; fi
	done
	echo ${correct}
}

#######################################################
## List available platforms
#######################################################
function list_available_platforms {
	echo ${AVAILABLE_PLATFORMS}
}

#######################################################
## Get the host build environment. Should be cygwin...
#######################################################
HOSTENV=`uname -s | tr '[:upper:]' '[:lower:]' | sed -e 's/\(cygwin\).*/cygwin/'`
if [ "$HOSTENV" = "cygwin" ]; then
	PATH="/bin:${PATH}";
	export PATH;
else
	echo -e "\nThis script must be used to build U-Boot under cygwin environment\n" >&2;
	exit 1;
fi

#######################################################
## Check if there are enough arguments...
#######################################################
if [ ${#} -le 1 ] ; then
	usage
	exit 1
fi

#######################################################
## Process options...
#######################################################
while getopts "lcbhi:" n; do
	case "${n}" in
		l)
			list_available_platforms
			exit 0 ;;
		c)
			CONFIGURE_PRJ="true";
			;;
		b)
			BUILD_PRJ="true";
			;;
		i)
			if [ "x${OPTARG}" = "x" ] ; then
				echo -e "\nSpecify the installation path for -i option";
				usage
				exit 1
			fi
			UBOOT_IMAGES_INSTALL_PATH="${OPTARG}"
			if [ ! -d ${UBOOT_IMAGES_INSTALL_PATH} ]; then
				echo -e "\nThe specified installation directory ${UBOOT_IMAGES_INSTALL_PATH}"
				echo -e "does not exist\n";
				exit 1
			fi
			;;
		h)
			usage
			exit 1 ;;
	esac;
done;

shift $((${OPTIND} - 1))
cd "${BASEDIR}"
PLATFORM="$1"
#######################################################
## Add CVS Tag into image string
#######################################################
if test -f "CVS-Tag"; then
	VERSION_TAG="`cat CVS-Tag`";
fi;

#######################################################
## Check if the selected platform is available
#######################################################
if ! `check_selected_platform`; then
	echo -e "\nThe selected platform ${PLATFORM} is not available."
	echo -e "\nAvailable platforms:\n${AVAILABLE_PLATFORMS}"
	exit 1
fi

#######################################################
## Configure project for the selected platform
#######################################################
if "${CONFIGURE_PRJ}"; then
	# remove dependencies
	find . -type f -name ".depend" -print0 | xargs -0 rm -f
	make clean

	# configure for selected platform
	eval "make "VERSION_TAG=${VERSION_TAG:=HEAD}" \"\${PLATFORM}_config\"";
fi

#######################################################
## Build
#######################################################
if "${BUILD_PRJ}"; then
	make;
fi

#######################################################
## Rename the build images
#######################################################
if test -f "u-boot.bin"; then
	mv "u-boot.bin" "u-boot-${PLATFORM}.bin";
fi
if test -f "System.map"; then
	mv "System.map" "u-boot-${PLATFORM}.map";
fi

#######################################################
## Install the images
#######################################################
if [ "x${UBOOT_IMAGES_INSTALL_PATH}" != "x" ]; then
	echo -e "\nInstalling images in ${UBOOT_IMAGES_INSTALL_PATH}\n"
	cp "u-boot-${PLATFORM}.bin" ${UBOOT_IMAGES_INSTALL_PATH};
	cp "u-boot-${PLATFORM}.map" ${UBOOT_IMAGES_INSTALL_PATH};
fi

exit 1
