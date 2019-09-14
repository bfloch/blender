/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 */

/** \file
 * \ingroup edtransform
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "MEM_guardedalloc.h"

#include "DNA_anim_types.h"
#include "DNA_armature_types.h"
#include "DNA_constraint_types.h"
#include "DNA_mask_types.h"
#include "DNA_mesh_types.h"
#include "DNA_movieclip_types.h"
#include "DNA_scene_types.h" /* PET modes */
#include "DNA_workspace_types.h"
#include "DNA_gpencil_types.h"

#include "BLI_alloca.h"
#include "BLI_utildefines.h"
#include "BLI_math.h"
#include "BLI_rect.h"
#include "BLI_listbase.h"
#include "BLI_string.h"
#include "BLI_ghash.h"
#include "BLI_utildefines_stack.h"
#include "BLI_memarena.h"

#include "BKE_nla.h"
#include "BKE_editmesh.h"
#include "BKE_editmesh_bvh.h"
#include "BKE_context.h"
#include "BKE_constraint.h"
#include "BKE_particle.h"
#include "BKE_unit.h"
#include "BKE_scene.h"
#include "BKE_mask.h"
#include "BKE_mesh.h"
#include "BKE_report.h"
#include "BKE_workspace.h"

#include "DEG_depsgraph.h"

#include "GPU_immediate.h"
#include "GPU_matrix.h"
#include "GPU_state.h"

#include "ED_image.h"
#include "ED_keyframing.h"
#include "ED_screen.h"
#include "ED_space_api.h"
#include "ED_markers.h"
#include "ED_view3d.h"
#include "ED_mesh.h"
#include "ED_clip.h"
#include "ED_node.h"
#include "ED_gpencil.h"
#include "ED_sculpt.h"

#include "WM_types.h"
#include "WM_api.h"

#include "UI_view2d.h"
#include "UI_interface.h"
#include "UI_interface_icons.h"
#include "UI_resources.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "BLF_api.h"
#include "BLT_translation.h"

#include "transform.h"

/* -------------------------------------------------------------------- */
/* Transform (Bake-Time) */

/** \name Transform Bake-Time
 * \{ */

static void applyBakeTime(TransInfo *t, const int mval[2])
{
  float time;
  int i;
  char str[UI_MAX_DRAW_STR];

  float fac = 0.1f;

  /* XXX, disable precision for now,
   * this isn't even accessible by the user */
#if 0
  if (t->mouse.precision) {
    /* calculate ratio for shiftkey pos, and for total, and blend these for precision */
    time = (float)(t->center2d[0] - t->mouse.precision_mval[0]) * fac;
    time += 0.1f * ((float)(t->center2d[0] * fac - mval[0]) - time);
  }
  else
#endif
  {
    time = (float)(t->center2d[0] - mval[0]) * fac;
  }

  snapGridIncrement(t, &time);

  applyNumInput(&t->num, &time);

  /* header print for NumInput */
  if (hasNumInput(&t->num)) {
    char c[NUM_STR_REP_LEN];

    outputNumInput(&(t->num), c, &t->scene->unit);

    if (time >= 0.0f) {
      BLI_snprintf(str, sizeof(str), TIP_("Time: +%s %s"), c, t->proptext);
    }
    else {
      BLI_snprintf(str, sizeof(str), TIP_("Time: %s %s"), c, t->proptext);
    }
  }
  else {
    /* default header print */
    if (time >= 0.0f) {
      BLI_snprintf(str, sizeof(str), TIP_("Time: +%.3f %s"), time, t->proptext);
    }
    else {
      BLI_snprintf(str, sizeof(str), TIP_("Time: %.3f %s"), time, t->proptext);
    }
  }

  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    TransData *td = tc->data;
    for (i = 0; i < tc->data_len; i++, td++) {
      if (td->flag & TD_NOACTION) {
        break;
      }

      if (td->flag & TD_SKIP) {
        continue;
      }

      if (td->val) {
        *td->val = td->ival + time * td->factor;
        if (td->ext->size && *td->val < *td->ext->size) {
          *td->val = *td->ext->size;
        }
        if (td->ext->quat && *td->val > *td->ext->quat) {
          *td->val = *td->ext->quat;
        }
      }
    }
  }

  recalcData(t);

  ED_area_status_text(t->sa, str);
}

void initBakeTime(TransInfo *t)
{
  t->transform = applyBakeTime;
  initMouseInputMode(t, &t->mouse, INPUT_NONE);

  t->idx_max = 0;
  t->num.idx_max = 0;
  t->snap[0] = 0.0f;
  t->snap[1] = 1.0f;
  t->snap[2] = t->snap[1] * 0.1f;

  copy_v3_fl(t->num.val_inc, t->snap[1]);
  t->num.unit_sys = t->scene->unit.system;
  t->num.unit_type[0] = B_UNIT_NONE; /* Don't think this uses units? */
}
/** \} */
