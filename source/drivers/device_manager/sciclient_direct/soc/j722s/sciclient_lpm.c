
/*
 * Copyright (c) 2026, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file sciclient_lpm.c
 *
 * \brief Implements low power mode sequence
 *
 */

#include <drivers/hw_include/j722s/cslr_wkup_r5fss0_baseaddress.h>
#include <tisci/tisci_protocol.h>
#include <tisci/lpm/tisci_lpm.h>
#include <lib/trace_protocol.h>
#include <types/short_types.h>
#include <types/errno.h>
#include <lib/trace.h>
#include <osal_hwi.h>
#include "sciclient_s2r.h"

s32 dm_prepare_sleep_handler(u32 *msg_recv)
{
	s32 ret = -EFAIL;

	pm_trace(TRACE_PM_ACTION_MSG_RECEIVED, TISCI_MSG_PREPARE_SLEEP);

	return ret;
}

s32 dm_enter_sleep_handler(u32 *msg_recv)
{
	struct tisci_msg_enter_sleep_req *req =
		(struct tisci_msg_enter_sleep_req *) msg_recv;

	s32 ret = SUCCESS;
	u8 mode = req->mode;

	/* Check if the input mode is valid */
	if (mode != TISCI_MSG_VALUE_SLEEP_MODE_SOC_OFF) {
		ret = -EINVAL;
	} else {
		(void)osal_hwip_disable();

		S2R_goRetention();
		/* Never reach this point */
	}

	return ret;
}

s32 dm_lpm_wake_reason_handler(u32 *msg_recv)
{
	s32 ret = -EFAIL;

	pm_trace(TRACE_PM_ACTION_MSG_RECEIVED, TISCI_MSG_LPM_WAKE_REASON);

	return ret;
}

s32 dm_set_io_isolation_handler(u32 *msg_recv)
{
	s32 ret = -EFAIL;

	pm_trace(TRACE_PM_ACTION_MSG_RECEIVED, TISCI_MSG_SET_IO_ISOLATION);

	return ret;
}

s32 dm_lpm_get_device_constraint(u32 *msg_recv)
{
	s32 ret = -EFAIL;

	pm_trace(TRACE_PM_ACTION_MSG_RECEIVED, TISCI_MSG_LPM_GET_DEVICE_CONSTRAINT);

	return ret;
}

s32 dm_lpm_set_latency_constraint(u32 *msg_recv)
{
	s32 ret = -EFAIL;

	pm_trace(TRACE_PM_ACTION_MSG_RECEIVED, TISCI_MSG_LPM_SET_LATENCY_CONSTRAINT);

	return ret;
}

s32 dm_lpm_get_latency_constraint(u32 *msg_recv)
{
	s32 ret = -EFAIL;

	pm_trace(TRACE_PM_ACTION_MSG_RECEIVED, TISCI_MSG_LPM_GET_LATENCY_CONSTRAINT);

	return ret;
}

s32 dm_lpm_get_next_sys_mode(u32 *msg_recv)
{
	s32 ret = SUCCESS;
	struct tisci_msg_lpm_get_next_sys_mode_resp *resp =
		(struct tisci_msg_lpm_get_next_sys_mode_resp *) msg_recv;

	pm_trace(TRACE_PM_ACTION_MSG_RECEIVED, TISCI_MSG_LPM_GET_NEXT_SYS_MODE);

	resp->hdr.flags = 0U;

	resp->mode = TISCI_MSG_VALUE_SLEEP_MODE_SOC_OFF;

	return ret;
}

s32 dm_lpm_get_next_host_state(u32 *msg_recv)
{
	s32 ret = -EFAIL;

	pm_trace(TRACE_PM_ACTION_MSG_RECEIVED, TISCI_MSG_LPM_GET_NEXT_HOST_STATE);

	return ret;
}

s32 dm_lpm_abort(u32 *msg_recv)
{
	s32 ret = -EFAIL;

	pm_trace(TRACE_PM_ACTION_MSG_RECEIVED, TISCI_MSG_LPM_ABORT);

	return ret;
}

s32 dm_lpm_set_device_constraint(u32 *msg_recv)
{
	s32 ret = -EFAIL;

	pm_trace(TRACE_PM_ACTION_MSG_RECEIVED, TISCI_MSG_LPM_SET_DEVICE_CONSTRAINT);

	return ret;
}

void lpm_trace_reconfigure(u16 src_enables, u16 dst_enables)
{
}
