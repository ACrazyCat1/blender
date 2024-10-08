/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

/** \file
 * \ingroup bke
 */

#include "BLI_sys_types.h"

struct Main;
struct ReportList;
struct bContext;

/** Paste-buffer helper API. For copy, use directly the #PartialWriteContext API. */

/**
 * Paste data-blocks from the given .blend file 'buffer' (i.e. append them).
 *
 * Unlike #BKE_copybuffer_paste, it does not perform any instantiation of collections/objects/etc.
 *
 * \param libname: Full path to the .blend file used as copy/paste buffer.
 * \param id_types_mask: Only directly link IDs of those types from the given .blend file buffer.
 *
 * \return true on success, false otherwise.
 */
bool BKE_copybuffer_read(Main *bmain_dst,
                         const char *libname,
                         ReportList *reports,
                         uint64_t id_types_mask);
/**
 * Paste data-blocks from the given .blend file 'buffer'  (i.e. append them).
 *
 * Similar to #BKE_copybuffer_read, but also handles instantiation of collections/objects/etc.
 *
 * \param libname: Full path to the .blend file used as copy/paste buffer.
 * \param flag: A combination of #eBLOLibLinkFlags and ##eFileSel_Params_Flag to control
 * link/append behavior.
 * \note Ignores #FILE_LINK flag, since it always appends IDs.
 * \param id_types_mask: Only directly link IDs of those types from the given .blend file buffer.
 *
 * \return Number of IDs directly pasted from the buffer
 * (does not includes indirectly linked ones).
 */
int BKE_copybuffer_paste(
    bContext *C, const char *libname, int flag, ReportList *reports, uint64_t id_types_mask);
