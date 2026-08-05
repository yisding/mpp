/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/* Copyright (c) 2026 */

#define MODULE_TAG "mpp_buf_slot_test"

#include "mpp_log.h"
#include "mpp_frame.h"
#include "mpp_buf_slot.h"

#define TEST_EVENT_COUNT  40

int main(void)
{
    MppBufSlots slots = NULL;
    MppFrame info = NULL;
    MppFrame slot_frame = NULL;
    MppFrame frame = NULL;
    RK_S32 index = -1;
    RK_S32 out_index = -1;
    RK_S32 i;
    MppCodingType coding = MPP_VIDEO_CodingVP9;
    MPP_RET ret = MPP_NOK;

    if (mpp_buf_slot_init(&slots) || mpp_buf_slot_setup(slots, 1)) {
        mpp_err("failed to initialize slots\n");
        goto done;
    }

    if (mpp_frame_init(&info)) {
        mpp_err("failed to initialize frame info\n");
        goto done;
    }

    mpp_slots_set_prop(slots, SLOTS_CODING_TYPE, &coding);

    mpp_frame_set_width(info, 64);
    mpp_frame_set_height(info, 64);
    mpp_frame_set_hor_stride(info, 64);
    mpp_frame_set_ver_stride(info, 64);

    if (mpp_buf_slot_get_unused(slots, &index) || index != 0 ||
        mpp_buf_slot_set_prop(slots, index, SLOT_FRAME, info) ||
        mpp_buf_slot_set_flag(slots, index, SLOT_CODEC_READY) ||
        mpp_buf_slot_get_prop(slots, index, SLOT_FRAME_PTR, &slot_frame)) {
        mpp_err("failed to prepare slot\n");
        goto done;
    }

    /* More than the old five-bit queue counter, all on the same slot. */
    for (i = 0; i < TEST_EVENT_COUNT; i++) {
        mpp_frame_set_pts(slot_frame, 1000 + i);
        mpp_frame_set_dts(slot_frame, 2000 + i);
        if (mpp_buf_slot_enqueue_frame(slots, index, slot_frame)) {
            mpp_err("failed to enqueue event %d\n", i);
            goto done;
        }
    }

    for (i = 0; i < TEST_EVENT_COUNT; i++) {
        frame = NULL;
        out_index = -1;
        if (mpp_buf_slot_dequeue_frame(slots, &out_index, &frame,
                                       QUEUE_DISPLAY) ||
            out_index != index || !frame ||
            mpp_frame_get_pts(frame) != 1000 + i ||
            mpp_frame_get_dts(frame) != 2000 + i) {
            mpp_err("event %d lost or metadata changed\n", i);
            goto done;
        }

        mpp_frame_deinit(&frame);
        mpp_buf_slot_clr_flag(slots, index, SLOT_QUEUE_USE);
    }

    if (!mpp_slots_is_empty(slots, QUEUE_DISPLAY)) {
        mpp_err("display queue not empty after dequeue\n");
        goto done;
    }

    ret = MPP_OK;

done:
    if (frame)
        mpp_frame_deinit(&frame);
    if (info)
        mpp_frame_deinit(&info);
    if (slots)
        mpp_buf_slot_deinit(slots);

    if (ret)
        mpp_err("mpp_buf_slot_test failed\n");
    else
        mpp_log("mpp_buf_slot_test success\n");

    return ret;
}
