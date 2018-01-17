/*
 * This file is part of Disobey Badge 2018 firmware.
 *
 * Disobey Badge 2018 firmware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Disobey Badge 2018 firmware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Disobey Badge 2018 firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "badgekey.h"

extern void sha256_hash(const char *data, size_t len, char *res);

void key_gen(badgekey_t *key)
{
	for (int x = 0; x < 16; x++)
	{
		key->key[x] = rand();
	}
}

void key_reset(badgekey_t *key)
{
	for (int x = 0; x < 16; x++)
	{
		key->key[x] = 0;
	}
}

void key_printbytes(badgekey_t *key)
{
	for (int x = 0; x < 16; x++)
	{
		printf("%i, ", key->key[x]);
	}
}

char *hexchars = "0123456789ABCDEF";

void key_gethex(badgekey_t *key, char *hexdata)
{
	for (int x = 0; x < 16; x++)
	{
		hexdata[x*2+0] = hexchars[(key->key[x] >> 4) & 0xF];
		hexdata[x*2+1] = hexchars[(key->key[x] >> 0) & 0xF];
	}
}

void key_fromhex(badgekey_t *key, char *hexdata)
{
	for (int x = 0; x < 16; x++)
	{
		int v1 = hexdata[x*2+0];
		int v2 = hexdata[x*2+1];

		int n1 = strchr(hexchars, v1) - hexchars;
		int n2 = strchr(hexchars, v2) - hexchars;

		key->key[x] = (n1 << 4) | (n2);
	}
}


void key_combine(badgekey_t *key1, badgekey_t *key2)
{
	for (int i = 0; i < 16; i++)
	{
		key1->key[i] ^= key2->key[i];
	}
}

uint32_t key_gethash(badgekey_t *key)
{
	uint8_t sha256[32];
	uint32_t *ourhash = (uint32_t *)sha256;

	sha256_hash((const char *)key->key, 16, (char *)sha256);

	return *ourhash;
}
