/* SPDX-License-Identifier: GPL-3.0-or-later
 * C11 PKSM frontend can read Phase17 save status without legacy h168 types.
 * Shape validation is NOT a write permission or a CMAC attestation.
 */
#include "installed_save_probe_v1.h"
#include <assert.h>
#include <string.h>
int main(void) {
    P17SaveProbeReportV1 report;
    assert(sizeof(report)==P17_SAVE_PROBE_REPORT_SIZE);
    assert(p17_save_probe_init_v1(&report));
    assert(p17_save_probe_report_shape_v1(&report));
    assert(report.code==P17_SAVE_NOT_CHECKED && report.capability_bits==0);
    report.capability_bits=1;
    assert(!p17_save_probe_report_shape_v1(&report));
    report.capability_bits=0;
    report.version=2;
    assert(!p17_save_probe_report_shape_v1(&report));
    report.version=1;
    report.code=255;
    assert(!p17_save_probe_report_shape_v1(&report));
    assert(!p17_save_probe_report_shape_v1(0));
    assert(p17_save_probe_init_v1(&report));
    report.container_sha256[0]=1;
    assert(!p17_save_probe_report_shape_v1(&report));
    assert(p17_save_probe_init_v1(&report));
    report.code=P17_SAVE_READ_ONLY_VERIFIED;
    report.title_id=P17_SAVE_PROBE_EXPECTED_TITLE;
    report.container_bytes=P17_SAVE_PROBE_CONTAINER_BYTES;
    report.raw_bytes=P17_SAVE_PROBE_RAW_BYTES;
    report.slot0_counter=48;
    report.slot1_counter=49;
    report.active_slot=1;
    assert(p17_save_probe_report_shape_v1(&report)); /* STRUCTURE only! */
    report.active_slot=0;
    assert(!p17_save_probe_report_shape_v1(&report));
    return 0;
}
