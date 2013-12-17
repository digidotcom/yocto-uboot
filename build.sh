#!/bin/bash
#===============================================================================
#
#  build.sh
#
#  Copyright (C) 2013 by Digi International Inc.
#  All rights reserved.
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License version 2 as published by
#  the Free Software Foundation.
#
#
#  !Description: Interface script for U-Boot autobuild
#
#===============================================================================

set -e

basedir="$(cd $(dirname ${0}) && pwd)"
toolchain_dir="$(cd ${basedir}/.. && pwd)/toolchain"

buildserver="http://build-linux.digi.com/U-Boot/toolchain"

declare -r AVAILABLE_PLATFORMS="
	ccardimx28js
	cpx2
"

# <platform> <toolchain> <bootstreams>
while read pl to bs; do
	eval "${pl}_toolchain=\"${to}\""
	eval "${pl}_bootstream=\"${bs}\""
done<<-_EOF_
        ccardimx28js    arm-unknown-linux-gnueabi       y
        cpx2            arm-unknown-linux-gnueabi       y
_EOF_

while getopts "i:" c; do
	case "${c}" in
		i) install_dir="${OPTARG%/}";;
	esac
done
shift $((${OPTIND} - 1))

# install directory is mandatory
if [ -z "${install_dir}" ]; then
	printf "\n[ERROR] missing \"-i\" (install_dir) option (mandatory)\n\n"
	exit 1
fi

cd "${basedir}"

if [ ! -f .toolchain_installed ]; then
	# DENX toolchain is the default for u-boot-denx repository
	toolchain_tag="denx"
	if uboot_tag="$(git name-rev --tags --name-only --no-undefined HEAD 2>/dev/null)"; then
		uboot_tag="${uboot_tag%%^*}"
	fi
	if uboot_branch="$(git rev-parse --symbolic-full-name @{u} 2>/dev/null)"; then
		uboot_branch="$(echo ${uboot_branch#refs/remotes/$(git remote)/} | tr '/' '_')"
	fi
	for i in ${uboot_tag} ${uboot_branch}; do
		if wget -q --spider "${buildserver}/toolchain-${i}.tar.gz"; then
			toolchain_tag="${i}"
			printf "\n[INFO] Installing toolchain-${i}.tar.gz\n\n"
			break
		else
			printf "\n[WARNING] File toolchain-${i}.tar.gz not found on webserver.\n\n"
		fi
	done
	rm -rf ${toolchain_dir} && mkdir ${toolchain_dir}
	wget -q -O - "${buildserver}/toolchain-${toolchain_tag}.tar.gz" | tar xz -C ${toolchain_dir} -f -
	touch .toolchain_installed
fi

if [ "${#}" = "0" ]; then
	set ${AVAILABLE_PLATFORMS}
fi

CPUS="$(echo /sys/devices/system/cpu/cpu[0-9]* | wc -w)"
[ ${CPUS} -gt 1 ] && MAKE_JOBS="-j${CPUS}"

for platform; do
	eval _toolchain_str=\"\${${platform%%_*}_toolchain}\"
	eval _has_bootstreams=\"\${${platform%%_*}_bootstream}\"

	UBOOT_MAKE_TARGET=""
	[ "${_has_bootstreams}" = "y" ] && UBOOT_MAKE_TARGET="u-boot.sb"

	# Export the path and build in a sub-shell to avoid adding recursively
	# more path's to PATH environment variable in case we build more than
	# one platform.
	(
		export PATH="${toolchain_dir}/x-tools/${_toolchain_str}/bin:${PATH}"
		printf "\n[PLATFORM: ${platform} - PATH: ${PATH}]\n"
		make distclean
		make "${platform}_config"
		make ${MAKE_JOBS} ${UBOOT_MAKE_TARGET}
	)

	if [ -d "${install_dir}" ]; then
		cp u-boot.map "${install_dir}/u-boot-${platform}.map"
		if [ "${_has_bootstreams}" = "y" ]; then
			cp "u-boot.sb" "${install_dir}/u-boot-${platform}.sb"
		else
			cp "u-boot.bin" "${install_dir}/u-boot-${platform}.bin"
		fi
	fi
done
