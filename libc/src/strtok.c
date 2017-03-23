/* See LICENSE file for copyright and license details. */

#include <string.h>
#undef strtok

char *
strtok(char * restrict s, const char * restrict delim)
{
	static char *line;

	if (s)
		line = s;
	if (!s && !line)
		return NULL;

	s = line + strspn(line, delim);
	if (*s == '\0')
		return line = NULL;

	line = s + strcspn(s, delim);
	if (*line != '\0')
		*line++ = '\0';
	else
		line = NULL;

	return s;
}
