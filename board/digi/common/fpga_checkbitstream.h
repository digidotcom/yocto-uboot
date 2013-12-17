/*
 *  common/digi/fpga_checkbitstream.h
 *
 *  Copyright (C) 2006 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
 */
/*
 *  !Revision:   $Revision$
 *  !Author:     Markus Pietrek
 *  !Descr:      Defines the functions to access the DIGI fpga
 */

#ifndef __DIGI_FPGA_CHECKBITSTREAM_H
#define __DIGI_FPGA_CHECKBITSTREAM_H

#define LOAD_FPGA_OK	0
#define LOAD_FPGA_FAIL	1

extern int fpga_checkbitstream( uchar* fpgadata, ulong size );
extern ulong fpga_hw_version, fpga_fw_version, checksum_calc, checksum_read, fpgadatasize;

#endif /* __DIGI_FPGA_CHECKBITSTREAM_H */
