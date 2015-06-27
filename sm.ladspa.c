
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


/* sm.ladspa.c */

#include <stdlib.h>
#include <math.h>
#include "ladspa.h"

#define TWO_PI 6.28318530717958647692

#define SM_IA_IN 0
#define SM_IC_FREQ 1
#define SM_IC_LEVEL 2
#define SM_OA_OUT 3

struct SM_ST {
  LADSPA_Data *ports[4];
  LADSPA_Data sr;
  LADSPA_Data p;
};

LADSPA_Handle sm_make(const LADSPA_Descriptor *desc, unsigned long sr)
{
  struct SM_ST *h = (struct SM_ST*)malloc(sizeof(struct SM_ST));
  h->sr = sr * 1.;
 return h;
}

void sm_connect(LADSPA_Handle h, unsigned long p, LADSPA_Data *buff)
{
  ((struct SM_ST*)h)->ports[p] = buff;
}

void sm_activate(LADSPA_Handle h)
{
  ((struct SM_ST*)h)->p = 0.;
}

void sm_run(LADSPA_Handle h, unsigned long sc)
{
  LADSPA_Data *ibuff = ((struct SM_ST*)h)->ports[SM_IA_IN];
  register LADSPA_Data *obuff = ((struct SM_ST*)h)->ports[SM_OA_OUT];
  LADSPA_Data freq = *((struct SM_ST*)h)->ports[SM_IC_FREQ];
  LADSPA_Data level = *((struct SM_ST*)h)->ports[SM_IC_LEVEL];
  register LADSPA_Data p = ((struct SM_ST*)h)->p;
  LADSPA_Data sr = ((struct SM_ST*)h)->sr;
  register unsigned long i = 0;

  for (; i < sc; i++) {
    obuff[i] = ibuff[i] * (1. - level + (sin(p) / 2. + .5) * level);
    p += freq / sr * TWO_PI;
    p = fmodf(p, TWO_PI);
  }

  ((struct SM_ST*)h)->p = p;
}

void sm_run_adding(LADSPA_Handle h, unsigned long sc) {}

void sm_set_run_adding_gain(LADSPA_Handle h, LADSPA_Data gain) {}

void sm_deactivate(LADSPA_Handle h) {}

void sm_cleanup(LADSPA_Handle h) {}

static const char const *sm_ports_names[] = {
  "input",
  "frequency",
  "level",
  "output"
};

static const LADSPA_PortDescriptor sm_ports[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

static const LADSPA_PortRangeHint sm_hints[] = {
  {0, 0, 0},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_440,
   0., 0.5},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_DEFAULT_1,
   0., 1.},
  {0, 0, 0}
};

static const LADSPA_Descriptor sm_pl_desc = {
  9993,
  "sin_multiply",
  LADSPA_PROPERTY_REALTIME |
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "AATK - Sin Multiplier",
  "MrAmp",
  "",
  4,
  sm_ports,
  sm_ports_names,
  sm_hints,
  (void*)0,
  sm_make,
  sm_connect,
  sm_activate,
  sm_run,
  sm_run_adding,
  sm_set_run_adding_gain,
  sm_deactivate,
  sm_cleanup
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
  if (index == 0)
    return &sm_pl_desc;
  return NULL;
}


