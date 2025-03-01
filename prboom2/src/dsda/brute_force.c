//
// Copyright(C) 2022 by Ryan Krafnick
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	DSDA Brute Force
//

#include <math.h>

#include "d_player.h"
#include "d_ticcmd.h"
#include "doomstat.h"
#include "lprintf.h"
#include "m_random.h"
#include "r_state.h"

#include "dsda/build.h"
#include "dsda/features.h"
#include "dsda/key_frame.h"
#include "dsda/skip.h"
#include "dsda/time.h"
#include "dsda/utility.h"

#include "brute_force.h"

#define MAX_BF_DEPTH 35
#define MAX_BF_CONDITIONS 16

typedef struct {
  int min;
  int max;
  int i;
} bf_range_t;

typedef struct {
  dsda_key_frame_t key_frame;
  bf_range_t forwardmove;
  bf_range_t sidemove;
  bf_range_t angleturn;
} bf_t;

typedef struct {
  dsda_bf_attribute_t attribute;
  dsda_bf_operator_t operator;
  fixed_t value;
  fixed_t secondary_value;
} bf_condition_t;

typedef struct {
  dsda_bf_attribute_t attribute;
  dsda_bf_limit_t limit;
  fixed_t value;
  dboolean enabled;
  dboolean evaluated;
  fixed_t best_value;
  int best_depth;
  bf_t best_bf[MAX_BF_DEPTH];
} bf_target_t;

static bf_t brute_force[MAX_BF_DEPTH];
static int bf_depth;
static int bf_logictic;
static int bf_condition_count;
static bf_condition_t bf_condition[MAX_BF_CONDITIONS];
static long long bf_volume;
static long long bf_volume_max;
static dboolean bf_mode;
static bf_target_t bf_target;
static ticcmd_t bf_result[MAX_BF_DEPTH];

const char* dsda_bf_attribute_names[dsda_bf_attribute_max] = {
  [dsda_bf_x] = "x",
  [dsda_bf_y] = "y",
  [dsda_bf_z] = "z",
  [dsda_bf_momx] = "vx",
  [dsda_bf_momy] = "vy",
  [dsda_bf_speed] = "spd",
  [dsda_bf_damage] = "dmg",
  [dsda_bf_rng] = "rng",
};

const char* dsda_bf_misc_names[dsda_bf_misc_max] = {
  "line skip",
  "line activation",
};

const char* dsda_bf_operator_names[dsda_bf_operator_max] = {
  [dsda_bf_less_than] = "<",
  [dsda_bf_less_than_or_equal_to] = "<=",
  [dsda_bf_greater_than] = ">",
  [dsda_bf_greater_than_or_equal_to] = ">=",
  [dsda_bf_equal_to] = "==",
  [dsda_bf_not_equal_to] = "!="
};

const char* dsda_bf_limit_names[dsda_bf_limit_max] = {
  "acap",
  "max",
  "min",
};

static dboolean fixed_point_attribute[dsda_bf_attribute_max] = {
  [dsda_bf_x] = true,
  [dsda_bf_y] = true,
  [dsda_bf_z] = true,
  [dsda_bf_momx] = true,
  [dsda_bf_momy] = true,
  [dsda_bf_speed] = true,
  [dsda_bf_damage] = true,
  [dsda_bf_rng] = false,
};

static dboolean dsda_AdvanceBFRange(bf_range_t* range) {
  ++range->i;

  if (range->i > range->max) {
    range->i = range->min;
    return false;
  }

  return true;
}

static dboolean dsda_AdvanceBruteForceFrame(int frame) {
  if (!dsda_AdvanceBFRange(&brute_force[frame].angleturn))
    if (!dsda_AdvanceBFRange(&brute_force[frame].sidemove))
      if (!dsda_AdvanceBFRange(&brute_force[frame].forwardmove))
        return false;

  return true;
}

static int dsda_AdvanceBruteForce(void) {
  int i;

  for (i = bf_depth - 1; i >= 0; --i)
    if (dsda_AdvanceBruteForceFrame(i))
      break;

  return i;
}

static void dsda_CopyBFCommandDepth(ticcmd_t* cmd, bf_t* bf) {
  memset(cmd, 0, sizeof(*cmd));

  cmd->angleturn = bf->angleturn.i << 8;
  cmd->forwardmove = bf->forwardmove.i;
  cmd->sidemove = bf->sidemove.i;
}

static void dsda_CopyBFResult(bf_t* bf, int depth) {
  int i;

  for (i = 0; i < depth; ++i)
    dsda_CopyBFCommandDepth(&bf_result[i], &bf[i]);

  if (i != MAX_BF_DEPTH)
    memset(&bf_result[i], 0, sizeof(ticcmd_t) * (MAX_BF_DEPTH - i));
}

static void dsda_RestoreBFKeyFrame(int frame) {
  dsda_RestoreKeyFrame(&brute_force[frame].key_frame, true);
}

static void dsda_StoreBFKeyFrame(int frame) {
  dsda_StoreKeyFrame(&brute_force[frame].key_frame, true, false);
}

static void dsda_PrintBFProgress(void) {
  int percent;
  unsigned long long elapsed_time;

  percent = 100 * bf_volume / bf_volume_max;
  elapsed_time = dsda_ElapsedTimeMS(dsda_timer_brute_force);

  lprintf(LO_INFO, "  %lld / %lld sequences tested (%d%%) in %.2f seconds!\n",
          bf_volume, bf_volume_max, percent, (float) elapsed_time / 1000);
}

#define BF_FAILURE 0
#define BF_SUCCESS 1

static const char* bf_result_text[2] = { "FAILURE", "SUCCESS" };
static dboolean brute_force_ended;

dboolean dsda_BruteForceEnded(void) {
  return brute_force_ended;
}

static void dsda_EndBF(int result) {
  brute_force_ended = true;

  lprintf(LO_INFO, "Brute force complete (%s)!\n", bf_result_text[result]);
  dsda_PrintBFProgress();

  dsda_RestoreBFKeyFrame(0);

  bf_mode = false;

  if (result == BF_SUCCESS)
    dsda_QueueBuildCommands(bf_result, bf_depth);
  else
    dsda_ExitSkipMode();
}

static fixed_t dsda_BFAttribute(int attribute) {
  player_t* player;

  player = &players[displayplayer];

  switch (attribute) {
    case dsda_bf_x:
      return player->mo->x;
    case dsda_bf_y:
      return player->mo->y;
    case dsda_bf_z:
      return player->mo->z;
    case dsda_bf_momx:
      return player->mo->momx;
    case dsda_bf_momy:
      return player->mo->momy;
    case dsda_bf_speed:
      return P_PlayerSpeed(player);
    case dsda_bf_damage:
      {
        extern int player_damage_last_tic;

        return player_damage_last_tic;
      }
    case dsda_bf_rng:
      return rng.rndindex;
    default:
      return 0;
  }
}

static dboolean dsda_BFMiscConditionReached(int i) {
  switch (bf_condition[i].attribute) {
    case dsda_bf_line_skip:
      return lines[bf_condition[i].value].player_activations == bf_condition[i].secondary_value;
    case dsda_bf_line_activation:
      return lines[bf_condition[i].value].player_activations > bf_condition[i].secondary_value;
    default:
      return false;
  }
}

static dboolean dsda_BFConditionReached(int i) {
  fixed_t value;

  if (bf_condition[i].operator == dsda_bf_operator_misc)
    return dsda_BFMiscConditionReached(i);

  value = dsda_BFAttribute(bf_condition[i].attribute);

  switch (bf_condition[i].operator) {
    case dsda_bf_less_than:
      return value < bf_condition[i].value;
    case dsda_bf_less_than_or_equal_to:
      return value <= bf_condition[i].value;
    case dsda_bf_greater_than:
      return value > bf_condition[i].value;
    case dsda_bf_greater_than_or_equal_to:
      return value >= bf_condition[i].value;
    case dsda_bf_equal_to:
      return value == bf_condition[i].value;
    case dsda_bf_not_equal_to:
      return value != bf_condition[i].value;
    default:
      return false;
  }
}

static void dsda_BFUpdateBestResult(fixed_t value) {
  int i;
  char str[FIXED_STRING_LENGTH];
  char cmd_str[COMMAND_MOVEMENT_STRING_LENGTH];

  bf_target.evaluated = true;
  bf_target.best_value = value;
  bf_target.best_depth = logictic - bf_logictic;

  for (i = 0; i < bf_target.best_depth; ++i)
    bf_target.best_bf[i] = brute_force[i];

  dsda_CopyBFResult(bf_target.best_bf, bf_target.best_depth);

  if (fixed_point_attribute[bf_target.attribute])
    dsda_FixedToString(str, value);
  else
    snprintf(str, FIXED_STRING_LENGTH, "%i", value);

  lprintf(LO_INFO, "New best: %s = %s\n", dsda_bf_attribute_names[bf_target.attribute], str);

  for (i = 0; i < bf_target.best_depth; ++i) {
    dsda_PrintCommandMovement(cmd_str, &bf_result[i]);
    lprintf(LO_INFO, "    %s\n", cmd_str);
  }

  lprintf(LO_INFO, "\n");
}

static dboolean dsda_BFNewBestResult(fixed_t value) {
  if (!bf_target.evaluated)
    return true;

  switch (bf_target.limit) {
    case dsda_bf_acap:
      return abs(value - bf_target.value) < abs(bf_target.best_value - bf_target.value);
    case dsda_bf_max:
      return value > bf_target.best_value;
    case dsda_bf_min:
      return value < bf_target.best_value;
    default:
      return false;
  }
}

static void dsda_BFEvaluateTarget(void) {
  fixed_t value;

  value = dsda_BFAttribute(bf_target.attribute);

  if (dsda_BFNewBestResult(value))
    dsda_BFUpdateBestResult(value);
}

static dboolean dsda_BFConditionsReached(void) {
  int i, reached;

  reached = 0;
  for (i = 0; i < bf_condition_count; ++i)
    reached += dsda_BFConditionReached(i);

  if (reached == bf_condition_count)
    if (bf_target.enabled) {
      dsda_BFEvaluateTarget();

      return false;
    }

  return reached == bf_condition_count;
}

dboolean dsda_BruteForce(void) {
  return bf_mode;
}

void dsda_ResetBruteForceConditions(void) {
  bf_condition_count = 0;
  memset(&bf_target, 0, sizeof(bf_target));
}

void dsda_AddMiscBruteForceCondition(dsda_bf_attribute_t attribute, fixed_t value) {
  if (bf_condition_count == MAX_BF_CONDITIONS)
    return;

  bf_condition[bf_condition_count].attribute = attribute;
  bf_condition[bf_condition_count].operator = dsda_bf_operator_misc;
  bf_condition[bf_condition_count].value = value;

  switch (attribute) {
    case dsda_bf_line_skip:
    case dsda_bf_line_activation:
      bf_condition[bf_condition_count].secondary_value = lines[value].player_activations;
      break;
    default:
      break;
  }

  ++bf_condition_count;

  lprintf(LO_INFO, "Added brute force condition: %s %d\n",
                   dsda_bf_misc_names[attribute],
                   value);
}

void dsda_AddBruteForceCondition(dsda_bf_attribute_t attribute,
                                 dsda_bf_operator_t operator, fixed_t value) {
  if (bf_condition_count == MAX_BF_CONDITIONS)
    return;

  bf_condition[bf_condition_count].attribute = attribute;
  bf_condition[bf_condition_count].operator = operator;
  bf_condition[bf_condition_count].value = value;

  if (fixed_point_attribute[attribute])
    bf_condition[bf_condition_count].value <<= FRACBITS;

  ++bf_condition_count;

  lprintf(LO_INFO, "Added brute force condition: %s %s %d\n",
                   dsda_bf_attribute_names[attribute],
                   dsda_bf_operator_names[operator],
                   value);
}

void dsda_SetBruteForceTarget(dsda_bf_attribute_t attribute,
                              dsda_bf_limit_t limit, fixed_t value) {
  bf_target.attribute = attribute;
  bf_target.limit = limit;
  bf_target.value = value;
  bf_target.best_value = dsda_BFAttribute(attribute);
  bf_target.enabled = true;

  lprintf(LO_INFO, "Set brute force target: %s %s %d\n",
                   dsda_bf_attribute_names[attribute],
                   dsda_bf_limit_names[limit],
                   value);
}

static void dsda_SortIntPair(int* a, int* b) {
  if (*a > *b) {
    int temp;

    temp = *a;
    *a = *b;
    *b = temp;
  }
}

int dsda_AddBruteForceFrame(int i,
                            int forwardmove_min, int forwardmove_max,
                            int sidemove_min, int sidemove_max,
                            int angleturn_min, int angleturn_max) {
  if (i < 0 || i >= MAX_BF_DEPTH)
    return false;

  dsda_SortIntPair(&forwardmove_min, &forwardmove_max);
  dsda_SortIntPair(&sidemove_min, &sidemove_max);
  dsda_SortIntPair(&angleturn_min, &angleturn_max);

  brute_force[i].forwardmove.min = forwardmove_min;
  brute_force[i].forwardmove.max = forwardmove_max;
  brute_force[i].forwardmove.i = forwardmove_min;

  brute_force[i].sidemove.min = sidemove_min;
  brute_force[i].sidemove.max = sidemove_max;
  brute_force[i].sidemove.i = sidemove_min;

  brute_force[i].angleturn.min = angleturn_min;
  brute_force[i].angleturn.max = angleturn_max;
  brute_force[i].angleturn.i = angleturn_min;

  return true;
}

dboolean dsda_StartBruteForce(int depth) {
  int i;

  if (depth <= 0 || depth > MAX_BF_DEPTH)
    return false;

  dsda_TrackFeature(uf_bruteforce);

  lprintf(LO_INFO, "Brute force starting:\n");

  bf_depth = depth;
  bf_logictic = logictic;
  bf_volume = 0;
  bf_volume_max = 1;

  for (i = 0; i < bf_depth; ++i) {
    lprintf(LO_INFO, "  %d: F %d:%d S %d:%d T %d:%d\n", i,
            brute_force[i].forwardmove.min, brute_force[i].forwardmove.max,
            brute_force[i].sidemove.min, brute_force[i].sidemove.max,
            brute_force[i].angleturn.min, brute_force[i].angleturn.max);

    bf_volume_max *= (brute_force[i].forwardmove.max - brute_force[i].forwardmove.min + 1) *
                     (brute_force[i].sidemove.max - brute_force[i].sidemove.min + 1) *
                     (brute_force[i].angleturn.max - brute_force[i].angleturn.min + 1);
  }

  lprintf(LO_INFO, "Testing %lld sequences with depth %d\n\n", bf_volume_max, bf_depth);

  bf_mode = true;

  dsda_EnterSkipMode();

  dsda_StartTimer(dsda_timer_brute_force);

  return true;
}

void dsda_UpdateBruteForce(void) {
  int frame;

  frame = logictic - bf_logictic;

  if (frame == bf_depth) {
    if (bf_volume % 10000 == 0)
      dsda_PrintBFProgress();

    frame = dsda_AdvanceBruteForce();

    if (frame >= 0)
      dsda_RestoreBFKeyFrame(frame);
  }
  else
    dsda_StoreBFKeyFrame(frame);
}

void dsda_EvaluateBruteForce(void) {
  if (logictic - bf_logictic != bf_depth)
    return;

  ++bf_volume;

  if (dsda_BFConditionsReached()) {
    dsda_CopyBFResult(brute_force, bf_depth);
    dsda_EndBF(BF_SUCCESS);
  }
  else if (bf_volume >= bf_volume_max) {
    if (bf_target.enabled && bf_target.evaluated)
      dsda_EndBF(BF_SUCCESS);
    else
      dsda_EndBF(BF_FAILURE);
  }
}

void dsda_CopyBruteForceCommand(ticcmd_t* cmd) {
  int depth;

  depth = logictic - bf_logictic;

  if (depth >= bf_depth) {
    memset(cmd, 0, sizeof(*cmd));

    return;
  }

  dsda_CopyBFCommandDepth(cmd, &brute_force[depth]);
}
