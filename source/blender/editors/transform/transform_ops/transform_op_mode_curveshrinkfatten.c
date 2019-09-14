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
#include "transform_convert.h"

/* -------------------------------------------------------------------- */
/* Transform (Curve Shrink/Fatten) */

/** \name Transform Curve Shrink/Fatten
 * \{ */

static void applyCurveShrinkFatten(TransInfo *t, const int UNUSED(mval[2]))
{
  float ratio;
  int i;
  char str[UI_MAX_DRAW_STR];

  ratio = t->values[0];

  snapGridIncrement(t, &ratio);

  applyNumInput(&t->num, &ratio);

  t->values_final[0] = ratio;

  /* header print for NumInput */
  if (hasNumInput(&t->num)) {
    char c[NUM_STR_REP_LEN];

    outputNumInput(&(t->num), c, &t->scene->unit);
    BLI_snprintf(str, sizeof(str), TIP_("Shrink/Fatten: %s"), c);
  }
  else {
    BLI_snprintf(str, sizeof(str), TIP_("Shrink/Fatten: %3f"), ratio);
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
        *td->val = td->ival * ratio;
        /* apply PET */
        *td->val = (*td->val * td->factor) + ((1.0f - td->factor) * td->ival);
        if (*td->val <= 0.0f) {
          *td->val = 0.001f;
        }
      }
    }
  }

  recalcData(t);

  ED_area_status_text(t->sa, str);
}

void initCurveShrinkFatten(TransInfo *t)
{
  t->mode = TFM_CURVE_SHRINKFATTEN;
  t->transform = applyCurveShrinkFatten;

  initMouseInputMode(t, &t->mouse, INPUT_SPRING);

  t->idx_max = 0;
  t->num.idx_max = 0;
  t->snap[0] = 0.0f;
  t->snap[1] = 0.1f;
  t->snap[2] = t->snap[1] * 0.1f;

  copy_v3_fl(t->num.val_inc, t->snap[1]);
  t->num.unit_sys = t->scene->unit.system;
  t->num.unit_type[0] = B_UNIT_NONE;

  t->flag |= T_NO_ZERO;
#ifdef USE_NUM_NO_ZERO
  t->num.val_flag[0] |= NUM_NO_ZERO;
#endif

  t->flag |= T_NO_CONSTRAINT;
}
/** \} */
