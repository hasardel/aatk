
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


/* amp.ladspa.c : ladspa plugin test */

#include <stdlib.h>
#include "ladspa.h"

#define AMP_IA_INP 0
#define AMP_IC_GAIN 1
#define AMP_OA_OUT 2

LADSPA_Handle amp_make(const LADSPA_Descriptor *desc, unsigned long sr) {
  return malloc(3 * sizeof(LADSPA_Data*));
}

void amp_connect(LADSPA_Handle h, unsigned long p, LADSPA_Data *buff) {
  ((LADSPA_Data**)h)[p] = buff;
}

void amp_activate(LADSPA_Handle h) {}

void amp_run(LADSPA_Handle h, unsigned long sc) {
  LADSPA_Data *ibuff = ((LADSPA_Data**)h)[AMP_IA_INP];
  LADSPA_Data *obuff = ((LADSPA_Data**)h)[AMP_OA_OUT];
  LADSPA_Data gain = *((LADSPA_Data**)h)[AMP_IC_GAIN];
  unsigned long i = 0;
  for (; i < sc; i++) {
    obuff[i] = ibuff[i] * gain;
  }
}

void amp_run_adding(LADSPA_Handle h, unsigned long sc) {}

void amp_set_run_adding_gain(LADSPA_Handle h, LADSPA_Data gain) {}

void amp_deactivate(LADSPA_Handle h) {}

void amp_cleanup(LADSPA_Handle h) {}

static const char const *amp_ports_names[] = {
  "input",
  "gain",
  "output"
};

static const LADSPA_PortDescriptor amp_ports[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

static const LADSPA_PortRangeHint amp_hints[] = {
  {0,0,0},
  {LADSPA_HINT_BOUNDED_BELOW |
   LADSPA_HINT_BOUNDED_ABOVE |
   LADSPA_HINT_DEFAULT_1,
   -10., 10.},
  {0,0,0}
};

static const LADSPA_Descriptor amp_pl_desc = {
  9999,
  "mono_amp",
  LADSPA_PROPERTY_REALTIME |
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "AATK - Mono amplificator",
  "MrAmp",
  "",
  3,
  amp_ports,
  amp_ports_names,
  amp_hints,
  (void*)0,
  amp_make,
  amp_connect,
  amp_activate,
  amp_run,
  amp_run_adding,
  amp_set_run_adding_gain,
  amp_deactivate,
  amp_cleanup
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
  if (index == 0)
    return &amp_pl_desc;
  return NULL;
}
