/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#include <string.h>

#include "malloc.h"

void *
realloc(void *ptr, size_t nbytes)
{
	Header *oh, *prev, *next, *new;
	size_t nunits, avail, onbytes, n;

	if (!nbytes)
		return NULL;

	if (!ptr)
		return malloc(nbytes);

	nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
	oh = (Header*)ptr - 1;

	if (oh->h.size == nunits)
		return ptr;

	new = oh + nunits;

	if (nunits < oh->h.size - 1) {
		new->h.size = oh->h.size - nunits;
		oh->h.size = nunits;
		free(new + 1);
		return oh;
	}

	prev = _prevchunk(oh);

	if (oh + oh->h.size == prev->h.next) {
		/*
		 * if there is free space adjacent
		 * to the current memory
		 */
		next = prev->h.next;
		avail = oh->h.size + next->h.size;

		if (avail == nunits) {
			oh->h.size = nunits;
			prev->h.next = next->h.next;
			return oh;
		}

		if (avail > nunits) {
			oh->h.size = nunits;
			prev->h.next = new;
			new->h.next = next;
			new->h.size = avail - nunits;
			return oh;
		}
	}

	onbytes = (oh->h.size - 1) * sizeof(Header);
	if ((new = malloc(nbytes)) == NULL)
		return NULL;

	n = (onbytes > nbytes) ? nbytes : onbytes;
	memcpy(new, ptr, n);
	free(ptr);

	return new;
}
