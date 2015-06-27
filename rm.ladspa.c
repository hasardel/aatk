
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


/* rm.ladspa.c */

#include <stdlib.h>
#include <math.h>
#include "ladspa.h"

#define RM_IA_C1 0
#define RM_IA_C2 1
#define RM_IC_FREQ 2
#define RM_OA_OUT 3

struct RM_ST {
  LADSPA_Data *ports[4];
  LADSPA_Data sr;
  unsigned long s;
};

LADSPA_Handle rm_make(const LADSPA_Descriptor *desc, unsigned long sr) {
  struct RM_ST *h = (struct RM_ST*)malloc(sizeof(struct RM_ST));
  h->sr = sr * 1.;
 return h;
}

void rm_connect(LADSPA_Handle h, unsigned long p, LADSPA_Data *buff) {
  ((struct RM_ST*)h)->ports[p] = buff;
}

void rm_activate(LADSPA_Handle h) {
  ((struct RM_ST*)h)->s = 0;
}

void rm_run(LADSPA_Handle h, unsigned long sc)
{
  LADSPA_Data *ibuff1 = ((struct RM_ST*)h)->ports[RM_IA_C1];
  LADSPA_Data *ibuff2 = ((struct RM_ST*)h)->ports[RM_IA_C2];
  LADSPA_Data *obuff = ((struct RM_ST*)h)->ports[RM_OA_OUT];
  LADSPA_Data freq = *((struct RM_ST*)h)->ports[RM_IC_FREQ];
  LADSPA_Data sr = ((struct RM_ST*)h)->sr;
  LADSPA_Data x;
  unsigned long s = ((struct RM_ST*)h)->s;
  unsigned long i;

  for (i = 0; i < sc; i++, s++) {
    x = sin(s / sr * 2. * M_PI * freq) / 2. + 0.5;
    obuff[i] = x * ibuff1[i] + (1. - x) * ibuff2[i];
  }
}

void rm_run_adding(LADSPA_Handle h, unsigned long sc) {}

void rm_set_run_adding_gain(LADSPA_Handle h, LADSPA_Data gain) {}

void rm_deactivate(LADSPA_Handle h) {}

void rm_cleanup(LADSPA_Handle h) {}

static const char const *rm_ports_names[] = {
  "channel 1",
  "channel 2",
  "frequency",
  "output"
};

static const LADSPA_PortDescriptor rm_ports[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

static const LADSPA_PortRangeHint rm_hints[] = {
  {0, 0, 0},
  {0, 0, 0},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_440,
   0., 0.5},
  {0, 0, 0}
};

static const LADSPA_Descriptor rm_pl_desc = {
  9994,
  "rm",
  LADSPA_PROPERTY_REALTIME |
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "AATK - Ring Modulator",
  "MrAmp",
  "",
  4,
  rm_ports,
  rm_ports_names,
  rm_hints,
  (void*)0,
  rm_make,
  rm_connect,
  rm_activate,
  rm_run,
  rm_run_adding,
  rm_set_run_adding_gain,
  rm_deactivate,
  rm_cleanup
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
  if (index == 0)
    return &rm_pl_desc;
  return NULL;
}


