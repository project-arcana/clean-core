#pragma once

#include "sbo_string.hh"

namespace cc
{
/**
 * utf8 null-terminated string with small-string-optimizations
 *
 * TODO: maybe a different SBO strategy
 *       https://stackoverflow.com/questions/27631065/why-does-libcs-implementation-of-stdstring-take-up-3x-memory-as-libstdc/28003328#28003328
 *       https://stackoverflow.com/questions/10315041/meaning-of-acronym-sso-in-the-context-of-stdstring/10319672#10319672
 *       maybe via template arg the sbo capacity?
 */
struct string : sbo_string<15>
{
};

static_assert(sizeof(string) == 4 * 8, "wrong architecture?");
}
