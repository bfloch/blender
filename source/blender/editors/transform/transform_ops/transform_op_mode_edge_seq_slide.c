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
 */

/** \file
 * \ingroup edtransform
 */

#include "MEM_guardedalloc.h"

#include "DNA_mesh_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"

#include "BLI_math.h"
#include "BLI_string.h"
#include "BLI_utildefines.h"

#include "BLT_translation.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_report.h"
#include "BKE_editmesh.h"
#include "BKE_layer.h"
#include "BKE_scene.h"
#include "BKE_unit.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_enum_types.h"

#include "WM_api.h"
#include "WM_message.h"
#include "WM_types.h"
#include "WM_toolsystem.h"

#include "UI_interface.h"
#include "UI_resources.h"

#include "ED_screen.h"
/* for USE_LOOPSLIDE_HACK only */
#include "ED_mesh.h"

#include "transform.h"
#include "transform_convert.h"
#include "transform_op.h"

const char OP_SEQ_SLIDE[] = "TRANSFORM_OT_seq_slide";

/* -------------------------------------------------------------------- */
/* Transform Init (Sequencer Slide) */

/** \name Transform Sequencer Slide
 * \{ */

static void headerSeqSlide(TransInfo *t, const float val[2], char str[UI_MAX_DRAW_STR])
{
  char tvec[NUM_STR_REP_LEN * 3];
  size_t ofs = 0;

  if (hasNumInput(&t->num)) {
    outputNumInput(&(t->num), tvec, &t->scene->unit);
  }
  else {
    BLI_snprintf(&tvec[0], NUM_STR_REP_LEN, "%.0f, %.0f", val[0], val[1]);
  }

  ofs += BLI_snprintf(
      str + ofs, UI_MAX_DRAW_STR - ofs, TIP_("Sequence Slide: %s%s, ("), &tvec[0], t->con.text);

  if (t->keymap) {
    wmKeyMapItem *kmi = WM_modalkeymap_find_propvalue(t->keymap, TFM_MODAL_TRANSLATE);
    if (kmi) {
      ofs += WM_keymap_item_to_string(kmi, false, str + ofs, UI_MAX_DRAW_STR - ofs);
    }
  }
  ofs += BLI_snprintf(str + ofs,
                      UI_MAX_DRAW_STR - ofs,
                      TIP_(" or Alt) Expand to fit %s"),
                      WM_bool_as_string((t->flag & T_ALT_TRANSFORM) != 0));
}

static void applySeqSlideValue(TransInfo *t, const float val[2])
{
  int i;

  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    TransData *td = tc->data;
    for (i = 0; i < tc->data_len; i++, td++) {
      if (td->flag & TD_NOACTION) {
        break;
      }

      if (td->flag & TD_SKIP) {
        continue;
      }

      madd_v2_v2v2fl(td->loc, td->iloc, val, td->factor);
    }
  }
}

static void applySeqSlide(TransInfo *t, const int mval[2])
{
  char str[UI_MAX_DRAW_STR];

  snapSequenceBounds(t, mval);

  if (t->con.mode & CON_APPLY) {
    float pvec[3] = {0.0f, 0.0f, 0.0f};
    float tvec[3];
    t->con.applyVec(t, NULL, NULL, t->values, tvec, pvec);
    copy_v3_v3(t->values_final, tvec);
  }
  else {
    // snapGridIncrement(t, t->values);
    applyNumInput(&t->num, t->values);
    copy_v3_v3(t->values_final, t->values);
  }

  t->values_final[0] = floorf(t->values_final[0] + 0.5f);
  t->values_final[1] = floorf(t->values_final[1] + 0.5f);

  headerSeqSlide(t, t->values_final, str);
  applySeqSlideValue(t, t->values_final);

  recalcData(t);

  ED_area_status_text(t->sa, str);
}

void initSeqSlide(TransInfo *t)
{
  t->transform = applySeqSlide;

  initMouseInputMode(t, &t->mouse, INPUT_VECTOR);

  t->idx_max = 1;
  t->num.flag = 0;
  t->num.idx_max = t->idx_max;

  t->snap[0] = 0.0f;
  t->snap[1] = floorf(t->scene->r.frs_sec / t->scene->r.frs_sec_base);
  t->snap[2] = 10.0f;

  copy_v3_fl(t->num.val_inc, t->snap[1]);
  t->num.unit_sys = t->scene->unit.system;
  /* Would be nice to have a time handling in units as well
   * (supporting frames in addition to "natural" time...). */
  t->num.unit_type[0] = B_UNIT_NONE;
  t->num.unit_type[1] = B_UNIT_NONE;
}
/** \} */

void TRANSFORM_OT_seq_slide(struct wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Sequence Slide";
  ot->description = "Slide a sequence strip in time";
  ot->idname = OP_SEQ_SLIDE;
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO | OPTYPE_BLOCKING;

  /* api callbacks */
  ot->invoke = transform_invoke;
  ot->exec = transform_exec;
  ot->modal = transform_modal;
  ot->cancel = transform_cancel;
  ot->poll = ED_operator_sequencer_active;

  /* properties */
  PropertyRNA *prop;

  prop = RNA_def_float_vector(
      ot->srna, "value", 2, NULL, -FLT_MAX, FLT_MAX, "Offset", "", -FLT_MAX, FLT_MAX);
  RNA_def_property_ui_range(prop, -FLT_MAX, FLT_MAX, 1, 0);

  WM_operatortype_props_advanced_begin(ot);

  Transform_Properties(ot, P_SNAP);
}
