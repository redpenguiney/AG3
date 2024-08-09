#pragma once
#ifndef ASSERTIONS
#define ASSERTIONS
#include "log.hpp"

#define DO_ASSERTS

// basically a copy of cassert. used because visual studio defines NDEBUG in release builds and often we don't want that.
inline void private_assert(const char* file, int line, bool predicate, const char* assertion) {
#ifdef DO_ASSERTS
	if (!predicate) {
		DebugLogError("Assertion ", assertion, " not met at line ", line, " of \"", file, "\". ");
		abort();
	}
#endif
};

#define Assert(condition ) private_assert(__FILE__, __LINE__, static_cast<bool>(condition), #condition)
#endif