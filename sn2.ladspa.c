
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


/* sn2.ladspa.c
   noise spreaded using the frequency of your choice;
   with rectangular, linear, or cossine interpolation
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ladspa.h"

#define SN_IA_FM 0
#define SN_IC_FB0 1
#define SN_IC_FD 2
#define SN_IC_AMP 3
#define SN_IC_INTE 4
#define SN_IC_INST 5
#define SN_IC_INSTL 6
#define SN_IC_REG 7
#define SN_IC_DIST 8
#define SN_IC_MULT 9
#define SN_IC_MULT2 10
#define SN_IC_RIG 11
#define SN_IC_REN 12
#define SN_IC_HARMO 13
#define SN_IC_QTZ 14
#define SN_IC_ECHO 15
#define SN_IC_DELAY 16
#define SN_IC_RED 17
#define SN_IC_CEN 18
#define SN_OA_OUT 19

#define MAXDELAY 250.f

#define RANDOM() (random() / (float)RAND_MAX)

struct SN_ST {
  LADSPA_Data *ports[20];
  LADSPA_Data a;
  LADSPA_Data b;
  LADSPA_Data c;
  LADSPA_Data d;
  LADSPA_Data last;
  unsigned int iqtz;
  int echo;
  LADSPA_Data *ebuff;
  LADSPA_Data delay;
  unsigned int maxebsz;
  unsigned int ebsz;
  unsigned int esmpl;
  LADSPA_Data red;
  int sw;
  double p;
  double ps;
  LADSPA_Data sr;
};

LADSPA_Handle sn_make(const LADSPA_Descriptor *desc, unsigned long sr) {
  struct SN_ST *h = (struct SN_ST*)malloc(sizeof(struct SN_ST));
  h->maxebsz = MAXDELAY / 1000.f * sr;
  h->ebuff = (LADSPA_Data*)malloc(h->maxebsz * sizeof(float));
  h->sr = sr;
 return h;
}

void sn_connect(LADSPA_Handle h, unsigned long p, LADSPA_Data *buff) {
  ((struct SN_ST*)h)->ports[p] = buff;
}

void sn_activate(LADSPA_Handle h) {
  LADSPA_Data amp = *((struct SN_ST*)h)->ports[SN_IC_AMP];
  int c = *((struct SN_ST*)h)->ports[SN_IC_CEN];
  ((struct SN_ST*)h)->a = RANDOM() * 2. * amp - (c ? amp : 1.);
  ((struct SN_ST*)h)->b = RANDOM() * 2. * amp - (c ? amp : 1.);
  ((struct SN_ST*)h)->c = RANDOM();
  ((struct SN_ST*)h)->d = RANDOM();
  ((struct SN_ST*)h)->sw = 1.;
  ((struct SN_ST*)h)->p = 0.;
  ((struct SN_ST*)h)->ps = 0.;
  ((struct SN_ST*)h)->iqtz = 0;
  ((struct SN_ST*)h)->echo = 0;
}

void sn_run(LADSPA_Handle h, unsigned long sc)
{
  LADSPA_Data *ibuff = ((struct SN_ST*)h)->ports[SN_IA_FM];
  LADSPA_Data *obuff = ((struct SN_ST*)h)->ports[SN_OA_OUT];
  LADSPA_Data freq0 = *((struct SN_ST*)h)->ports[SN_IC_FB0] * 2.;
  LADSPA_Data freqd = *((struct SN_ST*)h)->ports[SN_IC_FD] * 2.;
  LADSPA_Data amp = *((struct SN_ST*)h)->ports[SN_IC_AMP];
  LADSPA_Data insta = *((struct SN_ST*)h)->ports[SN_IC_INST];
  LADSPA_Data linsta = *((struct SN_ST*)h)->ports[SN_IC_INSTL];
  LADSPA_Data reg = *((struct SN_ST*)h)->ports[SN_IC_REG];
  LADSPA_Data dist = *((struct SN_ST*)h)->ports[SN_IC_DIST];
  LADSPA_Data mult = *((struct SN_ST*)h)->ports[SN_IC_MULT];
  LADSPA_Data mult2 = *((struct SN_ST*)h)->ports[SN_IC_MULT2];
  LADSPA_Data rig = *((struct SN_ST*)h)->ports[SN_IC_RIG];
  LADSPA_Data ren = *((struct SN_ST*)h)->ports[SN_IC_REN];
  unsigned int harmo = *((struct SN_ST*)h)->ports[SN_IC_HARMO];
  unsigned int l;
  unsigned int inte = *((struct SN_ST*)h)->ports[SN_IC_INTE];
  int cen = *((struct SN_ST*)h)->ports[SN_IC_CEN];
  unsigned int qtz;
  unsigned int j = ((struct SN_ST*)h)->iqtz;
  int echo = *((struct SN_ST*)h)->ports[SN_IC_ECHO];
  LADSPA_Data *ebuff = ((struct SN_ST*)h)->ebuff;
  LADSPA_Data delay;
  unsigned int ebsz;
  unsigned int k;
  LADSPA_Data red;
  LADSPA_Data a = ((struct SN_ST*)h)->a;
  LADSPA_Data b = ((struct SN_ST*)h)->b;
  LADSPA_Data c = ((struct SN_ST*)h)->c;
  LADSPA_Data d = ((struct SN_ST*)h)->d;
  int sw = ((struct SN_ST*)h)->sw;
  double p = ((struct SN_ST*)h)->p;
  double ps = ((struct SN_ST*)h)->ps;
  LADSPA_Data sr = ((struct SN_ST*)h)->sr;
  double ifreq;
  int n = 0;
  unsigned long i;
  LADSPA_Data r = ((struct SN_ST*)h)->last;
  LADSPA_Data r1, r2, rs;

  qtz = *((struct SN_ST*)h)->ports[SN_IC_QTZ] * sr;
  if (!qtz) qtz = 1;
  if (echo) {
    delay = *((struct SN_ST*)h)->ports[SN_IC_DELAY];
    if (delay != ((struct SN_ST*)h)->delay) {
      ebsz = delay / 1000.f * sr;
      if (!ebsz) ebsz = 1;
      ((struct SN_ST*)h)->ebsz = ebsz;
      echo = 0;
    } else
      ebsz = ((struct SN_ST*)h)->ebsz;
    red = *((struct SN_ST*)h)->ports[SN_IC_RED];
    if (red != ((struct SN_ST*)h)->red) {
      ((struct SN_ST*)h)->red = red;
      echo = 0;
    }
    if (!((struct SN_ST*)h)->echo) {
      ((struct SN_ST*)h)->echo = 1;
      echo = 0;
    }
    if (!echo) { /* echo buffer must be reinitialized */
      for (k = 0; k < ebsz; k++)
        ((struct SN_ST*)h)->ebuff[k] = 0.f;
      k = 0;
      echo = 1;
    } else
      k = ((struct SN_ST*)h)->esmpl;
  }

  for (i = 0; i < sc; i++) {
    if (j == 0) {
      switch (inte) {
        case 0: /* rectangular */
          r1 = a;
          r2 = sw ? 1. : -1.;
          break;
        case 1: /* linear */
          r1 = a + p * (b - a);
          r2 = (sw ? 1 - p : p) * 2. - 1. ;
          break;
        case 2: /* cossine */
          r1 = a + (cos(p * M_PI) - 1.) / -2. * (b - a);
          r2 = cos((sw ? p : 1. + p) * M_PI);
          break;
        default:
          r1 = 0.;
          r2 = 0.;
      }
      rs = c + (cos(ps * M_PI) - 1.) / -2. * (d - c);
      r = r1 * (1. - reg) + r2 * reg;
      if (r < 0.) {
        n = 1;
        r *= -1.;
      }
      r = powf(powf(r, 1. - dist), mult + mult2);
      if (n) {
        r *= -1.;
        n = 0;
      }
      r = r * (1. - rig) + asinf(r) / (M_PI / 2.) * rig;
      r *= 1. - ren + RANDOM() * ren;
      for (l = 0; l < harmo; l++) {
        r = powf(r, 2.f) * 2.f - 1.f;
      }
      r *= amp;
    }
    j++;
    j %= qtz;
    if (echo) {
      r += ebuff[k] * red;
      ebuff[k] = r;
      k++;
      k %= ebsz;
    }
    obuff[i] = cen ? r : r - amp;
    ifreq = (ibuff[i] + 1.) * freqd + (freq0 - freqd);
    p += ifreq * (insta * (rs - .5) + 1.) / sr;
    ps += ifreq / 2. / powf(2., linsta) / sr;
    if (p >= 1.) {
      a = b;
      b = RANDOM() * 2. - 1.;
      sw = !sw;
      p = fmod(p, 1.);
    }
    if (ps >= 1.) {
      c = d;
      c = RANDOM();
      ps = fmod(ps, 1.);
    }
  }

  ((struct SN_ST*)h)->a = a;
  ((struct SN_ST*)h)->b = b;
  ((struct SN_ST*)h)->c = c;
  ((struct SN_ST*)h)->d = d;
  ((struct SN_ST*)h)->last = r;
  ((struct SN_ST*)h)->iqtz = j;
  ((struct SN_ST*)h)->esmpl = k;
  ((struct SN_ST*)h)->sw = sw;
  ((struct SN_ST*)h)->p = p;
  ((struct SN_ST*)h)->ps = ps;
}

void sn_run_adding(LADSPA_Handle h, unsigned long sc) {}

void sn_set_run_adding_gain(LADSPA_Handle h, LADSPA_Data gain) {}

void sn_deactivate(LADSPA_Handle h) {}

void sn_cleanup(LADSPA_Handle h) {}

static const char const *sn_ports_names[] = {
  "FM",
  "lower bound(Hz)",
  "delta(Hz)",
  "amplification",
  "interpolation",
  "instability",
  "length of instability",
  "regularity",
  "distortion",
  "multiplication",
  "additional multiplication",
  "rigidify",
  "renoising",
  "harmonic",
  "quantization(s)",
  "echo",
  "delay(ms)",
  "reduction",
  "centered",
  "output"
};

static const LADSPA_PortDescriptor sn_ports[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

static const LADSPA_PortRangeHint sn_hints[] = {
  {0, 0, 0},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_440,
   0., .5},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_0,
   0., .5},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_1,
   0., 1.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_0,
   0., 2.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_DEFAULT_0,
   0., 50.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_1,
   -10., 10.},
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
   LADSPA_HINT_DEFAULT_1,
   1., 5.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0., 50.},
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
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_0,
   0., 20.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_DEFAULT_0,
   0., .125},
  {LADSPA_HINT_TOGGLED |
   LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_0,
   0., 1.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0., MAXDELAY},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0., 1.},
  {LADSPA_HINT_TOGGLED |
   LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_1,
   0., 1.},
  {0, 0, 0}
};

static const LADSPA_Descriptor sn_pl_desc = {
  9984,/*9991*/
  "sn_noise2",
  LADSPA_PROPERTY_REALTIME,
  "AATK - SN Noise 2",
  "MrAmp",
  "",
  20,
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


