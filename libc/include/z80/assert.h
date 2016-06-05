/* See LICENSE file for copyright and license details. */
#ifndef _ASSERT_H
#define _ASSERT_H

#ifndef NDEBUG
#define assert(exp) __assert(#exp, __FILE__, __LINE__)
#endif

#endif
