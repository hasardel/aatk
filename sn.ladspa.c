
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


/* sn.ladspa.c
   noise spreaded using the frequency of your choice;
   with rectangular, linear, or cossine interpolation
*/

#include <stdlib.h>
#include <math.h>
#include "ladspa.h"

#define SN_IC_FREQ 0
#define SN_IC_AMP 1
#define SN_IC_INTE 2
#define SN_IC_CEN 3
#define SN_OA_OUT 4

struct SN_ST {
  LADSPA_Data *ports[5];
  unsigned long x;
  LADSPA_Data a;
  LADSPA_Data b;
  LADSPA_Data freq;
  LADSPA_Data sr;
};

LADSPA_Handle sn_make(const LADSPA_Descriptor *desc, unsigned long sr) {
  struct SN_ST *h = (struct SN_ST*)malloc(sizeof(struct SN_ST));
  h->freq = 440.;
  h->sr = sr * 1.;
 return h;
}

void sn_connect(LADSPA_Handle h, unsigned long p, LADSPA_Data *buff) {
  ((struct SN_ST*)h)->ports[p] = buff;
}

void sn_activate(LADSPA_Handle h) {
  LADSPA_Data amp = *((struct SN_ST*)h)->ports[SN_IC_AMP];
  int c = *((struct SN_ST*)h)->ports[SN_IC_CEN];
  ((struct SN_ST*)h)->x = 0;
  ((struct SN_ST*)h)->a = random() * 2. / RAND_MAX * amp - (c ? amp : 1.);
  ((struct SN_ST*)h)->b = random() * 2. / RAND_MAX * amp - (c ? amp : 1.);
}

void sn_run(LADSPA_Handle h, unsigned long sc)
{
  LADSPA_Data *obuff = ((struct SN_ST*)h)->ports[SN_OA_OUT];
  unsigned long l, x, i;
  LADSPA_Data freq = *((struct SN_ST*)h)->ports[SN_IC_FREQ];
  LADSPA_Data amp = *((struct SN_ST*)h)->ports[SN_IC_AMP];
  LADSPA_Data r;
  LADSPA_Data a = ((struct SN_ST*)h)->a;
  LADSPA_Data b = ((struct SN_ST*)h)->b;
  unsigned int inte = *((struct SN_ST*)h)->ports[SN_IC_INTE];
  int c = *((struct SN_ST*)h)->ports[SN_IC_CEN];

  if (((struct SN_ST*)h)->freq != freq)
    x = 0;
  else
    x = ((struct SN_ST*)h)->x;

  if (freq == 0.)
    inte = 3;
  else
    l = ((struct SN_ST*)h)->sr / freq;

  for (i = 0; i < sc; i++, x++) {
    if (x == l) {
      a = b;
      b = random() * 2. / RAND_MAX * amp - (c ? amp : 1.);
      x = 0;
    }
    switch (inte) {
      case 0: /* rectangular */
        r = a;
        break;
      case 1: /* linear */
        r = a + x * 1. / l * (b - a);
        break;
      case 2: /* cossine */
        r = a + (cos(x * 1. / l * M_PI) - 1.) / -2. * (b - a);
        break;
      default:
        r = 0.;
    }
    obuff[i] = r;
  }

  ((struct SN_ST*)h)->a = a;
  ((struct SN_ST*)h)->b = b;
  ((struct SN_ST*)h)->x = x;
}

void sn_run_adding(LADSPA_Handle h, unsigned long sc) {}

void sn_set_run_adding_gain(LADSPA_Handle h, LADSPA_Data gain) {}

void sn_deactivate(LADSPA_Handle h) {}

void sn_cleanup(LADSPA_Handle h) {}

static const char const *sn_ports_names[] = {
  "frequency",
  "amplification",
  "interpolation",
  "centered",
  "output"
};

static const LADSPA_PortDescriptor sn_ports[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

static const LADSPA_PortRangeHint sn_hints[] = {
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_440,
   0., 1.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_1,
   0., 10.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_0,
   0., 2.},
  {LADSPA_HINT_TOGGLED |
   LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_1,
   0., 1.},
  {0, 0, 0}
};

static const LADSPA_Descriptor sn_pl_desc = {
  9996,
  "sn_noise",
  LADSPA_PROPERTY_REALTIME |
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "AATK - SN Noise",
  "MrAmp",
  "",
  5,
  sn_ports,
  sn_ports_names,
  sn_hints,
  (void*)0,
  sn_make,
  sn_connect,
  sn_activate,
  sn_run,
  sn_run_adding,
  sn_set_run_adding_gain,
  sn_deactivate,
  sn_cleanup
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
  if (index == 0)
    return &sn_pl_desc;
  return NULL;
}


