/*
 * Copyright (C) 2023 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WIN_UTIL_H
#define _WIN_UTIL_H

#include "platform_wasi.h"
#include "windows.h"

// Convert a FILETIME struct to a WASI timestamp
__wasi_timestamp_t
convert_filetime(LPFILETIME filetime);

#endif /* end of _WIN_UTIL_H */