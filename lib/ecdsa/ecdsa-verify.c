// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2020, Alexandru Gagniuc <mr.nuke.me@gmail.com>
 */

#include <u-boot/ecdsa.h>

int ecdsa_verify(struct image_sign_info *info,
		 const struct image_region region[], int region_count,
		 uint8_t *sig, uint sig_len)
{
	return -EOPNOTSUPP;
}
