#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <cassert>
#include <unistd.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* This code to undef const added in libiberty.  */
#ifndef __STDC__
/* This is a separate conditional since some stdc systems
   reject `defined (const)'.  */
#ifndef const
#define const
#endif
#endif

#include "OS4Helpers.h"

namespace std {

    int iswspace(wint_t c) {
#ifdef _MB_CAPABLE
          //c = _jp2uc (c);
        return ((c >= 0x0009 && c <= 0x000d) || c == 0x0020 ||
          c == 0x1680 || c == 0x180e ||
          (c >= 0x2000 && c <= 0x2006) ||
          (c >= 0x2008 && c <= 0x200a) ||
          c == 0x2028 || c == 0x2029 ||
          c == 0x205f || c == 0x3000);
#else
        return (c < 0x100 ? isspace (c) : 0);
#endif /* _MB_CAPABLE */
    }

    long int lround(double x) {
        return round(x);
    }
}

static int _putwc_cb(wchar_t wc, void *abstract) {
	_putwc_data_cb *data = static_cast<_putwc_data_cb*>(abstract);
	char buf[8];
	size_t len;

	len = wcrtomb(buf, wc, &data->mbs);
	if (len != (size_t)-1) {
		if (fwrite(buf, 1, len, data->fp) == len)
			return 0;
	}

	return -1;
}

int vfwprintf(FILE *fp, const wchar_t *fmt, va_list ap) {
	_putwc_data_cb cb_data;

	cb_data.fp = fp;
	memset(&cb_data.mbs, 0, sizeof(cb_data.mbs));

	return _libwprintf_dofmt(_putwc_cb, &cb_data, fmt, ap);
}


int swprintf(wchar_t *buffer, size_t maxlen, const wchar_t *fmt, ...) {
	va_list ap;
	int wc;

	va_start(ap, fmt);
	wc = vswprintf(buffer, maxlen, fmt, ap);
	va_end(ap);

	return wc;
}

int vswprintf(wchar_t *buffer, size_t maxlen, const wchar_t *fmt, va_list ap) {
	_putwc_data cb_data;
	int result;

	cb_data.buffer = buffer;
	cb_data.space  = maxlen;

	result = _libwprintf_dofmt(_putwc_cb, &cb_data, fmt, ap);

	/* Make sure that output is NUL terminated */
	_putwc_cb('\0', &cb_data);

	return result;
}

#define PUTWC(wc)                   \
	__extension__ ({                              \
		if (cb((wc), cb_data) == 0) \
			count++;                \
		else                        \
			return -1;              \
	})

static void reverse(char *str, size_t len) {
	if (len > 1) {
		char *start = str;
		char *end = str + len - 1;

		while (start < end) {
			char tmp = *end;
			*end-- = *start;
			*start++ = tmp;
		}
	}
}

static size_t itoa(unsigned num, char *dst, unsigned base, int issigned, int addplus, int uppercase) {
	int a = uppercase ? 'A' : 'a';
	int negative = 0;
	char *d = dst;
	size_t len;

	if (num == 0) {
		*d = '0';
		return 1;
	}

	if (issigned && (int)num < 0 && base == 10) {
		negative = 1;
		num = -num;
	}

	while (num > 0) {
		unsigned rem = num % base;
		num /= base;
		*d++ = (rem > 9) ? (rem - 10 + a) : (rem + '0');
	}

	if (negative)
		*d++ = '-';
	else if (addplus)
		*d++ = '+';

	len = d - dst;
	reverse(dst, len);

	return len;
}

static size_t lltoa(unsigned long long num, char *dst, unsigned base, int issigned, int addplus, int uppercase) {
	int a = uppercase ? 'A' : 'a';
	int negative = 0;
	char *d = dst;
	size_t len;

	if (num == 0) {
		*d = '0';
		return 1;
	}

	if (issigned && (signed long long)num < 0 && base == 10) {
		negative = 1;
		num = -num;
	}

	while (num > 0) {
		unsigned rem = num % base;
		num /= base;
		*d++ = (rem > 9) ? (rem - 10 + a) : (rem + '0');
	}

	if (negative)
		*d++ = '-';
	else if (addplus)
		*d++ = '+';

	len = d - dst;
	reverse(dst, len);

	return len;
}

int _libwprintf_dofmt(libwprintf_putwc_cb cb, void *cb_data, const wchar_t *fmt, va_list ap) {
	struct _reent *r = _REENT;
	wchar_t wc;
	const wchar_t *src;
	char tmp[128];
	const char *hsrc;
	int count = 0;

	while ((wc = *fmt++) != '\0') {
		if (wc == '%') {
			int left = 0;
			int addplus = 0;
			int alternate = 0;
			int padzeros = 0;
			size_t width = 0;
			size_t limit = -1;
			int longlong = 0;
			int half = 0;
			size_t len, i;
			int uppercase;
			void *ptr;
			const wchar_t *start = fmt;
			int prec;
			union {
				double d;
				uint32_t i[2];
			} fpval;
			int dsgn;
			int expt;
			size_t ndig;
			char *cp, *rve, *bp;

			if ((wc = *fmt++) == '\0')
				return count;

			for (;;) {
				if (wc == '-')
					left = 1;
				else if (wc == '+')
					addplus = 1;
				else if (wc == '#')
					alternate = 1;
				else if (wc == '0')
					padzeros = 1;
				else
					break;

				if ((wc = *fmt++) == '\0')
					return count;
			}

			while (wc >= '0' && wc <= '9') {
				width = 10 * width + (wc - '0');

				if ((wc = *fmt++) == '\0')
					return count;
			}

			if (wc == '.') {
				if ((wc = *fmt++) == '\0')
					return count;

				if (wc == '*') {
					limit = va_arg(ap, size_t);

					if ((wc = *fmt++) == '\0')
						return count;
				} else {
					limit = 0;
					while (wc >= '0' && wc <= '9') {
						limit = 10 * limit + (wc - '0');

						if ((wc = *fmt++) == '\0')
							return count;
					}
				}
			}

			if (wc == 'L') {
				longlong = 1;

				if ((wc = *fmt++) == '\0')
					return count;
			} else if (wc == 'l') {
				if ((wc = *fmt++) == '\0')
					return count;

				if (wc == 'l') {
					longlong = 1;

					if ((wc = *fmt++) == '\0')
						return count;
				}
			} else if (wc == 'h') {
				half = 1;

				if ((wc = *fmt++) == '\0')
					return count;
			}

			switch (wc) {
				default:
					fmt = start;
					/* Fall through */
				case '%':
					PUTWC('%');
					break;
				case 'C':
				case 'c':
					PUTWC(va_arg(ap, int));
					break;
				case 'D':
				case 'd':
					uppercase = (wc == 'D');
					if (longlong)
						len = lltoa(va_arg(ap, long long), tmp, 10, 1, addplus, uppercase);
					else
						len = itoa(va_arg(ap, int), tmp, 10, 1, addplus, uppercase);

					hsrc = tmp;

					if (width > len)
						width -= len;
					else
						width = 0;

					if (left == 0) {
						for (i = 0; i < width; i++) PUTWC(padzeros ? '0' : ' ');
					}

					for (i = 0; i < len; i++) PUTWC(hsrc[i]);

					if (left != 0) {
						for (i = 0; i < width; i++) PUTWC(' ');
					}
					break;
				case 'U':
				case 'u':
					uppercase = (wc == 'U');
					if (longlong)
						len = lltoa(va_arg(ap, unsigned long long), tmp, 10, 0, addplus, uppercase);
					else
						len = itoa(va_arg(ap, unsigned int), tmp, 10, 0, addplus, uppercase);

					hsrc = tmp;

					if (width > len)
						width -= len;
					else
						width = 0;

					if (left == 0) {
						for (i = 0; i < width; i++) PUTWC(padzeros ? '0' : ' ');
					}

					for (i = 0; i < len; i++) PUTWC(hsrc[i]);

					if (left != 0) {
						for (i = 0; i < width; i++) PUTWC(' ');
					}
					break;
				case 'X':
				case 'x':
					uppercase = (wc == 'X');
					if (longlong)
						len = lltoa(va_arg(ap, unsigned long long), tmp, 16, 0, addplus, uppercase);
					else
						len = itoa(va_arg(ap, unsigned int), tmp, 16, 0, addplus, uppercase);

					hsrc = tmp;

					if (width > len)
						width -= len;
					else
						width = 0;

					if (left == 0) {
						for (i = 0; i < width; i++) PUTWC(padzeros ? '0' : ' ');
					}

					for (i = 0; i < len; i++) PUTWC(hsrc[i]);

					if (left != 0) {
						for (i = 0; i < width; i++) PUTWC(' ');
					}
					break;
				case 'P':
				case 'p':
					uppercase = (wc == 'P');
					ptr = va_arg(ap, void *);
					len = itoa((unsigned)ptr, tmp, 16, 0, 0, uppercase);

					hsrc = tmp;
					width = 8;

					if (width > len)
						width -= len;
					else
						width = 0;

					if (alternate && ptr != NULL) {
						PUTWC('0');
						PUTWC('x');
					}

					for (i = 0; i < width; i++) PUTWC('0');

					for (i = 0; i < len; i++) PUTWC(hsrc[i]);
					break;
				case 'S':
				case 's':
					if (half) {
						hsrc = va_arg(ap, const char *);
						if (hsrc == NULL)
							hsrc = "(null)";

						len = strlen(hsrc);

						if (limit > 0 && len > limit)
							len = limit;

						if (width > len)
							width -= len;
						else
							width = 0;

						if (left == 0) {
							for (i = 0; i < width; i++) PUTWC(' ');
						}

						for (i = 0; i < len; i++) PUTWC(hsrc[i]);

						if (left != 0) {
							for (i = 0; i < width; i++) PUTWC(' ');
						}
					} else {
						src = va_arg(ap, const wchar_t *);
						if (src == NULL)
							src = L"(null)";

						len = wcslen(src);

						if (limit > 0 && len > limit)
							len = limit;

						if (width > len)
							width -= len;
						else
							width = 0;

						if (left == 0) {
							for (i = 0; i < width; i++) PUTWC(' ');
						}

						for (i = 0; i < len; i++) PUTWC(src[i]);

						if (left != 0) {
							for (i = 0; i < width; i++) PUTWC(' ');
						}
					}
					break;
				case 'F':
				case 'f':
					prec = limit;

					if (prec == -1) {
						prec = 6;
					}

					fpval.d = va_arg(ap, double);

					if (isinf(fpval.d)) {
						if (fpval.d < 0) PUTWC('-');

						if (wc == 'F') {
							PUTWC('I');
							PUTWC('N');
							PUTWC('F');
						} else {
							PUTWC('i');
							PUTWC('n');
							PUTWC('f');
						}

						break;
					}

					if (isnan(fpval.d)) {
						if (wc == 'F') {
							PUTWC('N');
							PUTWC('A');
							PUTWC('N');
						} else {
							PUTWC('n');
							PUTWC('a');
							PUTWC('n');
						}

						break;
					}

					if (fpval.i[0] & 0x80000000UL) {
						fpval.d = -fpval.d;
						PUTWC('-');
					}

					cp = _dtoa_r(r, fpval.d, 3, prec, &expt, &dsgn, &rve);

					bp = cp + prec;

					if (*cp == '0' && fpval.d)
						expt = -prec + 1;
					bp += expt;

					if (fpval.d == 0)
						rve = bp;

					while (rve < bp)
						*rve++ = '0';

					ndig = rve - cp;

					if (fpval.d == 0) {
						PUTWC('0');
						if ((size_t)expt < ndig || alternate) {
							PUTWC('.');
							for (i = 0; i < (ndig - 1); i++) PUTWC('0');
						}
					} else if (expt <= 0) {
						PUTWC('0');
						if (expt || ndig) {
							PUTWC('.');
							for (i = 0; i < -(size_t)expt; i++) PUTWC('0');
							for (i = 0; i < ndig; i++) PUTWC(cp[i]);
						}
					} else if ((size_t)expt >= ndig) {
						for (i = 0; i < ndig; i++) PUTWC(cp[i]);
						for (i = 0; i < ((size_t)expt - ndig); i++) PUTWC('0');
						if (alternate) PUTWC('.');
					} else {
						for (i = 0; i < (size_t)expt; i++) PUTWC(cp[i]);
						PUTWC('.');
						for (i = expt; i < ndig; i++) PUTWC(cp[i]);
					}

					break;
			}
		} else {
			PUTWC(wc);
		}
	}

	return count;
}



int fnmatch (const char *pattern, const char *string, int flags)
{
  register const char *p = pattern, *n = string;
  register unsigned char c;

#define FOLD(c)	((flags & FNM_CASEFOLD) ? tolower (c) : (c))

  while ((c = *p++) != '\0')
    {
      c = FOLD (c);

      switch (c)
	{
	case '?':
	  if (*n == '\0')
	    return FNM_NOMATCH;
	  else if ((flags & FNM_FILE_NAME) && *n == '/')
	    return FNM_NOMATCH;
	  else if ((flags & FNM_PERIOD) && *n == '.' &&
		   (n == string || ((flags & FNM_FILE_NAME) && n[-1] == '/')))
	    return FNM_NOMATCH;
	  break;

	case '\\':
	  if (!(flags & FNM_NOESCAPE))
	    {
	      c = *p++;
	      c = FOLD (c);
	    }
	  if (FOLD ((unsigned char)*n) != c)
	    return FNM_NOMATCH;
	  break;

	case '*':
	  if ((flags & FNM_PERIOD) && *n == '.' &&
	      (n == string || ((flags & FNM_FILE_NAME) && n[-1] == '/')))
	    return FNM_NOMATCH;

	  for (c = *p++; c == '?' || c == '*'; c = *p++, ++n)
	    if (((flags & FNM_FILE_NAME) && *n == '/') ||
		(c == '?' && *n == '\0'))
	      return FNM_NOMATCH;

	  if (c == '\0')
	    return 0;

	  {
	    unsigned char c1 = (!(flags & FNM_NOESCAPE) && c == '\\') ? *p : c;
	    c1 = FOLD (c1);
	    for (--p; *n != '\0'; ++n)
	      if ((c == '[' || FOLD ((unsigned char)*n) == c1) &&
		  fnmatch (p, n, flags & ~FNM_PERIOD) == 0)
		return 0;
	    return FNM_NOMATCH;
	  }

	case '[':
	  {
	    /* Nonzero if the sense of the character class is inverted.  */
	    register int negate;

	    if (*n == '\0')
	      return FNM_NOMATCH;

	    if ((flags & FNM_PERIOD) && *n == '.' &&
		(n == string || ((flags & FNM_FILE_NAME) && n[-1] == '/')))
	      return FNM_NOMATCH;

	    negate = (*p == '!' || *p == '^');
	    if (negate)
	      ++p;

	    c = *p++;
	    for (;;)
	      {
		register unsigned char cstart = c, cend = c;

		if (!(flags & FNM_NOESCAPE) && c == '\\')
		  cstart = cend = *p++;

		cstart = cend = FOLD (cstart);

		if (c == '\0')
		  /* [ (unterminated) loses.  */
		  return FNM_NOMATCH;

		c = *p++;
		c = FOLD (c);

		if ((flags & FNM_FILE_NAME) && c == '/')
		  /* [/] can never match.  */
		  return FNM_NOMATCH;

		if (c == '-' && *p != ']')
		  {
		    cend = *p++;
		    if (!(flags & FNM_NOESCAPE) && cend == '\\')
		      cend = *p++;
		    if (cend == '\0')
		      return FNM_NOMATCH;
		    cend = FOLD (cend);

		    c = *p++;
		  }

		if (FOLD ((unsigned char)*n) >= cstart
		    && FOLD ((unsigned char)*n) <= cend)
		  goto matched;

		if (c == ']')
		  break;
	      }
	    if (!negate)
	      return FNM_NOMATCH;
	    break;

	  matched:;
	    /* Skip the rest of the [...] that already matched.  */
	    while (c != ']')
	      {
		if (c == '\0')
		  /* [... (unterminated) loses.  */
		  return FNM_NOMATCH;

		c = *p++;
		if (!(flags & FNM_NOESCAPE) && c == '\\')
		  /* XXX 1003.2d11 is unclear if this is right.  */
		  ++p;
	      }
	    if (negate)
	      return FNM_NOMATCH;
	  }
	  break;

	default:
	  if (c != FOLD ((unsigned char)*n))
	    return FNM_NOMATCH;
	}

      ++n;
    }

  if (*n == '\0')
    return 0;

  if ((flags & FNM_LEADING_DIR) && *n == '/')
    /* The FNM_LEADING_DIR flag says that "foo*" matches "foobar/frobozz".  */
    return 0;

  return FNM_NOMATCH;
}

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	UNUSED(prot);
	UNUSED(flags);
	if (addr != nullptr) {
		if ((addr = malloc(len))) {
			lseek(fd, offset, SEEK_SET);
			read(fd, addr, len);
		}

		if (!addr) {
                	return nullptr;
        	}

		return addr;
	}
	else {
		void *data = malloc(len);
		if (data != nullptr) {
                        lseek(fd, offset, SEEK_SET);
                        read(fd, data, len);
		}

		if (!data) {
			return nullptr;
		}

		return data;
	}
}

int munmap(void *map, size_t length)
{
	UNUSED(length);
	if (map!=NULL)
		free(map);

	return 0;
}
