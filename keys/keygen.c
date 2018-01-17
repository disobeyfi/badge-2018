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

#include "badgekey.h"

#define KEYCOUNT 9
#define KEYCOMBOCOUNT (1 << KEYCOUNT)

int main(void)
{
	badgekey_t keys[KEYCOUNT];

	srand(0xd150b39);

	fprintf(stderr, "generating keys:\n");

	for (int i = 0; i < KEYCOUNT; i++)
	{
		key_gen(&keys[i]);
	}

	for (int i = 0; i < KEYCOUNT; i++)
	{
		uint32_t hash = key_gethash(&keys[i]);

		char keyhex[33];
		keyhex[32] = 0;
		key_gethex(&keys[i], keyhex);

		fprintf(stderr, "key %i: '%s', hash %08X\n", i, keyhex, hash);
	}

	badgekey_t keycombos[KEYCOMBOCOUNT];
	uint32_t keycombohashes[KEYCOMBOCOUNT];

	for (int i = 0; i < KEYCOMBOCOUNT; i++)
	{
		badgekey_t tmpkey;
		key_reset(&tmpkey);

		for (int x = 0; x < KEYCOUNT; x++)
		{
			if (i & (1 << x))
				key_combine(&tmpkey, &keys[x]);
		}

		uint32_t hash = key_gethash(&tmpkey);

		//printf("hash for combo %i: %08X\n", i, hash);
		keycombos[i] = tmpkey;
		keycombohashes[i] = hash;
	}

	fprintf(stderr, "\nchecking for duplicates..\n");

	for (int i = 0; i < KEYCOMBOCOUNT; i++)
	{
		for (int j = 0; j < KEYCOMBOCOUNT; j++)
		{
			if (i != j)
			{
				if (keycombohashes[i] == keycombohashes[j])
				{
					fprintf(stderr, "collision between %i and %i\n", i, j);
					return 2;
				}
			}
		}

	}

	fprintf(stderr, "\nresulting dadas:\n");

	printf("#include \"badgekey.h\"\n");

	for (int i = 0; i < KEYCOUNT; i++)
	{
		printf("#ifdef BADGETYPE%i\n", i);
		printf("badgekey_t mykey = { {");
		key_printbytes(&keys[i]);
		printf("} }; // hash %08X\n", key_gethash(&keys[i]));
		printf("#endif\n");
	}
	printf("\n");

	printf("uint32_t keycombos[] = {\n");
	for (int i = 0; i < KEYCOMBOCOUNT; i++)
	{
		char keyhex[33];
		keyhex[32] = 0;
		key_gethex(&keycombos[i], keyhex);

		printf("0x%08X, // % 3i, '%s'\n", keycombohashes[i], i, keyhex);
	}
	printf("};\n");

	return 0;
}