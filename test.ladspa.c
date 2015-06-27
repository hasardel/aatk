
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


/* test.ladspa.c : ladspa plugin test */

#include <stdio.h>
#include <stdlib.h>
#include "ladspa.h"

LADSPA_Handle test_make(const LADSPA_Descriptor *desc, unsigned long sr) {
  return malloc(sizeof(LADSPA_Data*));
}

void test_connect(LADSPA_Handle h, unsigned long p, LADSPA_Data *buff) {
  h = (LADSPA_Handle)buff;
}

void test_activate(LADSPA_Handle h) {}

void test_run(LADSPA_Handle h, unsigned long sc) {
  LADSPA_Data *ibuff = (LADSPA_Data*)h;
  unsigned int i = 0;
  for (; i < sc; i++) {
    printf("[%06u:%f]\t", i, ibuff[i]);
  }
}

void test_run_adding(LADSPA_Handle h, unsigned long sc) {}

void test_set_run_adding_gain(LADSPA_Handle h, LADSPA_Data gain) {}

void test_deactivate(LADSPA_Handle h) {}

void test_cleanup(LADSPA_Handle h) {}

static const char const *test_ports_names[] = {
  "input"
};

static const LADSPA_PortDescriptor test_ports[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO
};

static const LADSPA_PortRangeHint test_hints[] = {
  {0,0,0}
};

static const LADSPA_Descriptor test_pl_desc = {
  9997,
  "test",
  LADSPA_PROPERTY_REALTIME |
  LADSPA_PROPERTY_HARD_RT_CAPABLE,
  "AATK - Test",
  "MrAmp",
  "",
  3,
  test_ports,
  test_ports_names,
  test_hints,
  (void*)0,
  test_make,
  test_connect,
  test_activate,
  test_run,
  test_run_adding,
  test_set_run_adding_gain,
  test_deactivate,
  test_cleanup
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
  if (index == 0)
    return &test_pl_desc;
  return NULL;
}
