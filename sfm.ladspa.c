
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


/* sfm.ladspa.c */

#include <stdlib.h>
#include <math.h>
#include "ladspa.h"

#define TWO_PI 6.28318530717958647692

#define SFM_IA_CF 0
#define SFM_IC_FB0 1
#define SFM_IC_FB1 2
#define SFM_IC_MF 3
#define SFM_IC_IDX 4
#define SFM_IC_AMP 5
#define SFM_IC_TF 6
#define SFM_OA_OUT 7

struct SFM_ST {
  LADSPA_Data *ports[8];
  LADSPA_Data sr;
  LADSPA_Data pc;
  LADSPA_Data pm;
};

LADSPA_Handle sfm_make(const LADSPA_Descriptor *desc, unsigned long sr)
{
  struct SFM_ST *h = (struct SFM_ST*)malloc(sizeof(struct SFM_ST));
  h->sr = sr * 1.;
 return h;
}

void sfm_connect(LADSPA_Handle h, unsigned long p, LADSPA_Data *buff)
{
  ((struct SFM_ST*)h)->ports[p] = buff;
}

void sfm_activate(LADSPA_Handle h)
{
  ((struct SFM_ST*)h)->pc = 0.;
  ((struct SFM_ST*)h)->pm = 0.;
}

void sfm_run(LADSPA_Handle h, unsigned long sc)
{
  LADSPA_Data *ibuff = ((struct SFM_ST*)h)->ports[SFM_IA_CF];
  register LADSPA_Data *obuff = ((struct SFM_ST*)h)->ports[SFM_OA_OUT];
  LADSPA_Data freq0 = *((struct SFM_ST*)h)->ports[SFM_IC_FB0];
  LADSPA_Data freq1 = *((struct SFM_ST*)h)->ports[SFM_IC_FB1];
  LADSPA_Data mfreq = *((struct SFM_ST*)h)->ports[SFM_IC_MF];
  LADSPA_Data idx = *((struct SFM_ST*)h)->ports[SFM_IC_IDX];
  LADSPA_Data amp = *((struct SFM_ST*)h)->ports[SFM_IC_AMP];
  LADSPA_Data pc = ((struct SFM_ST*)h)->pc;
  LADSPA_Data pm = ((struct SFM_ST*)h)->pm;
  LADSPA_Data sr = ((struct SFM_ST*)h)->sr;
  int is = *((struct SFM_ST*)h)->ports[SFM_IC_TF];
  register unsigned long i = 0;

  for (; i < sc; i++) {
    obuff[i] = (is ? sin(pc + idx * sin(pm)) : cos(pc + idx * cos(pm))) * amp;
    pc += (freq0 + (freq1 - freq0) * (ibuff[i] / 2. + .5)) /
          sr * TWO_PI;
    pc = fmodf(pc, TWO_PI);
    pm += mfreq / sr * TWO_PI;
    pm = fmodf(pm, TWO_PI);
  }

  ((struct SFM_ST*)h)->pc = pc;
  ((struct SFM_ST*)h)->pm = pm;
}

void sfm_run_adding(LADSPA_Handle h, unsigned long sc) {}

void sfm_set_run_adding_gain(LADSPA_Handle h, LADSPA_Data gain) {}

void sfm_deactivate(LADSPA_Handle h) {}

void sfm_cleanup(LADSPA_Handle h) {}

static const char const *sfm_ports_names[] = {
  "input",
  "lower bound",
  "upper bound",
  "modulation frequency",
  "index",
  "amplitude",
  "sin? (or cos)",
  "output"
};

static const LADSPA_PortDescriptor sfm_ports[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

static const LADSPA_PortRangeHint sfm_hints[] = {
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
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_440,
   0., 0.5},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_SAMPLE_RATE |
   LADSPA_HINT_DEFAULT_1,
   0., 0.5},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_LOGARITHMIC |
   LADSPA_HINT_DEFAULT_0,
   0., 100000.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_1,
   0., 1.},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_INTEGER |
   LADSPA_HINT_TOGGLED |
   LADSPA_HINT_DEFAULT_1,
   0., 1.},
  {0, 0, 0}
};

static const LADSPA_Descriptor sfm_pl_desc = {
  9992,
  "fm_synthesis_simple",
  LADSPA_PROPERTY_REALTIME |
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "AATK - Simple FM Synthesis",
  "MrAmp",
  "",
  8,
  sfm_ports,
  sfm_ports_names,
  sfm_hints,
  (void*)0,
  sfm_make,
  sfm_connect,
  sfm_activate,
  sfm_run,
  sfm_run_adding,
  sfm_set_run_adding_gain,
  sfm_deactivate,
  sfm_cleanup
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
  if (index == 0)
    return &sfm_pl_desc;
  return NULL;
}


