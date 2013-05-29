#!/bin/bash
#===============================================================================
#
#  build.sh
#
#  Copyright (C) 2010 by Digi International Inc.
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
	ccardimx28js_261MHz
	ccardimx28js_360MHz
	ccardimx28js_dbg
	ccardimx28js_legacynames
	ccardimx28js_test
	ccimx51js
	ccimx51js_128sdram
	ccimx51js_128sdram_dbg
	ccimx51js_128sdram_ext_eth
	ccimx51js_128sdram_legacynames
	ccimx51js_128sdram_test
	ccimx51js_128sdram_test_dbg
	ccimx51js_dbg
	ccimx51js_db_pa
	ccimx51js_db_pp
	ccimx51js_db_ra
	ccimx51js_db_rp
	ccimx51js_EAK
	ccimx51js_ext_eth
	ccimx51js_legacynames
	ccimx51js_test
	ccimx51js_test_dbg
	ccimx53js
	ccimx53js_128sdram
	ccimx53js_128sdram_dbg
	ccimx53js_128sdram_legacynames
	ccimx53js_128sdram_test
	ccimx53js_128sdram_test_dbg
	ccimx53js_4Kpage
	ccimx53js_dbg
	ccimx53js_db_pa
	ccimx53js_db_pp
	ccimx53js_db_ra
	ccimx53js_db_rp
	ccimx53js_ext_eth
	ccimx53js_legacynames
	ccimx53js_test
	ccimx53js_test_dbg
	cpx2
	cpx2_128sdram
	wr21
"

# <platform> <toolchain> <bootstreams>
while read pl to bs; do
	eval "${pl}_toolchain=\"${to}\""
	eval "${pl}_bootstream=\"${bs}\""
done<<-_EOF_
        ccardimx28js    arm-unknown-linux-gnueabi       y
        ccimx51js       arm-cortex_a8-linux-gnueabi     n
        ccimx53js       arm-cortex_a8-linux-gnueabi     n
        cpx2            arm-unknown-linux-gnueabi       y
        wr21            arm-unknown-linux-gnueabi       y
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

OLDPATH="${PATH}"

CPUS="$(echo /sys/devices/system/cpu/cpu[0-9]* | wc -w)"
[ ${CPUS} -gt 1 ] && MAKE_JOBS="-j${CPUS}"

for platform; do
	eval _toolchain_str=\"\${${platform%%_*}_toolchain}\"
	eval _has_bootstreams=\"\${${platform%%_*}_bootstream}\"
	export PATH="${toolchain_dir}/x-tools/${_toolchain_str}/bin:${OLDPATH}"

	printf "\n[PLATFORM: ${platform} - PATH: ${PATH}]\n"

	make distclean
	make "${platform}_config"
	make ${MAKE_JOBS}
	if [ -d "${install_dir}" ]; then
		cp "u-boot-${platform}.bin" "${install_dir}/"
		cp System.map "${install_dir}/u-boot-${platform}.map"
		if [ "${_has_bootstreams}" = "y" ]; then
			cp "u-boot-${platform}.sb" "${install_dir}/"
			cp "u-boot-${platform}-ivt.sb" "${install_dir}/"
		fi
	fi
done
