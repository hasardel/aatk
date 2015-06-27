
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


/* fm.ladspa.c */

#include <stdlib.h>
#include <math.h>
#include "ladspa.h"

#define FM_IA_CFAM 0
#define FM_IC_CF 1
#define FM_IC_CFB1 2
#define FM_IC_AMP 3
#define FM_IC_IDX 4
#define FM_IA_MF 5
#define FM_OA_OUT 6

struct FM_ST {
  LADSPA_Data *ports[7];
  LADSPA_Data sr;
  unsigned long s;
};

LADSPA_Handle fm_make(const LADSPA_Descriptor *desc, unsigned long sr)
{
  struct FM_ST *h = (struct FM_ST*)malloc(sizeof(struct FM_ST));
  h->sr = sr * 1.;
 return h;
}

void fm_connect(LADSPA_Handle h, unsigned long p, LADSPA_Data *buff)
{
  ((struct FM_ST*)h)->ports[p] = buff;
}

void fm_activate(LADSPA_Handle h)
{
  ((struct FM_ST*)h)->s = 0;
}

void fm_run(LADSPA_Handle h, unsigned long sc)
{
  LADSPA_Data *ibuffc = ((struct FM_ST*)h)->ports[FM_IA_CFAM];
  LADSPA_Data *ibuffm = ((struct FM_ST*)h)->ports[FM_IA_MF];
  LADSPA_Data *obuff = ((struct FM_ST*)h)->ports[FM_OA_OUT];
  LADSPA_Data freq = *((struct FM_ST*)h)->ports[FM_IC_CF];
  LADSPA_Data amb1 = *((struct FM_ST*)h)->ports[FM_IC_CFB1];
  LADSPA_Data amp = *((struct FM_ST*)h)->ports[FM_IC_AMP];
  LADSPA_Data idx = *((struct FM_ST*)h)->ports[FM_IC_IDX];
  LADSPA_Data sr = ((struct FM_ST*)h)->sr;
  unsigned long s = ((struct FM_ST*)h)->s;
  unsigned long i = 0;

  for (; i < sc; i++, s++)
    obuff[i] = amb1 == -1. ? 0. :
               sin(s / sr * 2.0 * M_PI *
                   freq * (1. + ibuffc[i]) / (1. + amb1) +
                   idx * ibuffm[i]) * amp;

  ((struct FM_ST*)h)->s = s;
}

void fm_run_adding(LADSPA_Handle h, unsigned long sc) {}

void fm_set_run_adding_gain(LADSPA_Handle h, LADSPA_Data gain) {}

void fm_deactivate(LADSPA_Handle h) {}

void fm_cleanup(LADSPA_Handle h) {}

static const char const *fm_ports_names[] = {
  "CFAM",
  "carrier freq.",
  "bound 1.0",
  "amplification",
  "index",
  "FM",
  "output"
};

static const LADSPA_PortDescriptor fm_ports[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

static const LADSPA_PortRangeHint fm_hints[] = {
  {0, 0, 0},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_440,
   0., 0.5},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_0,
   -1., 1.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_1,
   0., 10.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_1,
   0., 1000000.},
  {0, 0, 0},
  {0, 0, 0}
};

static const LADSPA_Descriptor fm_pl_desc = {
  9995,
  "fm_synthesis",
  LADSPA_PROPERTY_REALTIME |
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "AATK - FM Synthesis",
  "MrAmp",
  "",
  7,
  fm_ports,
  fm_ports_names,
  fm_hints,
  (void*)0,
  fm_make,
  fm_connect,
  fm_activate,
  fm_run,
  fm_run_adding,
  fm_set_run_adding_gain,
  fm_deactivate,
  fm_cleanup
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
  if (index == 0)
    return &fm_pl_desc;
  return NULL;
}


