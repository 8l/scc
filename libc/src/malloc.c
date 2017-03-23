/* See LICENSE file for copyright and license details. */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "malloc.h"
#include "syscall.h"

#define MAXADDR ((char *)-1)
#define ERRADDR ((char *)-1)

extern char end[];
static Header base = { .h.next = &base };
static Header *freep = &base;
static char *heap = end;

/*
 * Run over the free list looking for the nearest previous
 * block. There are two possible results: end of the list
 * or an intermediary block.
 */
void *
_prevchunk(Header *hp)
{
	Header *p;

	for (p = freep; ;p = p->h.next) {
		/* hp between p and p->h.next? */
		if (p < hp && hp < p->h.next)
			break;
		/* p before hp and hp at the end of list? */
		if (p->h.next <= p && (hp < p->h.next || hp > p))
			break;
	}
	return p;
}

/*
 * Get the previous block and try to merge
 * with next and previous blocks
 */
void
free(void *mem)
{
	Header *hp, *prev;

	if (!mem)
		return;

	hp = (Header *) mem - 1;
	prev = _prevchunk(hp);

	/* join to next */
	if (hp + hp->h.size == prev->h.next) {
		hp->h.size += prev->h.next->h.size;
		hp->h.next = prev->h.next->h.next;
	} else {
		hp->h.next = prev->h.next;
	}

	/* join to previous */
	if (prev + prev->h.size == hp) {
		prev->h.size += hp->h.size;
		prev->h.next = hp->h.next;
	} else {
		prev->h.next = hp;
	}

	freep = prev;
}

static void *
sbrk(uintptr_t inc)
{
	char *new, *old = heap;

	if (old >= MAXADDR - inc)
		return ERRADDR;

	new = old + inc;
	if (_brk(new) < 0)
		return ERRADDR;
	heap = new;

	return old;
}

static Header *
morecore(size_t nunits)
{
	char *rawmem;
	Header *hp;

	if (nunits < NALLOC)
		nunits = NALLOC;

	rawmem = sbrk(nunits * sizeof(Header));
	if (rawmem == ERRADDR)
		return NULL;

	hp = (Header*)rawmem;
	hp->h.size = nunits;

	/* integrate new memory into the list */
	free(hp + 1);

	return freep;
}

/*
 * Run over the list of free blocks trying to find a block
 * big enough for nbytes. If the block fit perfectly with
 * the required size then we only have to unlink
 * the block. Otherwise we have to split the block and
 * return the right part. If we run over the full list
 * without a fit then we have to require more memory
 *
 *              ______________________________________
 * ___________./______________________________________\_____
 * ...| in   |   |     | in  |  |.....| in   |  |    | |....
 * ...| use  |   |     | use |  |.....| use  |  |    | |....
 * ___|______|___|.____|_____|._|_____|______|._|.___|.|____
 *            \__/ \_________/ \_____________/ \/ \__/
 */
void *
malloc(size_t nbytes)
{
	Header *cur, *prev;
	size_t nunits;

	/* 1 unit for header plus enough units to fit nbytes */
	nunits = (nbytes+sizeof(Header)-1) / sizeof(Header) + 1;

	for (prev = freep; ; prev = cur) {
		cur = prev->h.next;
		if (cur->h.size >= nunits) {
			if (cur->h.size == nunits) {
				prev->h.next = cur->h.next;
			} else {
				cur->h.size -= nunits;
				cur += cur->h.size;
				cur->h.size = nunits;
			}
			freep = prev;
			return cur + 1;
		}

		if (cur == freep) {
			if ((cur = morecore(nunits)) == NULL)
				return NULL;
		}
	}
}
