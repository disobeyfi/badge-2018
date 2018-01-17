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

typedef struct
{
	uint8_t key[16];
} badgekey_t;

void key_gen(badgekey_t *key);
void key_reset(badgekey_t *key);
void key_printbytes(badgekey_t *key);
void key_gethex(badgekey_t *key, char *hexdata);
void key_fromhex(badgekey_t *key, char *hexdata);
void key_combine(badgekey_t *key1, badgekey_t *key2);
uint32_t key_gethash(badgekey_t *key);
