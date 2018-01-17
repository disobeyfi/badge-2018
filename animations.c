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

#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include <math.h>

#include <string.h>

#include "animations.h"
#include "lights.h"
#include "debugf.h"

#include "utils.h"


typedef struct 
{
	float xp,yp,ang;
} ledpos_t;

ledpos_t ledpos[14] = 
{
{ 28.74f, 4.21f,55  },
{ 64.68f,50.61f,270 },
{ 55.58f,28.29f,90  },
{ 64.07f,28.69f,135 },
{ 38.83f,29.15f,60  },
{ 64.39f,17.84f,135 },
{ 69.04f,31.49f,135 },
{ 73.81f, 8.35f,138 },
{ 32.01f,48.15f,315 },
{ 63.27f,66.69f,180 },
{ 43.92f,51.09f,270 },
{ 44.15f,27.12f,60  },
{ 83.02f,55.77f,225 },
{ 36.93f,15.77f,55  },
};

static int ledmap(float in, float low, float high)
{
	float mm = (in - low) / (high - low);

	mm = mm < 0.f ? 0.f : (mm > 1.f ? 1.f : mm);

	mm *= 2048;

	int val = mm;
	val = val < 0 ? 0 : (val > 2047 ? 2047 : val);
	return val;
}


static struct {
	int up;
	int down;
	int downpos;

	int state;
	int curbright;
} faderp;

static void faderinit(int p1, int p2, int p3)
{
	faderp.up = 2048 / p1;
	faderp.down = 2048 / p3;
	faderp.downpos = p2;

	faderp.state = 0;
	faderp.curbright = 0;
}

static void fader(int p)
{
	if (faderp.state == 0)
	{
		faderp.curbright += faderp.up;
		if (faderp.curbright > 2047)
		{
			faderp.state = 1;
			faderp.curbright = 2047;
		}
	}
	else if (faderp.state == 1 && p >= faderp.downpos)
		faderp.curbright -= faderp.down;

	lights_limit(faderp.curbright);
}





static void anim_randomflashes_fast()
{
	lights_setup();

	int count = 80 + (random() % 80);

	faderinit(40, count-40, 40);

	int andy = (1 << (3 + (random() % 3))) - 1;

	for (int i = 0; i < count; i++)
	{
	int ledstate = 0;
		if ((i & andy) < 2)
			ledstate = random();


		fader(i);
		lights_set(ledstate);

		sleep_cpu(0x50);
	}
	lights_kill();
}

static void anim_randomtoggle()
{
	lights_setup();

	int count = 50 + (random() % 80);

	faderinit(25, count-25, 25);

	for (int i = 0; i < count; i++)
	{
		int ledstate = 0;
		int ledidx = random() % (ledcount*2);

		fader(i);

		if (ledidx < ledcount)
			ledstate |= 1 << ledidx;
		else
			ledstate &= ~(1 << (ledidx - ledcount));

		lights_set(ledstate);

		sleep_cpu(0x50);
	}


	lights_kill();
}

static void anim_offtoggles()
{
	lights_setup();

	for (int i = 0; i < 4; i++)
	{
//		while(buttons_read() == 0)
//			continue;

		int arv = random();


		for ( int j = 0; j < 2048; j+=200)
		{
			lights_limit(j);
			lights_set(arv);
			sleep_cpu(200);
		}
		lights_limit(2047);

#if 0
		while(arv)
		{
			lights_set(arv);
			sleep_cpu(460 + random() % 800);
			arv &= random();
		}
#endif

		while(arv)
		{
			lights_set(arv);


			int br[18];
			for ( int j = 0; j < 18; j++)
				br[j] = arv & ( 1 << j ) ? 2047 : 0;


			sleep_cpu(300 + random() % 500);

			arv &= random();

			int dropspeed = 150 + (random() % 150);
			int droplen = 2048 / dropspeed;

			for ( int k = 0; k < droplen; k++ )
			{
				for ( int j = 0; j < 18; j++)
				{
					if (br[j] > 0)
						br[j] -= arv & ( 1 << j ) ? 0 : dropspeed;
					else if (br[j] < 0)
						br[j] = 0;
				}

				lights_set_pwm(br);

				sleep_cpu(50);
			}

//			sleep_cpu(460 + random() % 800);
		}

		lights_set(0);

//		int uh = random() % 200000;
		sleep_cpu(700 + random() % 1000);
	}
	lights_kill();
}

static void anim_radiate()
{
	lights_setup();

	float t = (random() % 64) / 64.f;

	int titers = 50 + (random() % 100);

	faderinit(25, titers-25, 25);


	for (int ti = 0; ti < titers; ti++)
	{
		int bright[14];

		fader(ti);

		for (int i = 0; i < 14; i++)
		{
			//int val = random() % 2000;
			//float valf = 0.5f + sin(i * 1.6341f + t) * 0.5f;

//			float xd = ledpos[i].xp - 50.f;
//			float yd = ledpos[i].yp - 60.f;
			float xd = ledpos[i].xp - 20.f;
			float yd = ledpos[i].yp - 70.f;

			float d = sqrtf(xd*xd+yd*yd);

			float valf = 0.5f + sinf(d * 0.1f - t) * 0.5f;
			valf = valf * valf;

			float val = ledmap(valf, 0.5f, 1.f);

			bright[i] = val;


		}
		//bright[1] = random() % 2000;

			t += 0.1f;

			lights_set_pwm(bright);


			sleep_cpu(100);

	}
	lights_kill();
}


static void anim_mover()
{
	lights_setup();

	float tx = (random() % 64) / 64.f;
	float ty = (random() % 64) / 64.f;

	tx *= 3.14159f * 2.f;
	ty *= 3.14159f * 2.f;

	float tx2 = (random() % 64) / 64.f;
	float ty2 = (random() % 64) / 64.f;

	tx2 = tx2 * 0.1f + 0.1f;
	ty2 = ty2 * 0.1f + 0.1f;

	if (random() & 1)
		tx2 = -tx2;
	if (random() & 1)
		ty2 = -ty2;

	int titers = 50 + (random() % 100);

	faderinit(25, titers-25, 25);


	for (int ti = 0; ti < titers; ti++)
	{

		fader(ti);

		int bright[14];
		for (int i = 0; i < 14; i++)
		{
			//int val = random() % 2000;
			//float valf = 0.5f + sin(i * 1.6341f + t) * 0.5f;

			float xc = 55.f + sinf(tx) * 35.f;
			float yc = 30.f + sinf(ty) * 25.f;

//			xc += sin(tx2) * 20.f;
//			yc += sin(ty2) * 20.f;


			float xd = ledpos[i].xp - xc;
			float yd = ledpos[i].yp - yc;

			float d = sqrtf(xd*xd+yd*yd);

			bright[i] = ledmap(d, 25.f, 0.f);
		}

//		tx += 0.1128391823f;
//		ty += 0.19498128f;
		tx += tx2;
		ty += ty2;

		lights_set_pwm(bright);


			sleep_cpu(100);

	}
	lights_kill();
}

static void anim_plane()
{
	lights_setup();

#if 0
	float tx = (random() % 64) / 32.f;
	float ty = (random() % 64) / 32.f;
	tx -= 1.f;
	ty -= 1.f;
#else
	float ang = ((random() % 256) / 256.f) * 3.14159f * 2.f;
	float tx = cosf(ang)*0.1f;
	float ty = sinf(ang)*0.1f;
#endif

	int titers = 40 + (random() % 40);

	faderinit(20, titers-20, 20);

	for (int ti = 0; ti < titers; ti++)
	{
		fader(ti);

		static float t = 0.f;
		int bright[14];
		for (int i = 0; i < 14; i++)
		{

			float xd = ledpos[i].xp * tx;
			float yd = ledpos[i].yp * ty;

			float d = sinf(xd+yd+t);

			bright[i] = ledmap(d, 0.5f, 1.0f);
		}

		t += 0.4f;

		lights_set_pwm(bright);

		sleep_cpu(100);
	}
	lights_kill();
}


static void anim_rotplane()
{
	lights_setup();

	int titers = 100 + (random() % 100);

	faderinit(25, titers-25, 25);

	for (int ti = 0; ti < titers; ti++)
	{
		static float t = 0.f;

		fader(ti);

	float ang = t;
	float tx = cosf(ang)*0.1f;
	float ty = sinf(ang)*0.1f;

		int bright[14];
		for (int i = 0; i < 14; i++)
		{

			float xd = (ledpos[i].xp-50) * tx;
			float yd = (ledpos[i].yp-30) * ty;

			float d = sinf(xd+yd);

			bright[i] = ledmap(d, 0.5f, 1.0f);


			//d = 0.5f + sin(t) * 0.5f;
			//bright[i] = ledmap(d, 0.0f, 1.0f);
			//bright[i] = 2047;
		}

		t += 0.1f;

		lights_set_pwm(bright);

		sleep_cpu(100);
	}
	lights_kill();
}

static void anim_plasma()
{
	lights_setup();

	int titers = 50 + (random() % 50);

	faderinit(25, titers-25, 25);

	float t1 = (random() % 64) / 64.f;
	float t2 = (random() % 64) / 64.f;
	float t3 = (random() % 64) / 64.f;
	float t4 = (random() % 64) / 64.f;
	float t5 = (random() % 64) / 64.f;
	float t6 = (random() % 64) / 64.f;


	for (int ti = 0; ti < titers; ti++)
	{

		fader(ti);

		int bright[14];
		for (int i = 0; i < 14; i++)
		{


			float d = 0.f;
			d += sinf(ledpos[i].xp + t1);
			d += sinf(ledpos[i].yp + t2);
			d += sinf(ledpos[i].xp + t3);
			d += sinf(ledpos[i].yp + t4);
			d += sinf(ledpos[i].xp + t5);
			d += sinf(ledpos[i].yp + t6);

			bright[i] = ledmap(d, 0.f, 3.0f);


			//d = 0.5f + sin(t) * 0.5f;
			//bright[i] = ledmap(d, 0.0f, 1.0f);
			//bright[i] = 2047;
		}

		t1 += 1.5423f * 0.1f;
		t2 += 1.222939f * 0.1f;
		t3 += 1.942673f * 0.1f;
		t4 += 2.3364939f * 0.1f;
		t5 += 2.68843423f * 0.1f;
		t6 += 2.9423939f * 0.1f;

		lights_set_pwm(bright);

		sleep_cpu(50);
	}
	lights_kill();
}

const char *sikritcode = "01001111011101000110110101110011001000000100100001110100011100110111001101110100011101110111100000100000010100010110011001110101011110010111010001110101";

static void anim_codeplayer()
{
	lights_setup();

	int codelen = strlen(sikritcode);
	for (int i = 0; i < codelen; i++)
	{
		if (sikritcode[i] == '1')
			lights_set(1 << 9);
		else
			lights_set(0 << 9);

		sleep_cpu(50);
	}
	lights_kill();
}

void (*anims[])() =
{
	//anim_randomflashes,
	anim_randomflashes_fast,
	anim_randomtoggle,
	anim_offtoggles,

	anim_radiate,
	anim_mover,
	anim_plane,
	anim_rotplane,

	anim_plasma,
	anim_codeplayer
};


void anim_run()
{
	int anim = random() % 9;

	//anim = 8;

	debugf("%s: running animation %i...\r\n", __func__, anim);

	anims[anim]();
}