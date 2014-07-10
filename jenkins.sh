#!/bin/bash
#===============================================================================
#
#  jenkins.sh
#
#  Copyright (C) 2014 by Digi International Inc.
#  All rights reserved.
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License version 2 as published by
#  the Free Software Foundation.
#
#  Description: U-Boot jenkins build script
#
#  Parameters from the environment (not all needed):
#    DUB_PLATFORMS:      Platforms to build
#    DUB_REVISION:       Revision of the U-Boot repository
#    DUB_GIT_URL:        U-Boot repo url
#    DUB_TOOLCHAIN_URL:  Toolchain installer base url
#
#===============================================================================

set -e

AVAILABLE_PLATFORMS=" \
	ccimx6adpt \
	ccimx6sbc \
"

# <platform> <uboot_make_target>
while read pl mt; do
	eval "${pl}_make_target=\"${mt}\""
done<<-_EOF_
	ccimx6adpt    u-boot.imx
	ccimx6sbc     u-boot.imx
_EOF_

# Set default values if not provided by Jenkins
DUB_GIT_URL="${DUB_GIT_URL:-ssh://git@stash.digi.com/uboot/u-boot-denx.git}"
DUB_TOOLCHAIN_URL="${DUB_TOOLCHAIN_URL:-http://build-linux.digi.com/yocto/toolchain}"
DUB_PLATFORMS="${DUB_PLATFORMS:-$(echo ${AVAILABLE_PLATFORMS})}"

error() {
	printf "${1}"
	exit 1
}

# Sanity check (Jenkins environment)
[ -z "${WORKSPACE}" ] && error "WORKSPACE not specified"

printf "\n[INFO] Build U-Boot \"${DUB_REVISION}\" for \"${DUB_PLATFORMS}\"\n\n"

# Remove nested directories from the revision
DUB_REVISION_SANE="$(echo ${DUB_REVISION} | tr '/' '_')"

DUB_IMGS_DIR="${WORKSPACE}/images"
DUB_TOOLCHAIN_DIR="${WORKSPACE}/toolchain"
DUB_UBOOT_DIR="${WORKSPACE}/u-boot${DUB_REVISION_SANE:+-${DUB_REVISION_SANE}}.git"
rm -rf ${DUB_IMGS_DIR} ${DUB_TOOLCHAIN_DIR} ${DUB_UBOOT_DIR}

mkdir -p ${DUB_IMGS_DIR} ${DUB_TOOLCHAIN_DIR} ${DUB_UBOOT_DIR}
if pushd ${DUB_UBOOT_DIR}; then
	# Install U-Boot
	git clone ${DUB_GIT_URL} .
	if [ -n "${DUB_REVISION}" -a "${DUB_REVISION}" != "master" ]; then
		git checkout ${DUB_REVISION}
	fi

	# Install toolchain
	for tlabel in ${DUB_REVISION_SANE} default; do
		if wget -q --spider "${DUB_TOOLCHAIN_URL}/toolchain-${tlabel}.sh"; then
			printf "\n[INFO] Installing toolchain-${tlabel}.sh\n\n"
			tmp_toolchain="$(mktemp /tmp/toolchain.XXXXXX)"
			wget -q -O ${tmp_toolchain} "${DUB_TOOLCHAIN_URL}/toolchain-${tlabel}.sh"
			sh ${tmp_toolchain} -y -d ${DUB_TOOLCHAIN_DIR}
			rm -f ${tmp_toolchain}
			break
		fi
	done

	eval $(grep "^export CROSS_COMPILE=" ${DUB_TOOLCHAIN_DIR}/environment-setup-*)
	eval $(grep "^export PATH=" ${DUB_TOOLCHAIN_DIR}/environment-setup-*)
	eval $(grep "^export SDKTARGETSYSROOT=" ${DUB_TOOLCHAIN_DIR}/environment-setup-*)

	CPUS="$(echo /sys/devices/system/cpu/cpu[0-9]* | wc -w)"
	[ ${CPUS} -gt 1 ] && MAKE_JOBS="-j${CPUS}"

	for platform in ${DUB_PLATFORMS}; do
		printf "\n[PLATFORM: ${platform} - CPUS: ${CPUS}]\n"

		# We need to explicitly pass the CC variable. Otherwise u-boot discards the
		# '--sysroot' option and the build fails
		eval UBOOT_MAKE_TARGET=\"\${${platform}_make_target}\"
		make distclean
		make ${platform}_config
		make ${MAKE_JOBS} CC="${CROSS_COMPILE}gcc --sysroot=${SDKTARGETSYSROOT}" ${UBOOT_MAKE_TARGET}

		# Copy u-boot image
		cp ${UBOOT_MAKE_TARGET} ${DUB_IMGS_DIR}/${UBOOT_MAKE_TARGET/u-boot/u-boot-${platform}}
	done

	popd
fi
