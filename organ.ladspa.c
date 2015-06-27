
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


/* organ.ladspa.c : ladspa plugin test */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ladspa.h"

#define ORG_PNB 4

#define ORG_IC_FREQ 0
#define ORG_IC_N 1
#define ORG_IC_FALL 2
#define ORG_OA_OUT 3

struct org {
  LADSPA_Data *ports[ORG_PNB];
  LADSPA_Data sr;
  unsigned long s;
};

LADSPA_Handle org_make(const LADSPA_Descriptor *desc, unsigned long sr) {
  struct org *h = (struct org*)malloc(sizeof(struct org));
  h->sr = sr * 1.0;
  h->s = 0;
 return h;
}

void org_connect(LADSPA_Handle h, unsigned long p, LADSPA_Data *buff) {
  ((struct org*)h)->ports[p] = buff;
}

void org_activate(LADSPA_Handle h) {}

void org_run(LADSPA_Handle h, unsigned long sc) {
  LADSPA_Data *obuff = ((struct org*)h)->ports[ORG_OA_OUT];
  LADSPA_Data freq = *((struct org*)h)->ports[ORG_IC_FREQ];
  LADSPA_Data n = *((struct org*)h)->ports[ORG_IC_N];
  int fall = *((struct org*)h)->ports[ORG_IC_FALL];
  LADSPA_Data sr = ((struct org*)h)->sr;
  unsigned long s = ((struct org*)h)->s;
  LADSPA_Data f, r, m, b = fall ? 1.0 / (pow(2.0, n * 1.0) - 1.0) : 1.0 / n;
  unsigned long i, j;

  for (i = 0; i < sc; i++) {
    for (j = 0, r = 0, f = freq; j < n; j++) {
      m = fall ? b * pow(2.0, (n - j - 1.0)) : b;
      r += sin(((s + i) / sr) * 2.0 * M_PI * f) * m;
      f /= 2.0;
    }
    obuff[i] = r;
  }
  ((struct org*)h)->s += sc;
}

void org_run_adding(LADSPA_Handle h, unsigned long sc) {}

void org_set_run_adding_gain(LADSPA_Handle h, LADSPA_Data gain) {}

void org_deactivate(LADSPA_Handle h)
{
  ((struct org*)h)->s = 0;
}

void org_cleanup(LADSPA_Handle h) {}

static const char const *org_ports_names[] = {
  "frequency",
  "harmonics number",
  "fall",
  "output"
};

static const LADSPA_PortDescriptor org_ports[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

static const LADSPA_PortRangeHint org_hints[] = {
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_440,
   0., 0.5},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_1,
   1., 100.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_TOGGLED |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_DEFAULT_0,
   0., 1.},
  {0,0,0}
};

static const LADSPA_Descriptor org_pl_desc = {
  9998,
  "mono_org",
  LADSPA_PROPERTY_REALTIME |
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "AATK - Harmonics generator",
  "MrAmp",
  "",
  ORG_PNB,
  org_ports,
  org_ports_names,
  org_hints,
  (void*)0,
  org_make,
  org_connect,
  org_activate,
  org_run,
  org_run_adding,
  org_set_run_adding_gain,
  org_deactivate,
  org_cleanup
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
  if (index == 0)
    return &org_pl_desc;
  return NULL;
}
