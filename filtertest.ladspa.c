
/*
 * Copyright (C) 2015 Florent Pouthier
 * Copyright (C) 2015 Emmanuel Pouthier
 *
 * This file is part of AATK.
 *
 * Aye-Aye is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Aye-Aye is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/* ft2.ladspa.c
   noise spreaded using the frequency of your choice;
   with rectangular, linear, or cossine interpolation
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ladspa.h"

#define FT_PNB 8

#define FT_IA_INP0 0
#define FT_IC_FREQ0 1
#define FT_IC_FREQ1 2
#define FT_IC_VAR0 3
#define FT_IC_VAR1 4
#define FT_IC_SVAR0 5
#define FT_IC_SVAR1 6
#define FT_OA_OUT0 7

#define RANDOM() (random() / (float)RAND_MAX)

#define FTST(h) ((struct FT_ST*)h)

#define TSZ 256
static float table[TSZ+1];

void fill_table() __attribute__ ((constructor));
void fill_table()
{
  int i = 0;
  table[TSZ] = 0.;
  for(i = 0; i < TSZ; i++) {
    table[i] = powf(sinf(i / (double)TSZ * 2. * M_PI), .5);
  }
}

struct FT_ST {
  LADSPA_Data *ports[FT_PNB];
  LADSPA_Data a, b, c, d;
  LADSPA_Data r1, r2, r3, r4, r5, r6, r7, r8;
  double p0, p1;
  double sum;
  double sr;
};

LADSPA_Handle ft_make(const LADSPA_Descriptor *desc, unsigned long sr) {
  struct FT_ST *h = (struct FT_ST*)malloc(sizeof(struct FT_ST));
  h->p0 = 0.;
  h->p1 = 0.;
  h->r1 = 0.;
  h->r2 = 0.;
  h->r3 = 0.;
  h->r4 = 0.;
  h->r5 = 0.;
  h->r6 = 0.;
  h->r7 = 0.;
  h->r8 = 0.;
  h->sum = 0.;
  h->sr = sr;
 return h;
}

void ft_connect(LADSPA_Handle h, unsigned long p, LADSPA_Data *buff) {
  FTST(h)->ports[p] = buff;
}

void ft_activate(LADSPA_Handle h) {
  FTST(h)->d = 1.f/44100.f;
  FTST(h)->r1 = 0.f;
}

void ft_run(LADSPA_Handle h, unsigned long sc)
{
  LADSPA_Data *ibuff0 = FTST(h)->ports[FT_IA_INP0];
  LADSPA_Data freq0 = *FTST(h)->ports[FT_IC_FREQ0];
  LADSPA_Data freq1 = *FTST(h)->ports[FT_IC_FREQ1];
  LADSPA_Data var0 = *FTST(h)->ports[FT_IC_VAR0];
  LADSPA_Data var1 = *FTST(h)->ports[FT_IC_VAR1];
  LADSPA_Data svar0 = *FTST(h)->ports[FT_IC_SVAR0];
  LADSPA_Data svar1 = *FTST(h)->ports[FT_IC_SVAR1];
  LADSPA_Data *obuff0 = FTST(h)->ports[FT_OA_OUT0];
  LADSPA_Data a = FTST(h)->a;
  LADSPA_Data b = FTST(h)->b;
  LADSPA_Data c = FTST(h)->c;
  LADSPA_Data d = FTST(h)->d;
  LADSPA_Data r1 = FTST(h)->r1;
  LADSPA_Data r2 = FTST(h)->r2;
  LADSPA_Data r3 = FTST(h)->r3;
  LADSPA_Data r4 = FTST(h)->r4;
  LADSPA_Data r5 = FTST(h)->r5;
  LADSPA_Data r6 = FTST(h)->r6;
  LADSPA_Data r7 = FTST(h)->r7;
  LADSPA_Data r8 = FTST(h)->r8;
  LADSPA_Data p0 = FTST(h)->p0;
  LADSPA_Data p1 = FTST(h)->p1;
  double sum = FTST(h)->sum;
  double in;
  double sr = FTST(h)->sr;
  unsigned int i, j;

  a = d / (svar0 + d);

  for (i = 0; i < sc; i++) {
    in = ibuff0[i];
/*
    j = (unsigned int)(p0);
    r2 = p0 - (float)j;
    r1 = table[j] * (1. - r2) + table[j+1] * r2;
    p0 = fmod(p0 += freq0 / sr * (double)TSZ, (double)TSZ);
*/

  r1 = table[(unsigned int)(RANDOM() * TSZ)];

/* low pass filter
    r1 = a * in + (1.f - a) * r1;
*/

    obuff0[i] = r1;
  }

  FTST(h)->sum = sum;
  FTST(h)->a = a;
  FTST(h)->b = b;
  FTST(h)->c = c;
  FTST(h)->d = d;
  FTST(h)->r1 = r1;
  FTST(h)->r2 = r2;
  FTST(h)->r3 = r3;
  FTST(h)->r4 = r4;
  FTST(h)->r5 = r5;
  FTST(h)->r6 = r6;
  FTST(h)->r7 = r7;
  FTST(h)->r8 = r8;
  FTST(h)->p0 = p0;
  FTST(h)->p1 = p1;
}

void ft_run_adding(LADSPA_Handle h, unsigned long sc) {}

void ft_set_run_adding_gain(LADSPA_Handle h, LADSPA_Data gain) {}

void ft_deactivate(LADSPA_Handle h) {}

void ft_cleanup(LADSPA_Handle h) {}

static const char const *ft_ports_names[] = {
  "input0",
  "frequency 1",
  "frequency 2",
  "variable A",
  "variable B",
  "variable C",
  "variable D",
  "output0"
};

static const LADSPA_PortDescriptor ft_ports[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

static const LADSPA_PortRangeHint ft_hints[] = {
  {0, 0, 0},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_440,
   0., 1.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_440,
   0., .5},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0., 1.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0., 1.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   -1., 1.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   -1., 1.},
  {0, 0, 0}
};

static const LADSPA_Descriptor ft_pl_desc = {
  9990,
  "ftest",
  LADSPA_PROPERTY_REALTIME |
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "AATK - Filter Test",
  "MrAmp",
  "",
  FT_PNB,
  ft_ports,
  ft_ports_names,
  ft_hints,
  (void*)0,
  ft_make,
  ft_connect,
  ft_activate,
  ft_run,
  ft_run_adding,
  ft_set_run_adding_gain,
  ft_deactivate,
  ft_cleanup
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
  if (index == 0)
    return &ft_pl_desc;
  return NULL;
}


