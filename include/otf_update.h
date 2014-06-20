/*
 *  Copyright (C) 2014 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

#ifndef __OTF_UPDATE_H
#define __OTF_UPDATE_H

/* OTF flags */
#define OTF_FLAG_FLUSH 		(1 << 0)
#define OTF_FLAG_INIT 		(1 << 1)

typedef struct otf_data {
	unsigned int loadaddr;
	unsigned int offset;
	unsigned char *buf;
	unsigned int len;
	disk_partition_t *part;
	unsigned int flags;
}otf_data_t;

#endif  /* __OTF_UPDATE_H */
