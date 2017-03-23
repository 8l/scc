/* See LICENSE file for copyright and license details. */

#include <stdarg.h>
#include <stdio.h>
#undef vfprintf

static int
printn2(FILE * restrict fp, unsigned n, int  base)
{
	unsigned t;
	int cnt = 0;
	static char digits[] = "0123456789ABCDEF";

	if ((t = n / base) != 0)
		cnt += printn2(fp, t, base);
	putc(digits[n % base], fp);
	return cnt + 1;
}

static int
printn(FILE * restrict fp, int n, int b, int sign)
{
	int cnt = 0;

	if (sign && n < 0) {
		n = -n;
		putc('-', fp);
		++cnt;
	}
	cnt += printn2(fp, n, b);
	return cnt;
}

int
vfprintf(FILE * restrict fp, const char *fmt, va_list va)
{
	int c, base, sign, cnt;
	char *s;

	while (( c = *fmt++) != '\0') {
		if (c == '%') {
			sign = 0;
			switch (*fmt++) {
			case '%':
				c = '%';
				break;
			case 'c':
				c = va_arg(va, int);
				break;
			case 'o':
				base = 8;
				goto numeric;
			case 'd':
				sign = 1;
				base = 10;
				goto numeric;
			case 'x':
				base = 16;
			numeric:
				c = va_arg(va, int);
				cnt += printn(fp, c, base, sign);
				continue;
			case 's':
				s = va_arg(va, char *);
				while ((c = *s++) != '\0')
					putc(c, fp);
				/* passthrou */
			default:
				continue;
			}
		}
		putc(c, fp);
		++cnt;
	}
	return cnt;
}
