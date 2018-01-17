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

extern badgekey_t mykey;
extern uint32_t keycombos[];

int main(int argc, char **argv)
{
	badgekey_t key;
	key_reset(&key);

	{
		char keyhex[33];
		keyhex[32] = 0;
		key_gethex(&mykey, keyhex);
		printf("my key: '%s'\n", keyhex);
	}


	for (int i = 1; i < argc; i++)
	{
		char keyhex[33];
		keyhex[32] = 0;

		strncpy(keyhex, argv[i], 32);

		badgekey_t argkey;
		key_fromhex(&argkey, argv[i]);
		key_combine(&key, &argkey);
	}

	int thiscombo = -1;
	for (int i = 0; i < 256; i++)
	{
		if (key_gethash(&key) == keycombos[i])
		{
			if (thiscombo != -1)
			{
				printf("overmatch!\n");
				exit(1);
			}

			thiscombo = i;

		}
	}

	printf("this is combo %i\n", thiscombo);

	if (thiscombo >= 0)
	{
		char keyhex[33];
		keyhex[32] = 0;
		key_gethex(&key, keyhex);
		printf("outputting: '%s'\n", keyhex);
	}

	return 0;
}
