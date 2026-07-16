// uwp-shell/pch.h
// Precompiled header stub.
//
// We disable PCH in the vcxproj (<PrecompiledHeader>NotUsing</PrecompiledHeader>)
// because the project mixes C++17 + ForcedInclude (CommonPre.h) which doesn't
// play nicely with PCH. This file is provided for completeness so the IDE and
// any consumer that *wants* to use a PCH can `#include "pch.h"` and pull in
// the same set of headers that most TUs end up needing anyway.

#pragma once

// CommonPre.h is force-included on every TU via the vcxproj / -include flag,
// so it has already been processed by the time we reach this file. Including
// it again here is a no-op thanks to the XWR_COMMON_PRE_H include guard.
#include "CommonPre.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
