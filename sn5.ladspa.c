
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


/* sn4.ladspa.c
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
#define SN_IC_RIG 10
#define SN_IC_HARMO0 11
#define SN_IC_HARMO1 12
#define SN_IC_HARMO2 13
#define SN_IC_HARMO3 14
#define SN_IC_HARMO4 15
#define SN_IC_HARMO5 16
#define SN_IC_HARMO6 17
#define SN_IC_HARMO7 18
#define SN_IC_HARMO8 19
#define SN_IC_REN 20
#define SN_IC_QTZ 21
#define SN_IC_QTZL 22
#define SN_IC_ECHO 23
#define SN_IC_DELAY 24
#define SN_IC_DECAY 25
#define SN_IC_CEN 26
#define SN_OA_OUT 27

#define NUMPORTS 28

#define MAXDELAY 250.f

#define RANDOM() (random() / (float)RAND_MAX)

struct SN_BASE {
  LADSPA_Data a;
  LADSPA_Data b;
  LADSPA_Data c;
  LADSPA_Data d;
  int sw;
  double p;
  double pst;
};

struct SN_ST {
  LADSPA_Data *ports[NUMPORTS];
  struct SN_BASE noise_elems[9];
  LADSPA_Data lastval;
  unsigned int idxqtz;
  int use_echo;
  LADSPA_Data *echobuff;
  LADSPA_Data delay;
  unsigned int maxebuffsz;
  unsigned int ebuffsz;
  unsigned int idxecho;
  LADSPA_Data decay;
  LADSPA_Data sr;
};

LADSPA_Data compute_noise_sample(
  struct SN_BASE *nslm, 
  unsigned int inte,
  LADSPA_Data reg,
  LADSPA_Data dist, 
  LADSPA_Data mult, 
  LADSPA_Data rig,
  LADSPA_Data sr)
{
  LADSPA_Data r, r1, r2;
  int n = 0;

  switch (inte) {
    case 0: /* rectangular */
      r1 = nslm->a;
      r2 = nslm->sw ? 1.f : -1.f;
      break;
    case 1: /* linear */
      r1 = nslm->a + nslm->p * (nslm->b - nslm->a);
      r2 = (nslm->sw ? 1. - nslm->p : nslm->p) * 2. - 1. ;
      break;
    case 2: /* cossine */
      r1 = nslm->a + (cos(nslm->p * M_PI) - 1.) / -2. * (nslm->b - nslm->a);
      r2 = cos((nslm->sw ? nslm->p : 1. + nslm->p) * M_PI);
      break;
    default:
      r1 = 0.f;
      r2 = 0.f;
  }
  r = r1 * (1.f - reg) + r2 * reg;
  if (r < 0.f) {
    n = 1;
    r *= -1.f;
  }
  r = powf(powf(r, 1.f - dist), mult);
  if (n) {
    r *= -1.f;
    n = 0;
  }
  r = r * (1.f - rig) + asinf(r) / (M_PI / 2.f) * rig;

  return r;
}

void update_noise_phases(
  struct SN_BASE *nslm,
  LADSPA_Data inval, 
  LADSPA_Data freq0,
  LADSPA_Data freqd,
  int harmo,
  LADSPA_Data inst,
  LADSPA_Data instl,
  LADSPA_Data sr)
{
  double ifreq;
  LADSPA_Data rst;

  ifreq = (freq0 + inval * freqd) * (double)(1<<harmo);
  rst = nslm->c + (cos(nslm->pst * M_PI) - 1.) / -2. * (nslm->d - nslm->c);
  nslm->p += ifreq * ((double)inst * (rst - .5) + 1.) / sr;
  nslm->pst += ifreq / 2. / powf(2., (double)instl) / sr;

  if (nslm->p >= 1.) {
    nslm->a = nslm->b;
    nslm->b = RANDOM() * 2.f - 1.f;
    nslm->sw = !nslm->sw;
    nslm->p = fmod(nslm->p, 1.);
  }
  if (nslm->pst >= 1.) {
    nslm->c = nslm->d;
    nslm->c = RANDOM();
    nslm->pst = fmod(nslm->pst, 1.);
  }
}

LADSPA_Handle sn_make(const LADSPA_Descriptor *desc, unsigned long sr) {
  struct SN_ST *h = (struct SN_ST*)malloc(sizeof(struct SN_ST));
  h->maxebuffsz = MAXDELAY / 1000.f * sr;
  h->echobuff = (LADSPA_Data*)malloc(h->maxebuffsz * sizeof(float));
  h->sr = sr;
 return h;
}

void sn_connect(LADSPA_Handle h, unsigned long p, LADSPA_Data *buff) {
  ((struct SN_ST*)h)->ports[p] = buff;
}

void sn_activate(LADSPA_Handle h) {
  LADSPA_Data amp = *((struct SN_ST*)h)->ports[SN_IC_AMP];
  unsigned int i = 0;
  struct SN_BASE *nslm;

  for (i = 0; i < 9; i++) {
    nslm = &(((struct SN_ST*)h)->noise_elems[i]);
    nslm->a = RANDOM() * 2.f - 1.f;
    nslm->b = RANDOM() * 2.f - 1.f;
    nslm->c = RANDOM();
    nslm->d = RANDOM();
    nslm->sw = 1;
    nslm->p = 0.f;
    nslm->pst = 0.f;
  }
  ((struct SN_ST*)h)->idxqtz = 0;
  ((struct SN_ST*)h)->use_echo = 0;
}

void sn_run(LADSPA_Handle h, unsigned long sc)
{
  LADSPA_Data *ibuff = ((struct SN_ST*)h)->ports[SN_IA_FM];
  LADSPA_Data *obuff = ((struct SN_ST*)h)->ports[SN_OA_OUT];
  LADSPA_Data freq0 = *((struct SN_ST*)h)->ports[SN_IC_FB0] * 2.f;
  LADSPA_Data freqd = *((struct SN_ST*)h)->ports[SN_IC_FD] * 2.f;
  LADSPA_Data amp = *((struct SN_ST*)h)->ports[SN_IC_AMP];
  LADSPA_Data inst = *((struct SN_ST*)h)->ports[SN_IC_INST];
  LADSPA_Data instl = *((struct SN_ST*)h)->ports[SN_IC_INSTL];
  LADSPA_Data reg = *((struct SN_ST*)h)->ports[SN_IC_REG];
  LADSPA_Data dist = *((struct SN_ST*)h)->ports[SN_IC_DIST];
  LADSPA_Data mult = *((struct SN_ST*)h)->ports[SN_IC_MULT];
  LADSPA_Data rig = *((struct SN_ST*)h)->ports[SN_IC_RIG];
  LADSPA_Data harmo0 = *((struct SN_ST*)h)->ports[SN_IC_HARMO0];
  LADSPA_Data harmo1 = *((struct SN_ST*)h)->ports[SN_IC_HARMO1];
  LADSPA_Data harmo2 = *((struct SN_ST*)h)->ports[SN_IC_HARMO2];
  LADSPA_Data harmo3 = *((struct SN_ST*)h)->ports[SN_IC_HARMO3];
  LADSPA_Data harmo4 = *((struct SN_ST*)h)->ports[SN_IC_HARMO4];
  LADSPA_Data harmo5 = *((struct SN_ST*)h)->ports[SN_IC_HARMO5];
  LADSPA_Data harmo6 = *((struct SN_ST*)h)->ports[SN_IC_HARMO6];
  LADSPA_Data harmo7 = *((struct SN_ST*)h)->ports[SN_IC_HARMO7];
  LADSPA_Data harmo8 = *((struct SN_ST*)h)->ports[SN_IC_HARMO8];
  LADSPA_Data ren = *((struct SN_ST*)h)->ports[SN_IC_REN];
  unsigned int inte = *((struct SN_ST*)h)->ports[SN_IC_INTE];
  struct SN_BASE *nslm = ((struct SN_ST*)h)->noise_elems;
  int qtz;
  unsigned int qtzl;
  int echo = *((struct SN_ST*)h)->ports[SN_IC_ECHO];
  LADSPA_Data *echobuff = ((struct SN_ST*)h)->echobuff;
  LADSPA_Data delay;
  unsigned int ebuffsz;
  LADSPA_Data decay;
  int cen = *((struct SN_ST*)h)->ports[SN_IC_CEN];
  unsigned long i, k, l;
  unsigned j = ((struct SN_ST*)h)->idxqtz;
  LADSPA_Data sum, inval;
  LADSPA_Data sr = ((struct SN_ST*)h)->sr;
  LADSPA_Data r = ((struct SN_ST*)h)->lastval;

  qtz = *((struct SN_ST*)h)->ports[SN_IC_QTZ];
  qtzl = *((struct SN_ST*)h)->ports[SN_IC_QTZL] * sr;
  if (!qtzl) qtzl = 1;
  if (echo) {
    delay = *((struct SN_ST*)h)->ports[SN_IC_DELAY];
    if (delay != ((struct SN_ST*)h)->delay) {
      ebuffsz = delay / 1000.f * sr;
      if (!ebuffsz) ebuffsz = 1;
      ((struct SN_ST*)h)->ebuffsz = ebuffsz;
      echo = 0;
    } else
      ebuffsz = ((struct SN_ST*)h)->ebuffsz;
    decay = *((struct SN_ST*)h)->ports[SN_IC_DECAY];
    if (decay != ((struct SN_ST*)h)->decay) {
      ((struct SN_ST*)h)->decay = decay;
      echo = 0;
    }
    if (!((struct SN_ST*)h)->use_echo) {
      ((struct SN_ST*)h)->use_echo = 1;
      echo = 0;
    }
    if (!echo) { /* echo buffer must be reinitialized */
      for (k = 0; k < ebuffsz; k++)
        echobuff[k] = 0.f;
      k = 0;
      echo = 1;
    } else
      k = ((struct SN_ST*)h)->idxecho;
  }

  for (i = 0; i < sc; i++) {
    if (!qtz || j == 0) {
      inval = ibuff[i];
      sum = harmo0 + harmo1 + harmo2 + harmo3 + harmo4 +
                     harmo5 + harmo6 + harmo7 + harmo8;
      r = (harmo0 == 0.f ? 0.f : (compute_noise_sample(
            &(nslm)[0], inte, reg, dist, mult, rig, sr) * harmo0)) +
          (harmo1 == 0.f ? 0.f : (compute_noise_sample(
            &(nslm)[1], inte, reg, dist, mult, rig, sr) * harmo1)) +
          (harmo2 == 0.f ? 0.f : (compute_noise_sample(
            &(nslm)[2], inte, reg, dist, mult, rig, sr) * harmo2)) +
          (harmo3 == 0.f ? 0.f : (compute_noise_sample(
            &(nslm)[3], inte, reg, dist, mult, rig, sr) * harmo3)) +
          (harmo4 == 0.f ? 0.f : (compute_noise_sample(
            &(nslm)[4], inte, reg, dist, mult, rig, sr) * harmo4)) +
          (harmo5 == 0.f ? 0.f : (compute_noise_sample(
            &(nslm)[5], inte, reg, dist, mult, rig, sr) * harmo5)) +
          (harmo6 == 0.f ? 0.f : (compute_noise_sample(
            &(nslm)[6], inte, reg, dist, mult, rig, sr) * harmo6)) +
          (harmo7 == 0.f ? 0.f : (compute_noise_sample(
            &(nslm)[7], inte, reg, dist, mult, rig, sr) * harmo7)) +
          (harmo8 == 0.f ? 0.f : (compute_noise_sample(
            &(nslm)[8], inte, reg, dist, mult, rig, sr) * harmo8));
      if (sum > 1.f)
        r /= sum;
      r *= 1.f - ren + RANDOM() * ren;
      r *= amp;
    }
    j++;
    j %= qtzl;
    if (echo) {
      r += echobuff[k] * decay;
      echobuff[k] = r;
      k++;
      k %= ebuffsz;
    }
    obuff[i] = cen ? r : r - 1.f + amp;
    for (l = 0; l < 9; l++)
      update_noise_phases(&(nslm[l]), inval, freq0, freqd, l, inst, instl, sr);
  }

  ((struct SN_ST*)h)->lastval = r;
  ((struct SN_ST*)h)->idxqtz = j;
  ((struct SN_ST*)h)->idxecho = k;
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
  "rigidify",
  "harmonic 0",
  "harmonic 1",
  "harmonic 2",
  "harmonic 3",
  "harmonic 4",
  "harmonic 5",
  "harmonic 6",
  "harmonic 7",
  "harmonic 8",
  "renoising",
  "quantized",
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
/* basic parameters */
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_440,
   0.f, .5f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_0,
   0.f, .5f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_1,
   0.f, 2.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_0,
   0.f, 2.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_DEFAULT_0,
   0.f, 50.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_1,
   -10.f, 10.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_DEFAULT_1,
   0.f, 50.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
/* harmonics */
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_1,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
/* renoising */
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
/* quantization */
  {LADSPA_HINT_TOGGLED |
   LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_DEFAULT_0,
   0.f, .01f},
/* echo */
  {LADSPA_HINT_TOGGLED |
   LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, MAXDELAY},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   0.f, 1.f},
  {LADSPA_HINT_TOGGLED |
   LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_1,
   0.f, 1.f},
  {0, 0, 0}
};

static const LADSPA_Descriptor sn_pl_desc = {
  9985,
  "sn_noise5",
  LADSPA_PROPERTY_REALTIME,
  "AATK - SN Noise 5",
  "MrAmp",
  "",
  NUMPORTS,
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

