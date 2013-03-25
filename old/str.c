/*
 * Copyright (c) 1997-2003 Christopher Maxwell.  All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toolbox.h"

/*
 * tbstring.s	-> pointer to character array
 * tbstring.a	-> size of the buffer allocated
 * tbstring.len	-> length of the string
 */

/* C-string functions */

/*
 * Tacks *s onto *sa.
 */
int
tbstrcat(tbstring *sa, const char *s)
{
	int ret;
	size_t l, n;

	if (!sa)
		return TBOX_API_INVALID;

	/* null string */
	if (!s)
		return tbstrcpy(sa, s);

	/* empty sa, so just copying */
	if (!sa->s)
		return tbstrcpy(sa, s);

	/* grab the length of the string (no null) */
	l = strlen(s);

	/* ask for memory, old + new + null */
	if ((ret = tbstrchk(sa, (sa->len + l + 1))))
		return ret;

	/* cat the string, giving the size of our abuf */
	n = strlcat(sa->s, s, sa->a);

	/* truncated string */
	if (n > sa->a)
		return TBOX_TRUNCATED;

	/* increment length by what we add */
	sa->len += l;

	return TBOX_SUCCESS;
}

/*
 * Copies s over sa.   ex.   s = cow  sa = thepasture   cow(\0)asture
 */
int
tbstrcpy(tbstring *sa, const char *s)
{
	int ret;
	size_t l, n;

	if (!sa)
		return TBOX_API_INVALID;
	if (!s)
		return tbstrchk(sa, 1);

	/* grab the length of the string (no null) */
	l = strlen(s);

	/* ask for memory (new + null) */
	if ((ret = tbstrchk(sa, (l + 1))))
		return ret;

	/* copy the string, giving the size of our abuf */
	n = strlcpy(sa->s, s, sa->a);

	/* truncated string */
	if (n > sa->a)
		return TBOX_TRUNCATED;

	/* set to length of new string */
	sa->len = l;

	return TBOX_SUCCESS;
}

/*
 * the s != plural
 * tacks string s onto string sa.
 */
int
tbstrcats(tbstring *sa, tbstring *s)
{
	int ret;

	if (!sa || !s)
		return TBOX_API_INVALID;

	/* empty sa, so just copying */
	if (!sa->s)
		return tbstrcpy(sa, (char *)s->s);

	/* ask for memory (old + new + null) */
	if ((ret = tbstrchk(sa, (sa->len + s->len + 1))))
		return ret;

	/* cat the string, giving the size of our abuf */
	memcpy(&sa->s[sa->len], s->s, s->len);

	/* increment by length of new string */
	sa->len += s->len;

	sa->s[sa->len] = 0;

	return TBOX_SUCCESS;
}

/*
 * Same as stracpy, only with strings not char.
 */
int
tbstrcpys(tbstring *sa, tbstring *s)
{
	int ret;

	if (!sa || !s)
		return TBOX_API_INVALID;

	/* ask for memory (new + null) */
	if ((ret = tbstrchk(sa, (s->len + 1))))
		return ret;

	/* copy the string, giving the size of our abuf */
	memcpy(sa->s, s->s, sa->a);

	/* stop string at the length */
	sa->len = s->len;

	return TBOX_SUCCESS;
}

/*
 * Quickly build a new character string with SAFE memory allocation and return
 * a pointer.  Onus on safe. 
 *
 * ptr = stranow("hello");
 */
char
*strnow(const char *s)
{
	char *p;
	size_t i;

	if (!s)	return NULL;

	i = strlen(s);

	p = (char *)calloc(1, i+1);
	if (!p)
		return NULL;

	if (!strlcpy(p, s, (i+1))) {
		if (p)
			free(p);
		return NULL;
	}

	return p;
}

/*
 * Prefix a string sa with string s
 * sa=thepasture s=cow  sa==>cowthepasture
 */
int
tbstrpre(tbstring *sa, const char *s)
{
	tbstring tstr = {0};
	int ret;

	if (!sa)
		return TBOX_API_INVALID;
	if (!s)
		return TBOX_SUCCESS;

	/* start our tempstring with prefix */
	if ((ret = tbstrcat(&tstr, s)))
		return ret;

	/* cat our old string to tempstring */
	if ((ret = tbstrcats(&tstr, sa))) {
		tbstrfree(&tstr);
		return ret;
	}

	/* copy temp string over string */
	if ((ret = tbstrcpys(sa, &tstr))) {
		tbstrfree(&tstr);
		return ret;
	}

	tbstrfree(&tstr);

	return TBOX_SUCCESS;
}

int
tbstrinsc(tbstring *sa, int c, size_t pos)
{
	int ret;

	if (!sa)
		return TBOX_API_INVALID;

	/* ask for another byte of memory + NUL */
	if ((ret = tbstrchk(sa, (sa->len + 1 + 1))))
		return ret;

	/* oversize means append */
	if (pos > sa->len)
		pos = sa->len;

	/* move the afterpart */
	if (pos < sa->len)
		memmove(sa->s + pos + 1, sa->s + pos, sa->len - pos);

	/* increase the string len after movement */
	sa->len++;

	*(sa->s + pos) = c;
	*(sa->s + sa->len) = '\0';

	return TBOX_SUCCESS;
}

int
tbstrdelc(tbstring *sa, size_t pos)
{
	int ret;

	if (!sa)
		return TBOX_API_INVALID;

	if (sa->len == 0)
		return TBOX_TRUNCATED;
	if (pos > sa->len)
		pos = sa->len;

	if (pos < sa->len)
		memmove(sa->s + pos, sa->s + pos + 1, sa->len - pos);

	sa->len--;
	*(sa->s + sa->len) = '\0';

	if ((ret = tbstrchk(sa, (sa->len + 1))))
		return ret;

	return TBOX_SUCCESS;
}

int
tbstrgetc(tbstring *sa)
{
	int c;

	if (!sa || !sa->s)
		return EOF;

	c = *sa->s;

	if (tbstrdelc(sa, 0))
		return EOF;

	return c;
}

/* safe for both: Cstring and byte string */
/*
 * Frees allocated memory
 */
void
tbstrfree(tbstring *sa)
{
	if (!sa)
		return;

	if (!sa->s)
		return;

	free(sa->s);

	sa->len = 0;
	sa->s = NULL;
	sa->a = 0;
}

/*
 * Allocates memory for strings.
 *
 * sa->a	is our old length
 * size		is our requested length (should include null char)
 */
int
tbstrchk(tbstring *sa, size_t size)
{
	char *ptr2;

	if (!sa)
		return TBOX_API_INVALID;

	if (size == 0)
		return TBOX_SUCCESS;

	/* first time, no pointer */
	if (!sa->s) {
		sa->s = (char *)calloc(1, size);

		/* throw back error */
		if (!sa->s)
			return TBOX_NOMEM;

		sa->a = size;
		return TBOX_SUCCESS;
	}

	/* check if we need space */
	if (size > sa->a) {
		/* re-allocate the buffer */
		if ((ptr2 = (char *)realloc(sa->s, size)) == NULL) {
			if (sa->s) free(sa->s);
			sa->s = NULL;
			return TBOX_NOMEM;
		}
		sa->s = ptr2;
		sa->a = size;
	}

	return TBOX_SUCCESS;
}

/*
 * Finds first occurance of char c, starting at start, in string sa.
 * like say for searching for =  in   2 x 2 = your mom.
 */
size_t
tbstrpos(tbstring *sa, int c, size_t start)
{
	size_t i;

	if (!sa) return 0;

	if (start > sa->len)
		return 0;

	for (i = start; i < sa->len; ++i)
		if (sa->s[i] == c)
			return i;

	return 0;
}

/* byte only: */
/*
 * Here we keep a NULL on the end, just for safety. Because you might forget
 * that its a byte string, not a cstring (nub).
 */
int
tbstrbcat(tbstring *sa, const char *s, size_t len)
{
	int ret;

	if (!sa)
		return TBOX_API_INVALID;
	if (!s)
		return TBOX_SUCCESS;

	/* empty sa, so just copying */
	if (!sa->s)
		return tbstrbcpy(sa, s, len);

	/* ask for memory (old + new + hidden null)*/
	if ((ret = tbstrchk(sa, (sa->len + len + 1))))
		return ret;

	/* cat the string, starting at hidden null */
	memcpy((sa->s + sa->len), s, len);

	/* increment length by what we add */
	sa->len += len;

	/* add our safety null (the 0/1 shift gives correct offset) */
	sa->s[sa->len] = '\0'; 

	return TBOX_SUCCESS;
}

/*
 * Same as bcat but copys.
 */
int
tbstrbcpy(tbstring *sa, const char *s, size_t len)
{
	int ret;

	if (!sa)
		return TBOX_API_INVALID;
	if (!s)
		return TBOX_SUCCESS;

	/* ask for memory (new + hidden null) */
	if ((ret = tbstrchk(sa, (len + 1))))
		return ret;

	/* copy the string, starting at 0 */
	memcpy(sa->s, s, len);

	/* set to length of new string */
	sa->len = len;

	/* add our hidden null */
	sa->s[sa->len] = '\0';

	return TBOX_SUCCESS;
}

/*
 * NULLs the string.
 */
int
tbstrbzero(tbstring *sa)
{
	char ch = '\0';
	
	if (!sa)
		return TBOX_API_INVALID;

	return tbstrbcat(sa, &ch, 1);
}

/* Complementary Functions */
/*
 * Your kernel is looking so nice today.
 * Same as strapos but on cstrings rather than safestrings.
 */
size_t
strpos(const char *s, char c)
{
	size_t i;

	if (!s)	return 0;

	for (i = 0; i < strlen(s); ++i)
		if (s[i] == c)
			return i;
	return 0;
}

/*
 * Looks for old in s and replaces with new but in sa. With safe 
 * memory allocation.
 * sa = null  s thecowisinthepasture  old cow new house
 * then sa = thehouseisinthepasture
 */
int
tbstrrep(tbstring *sa, char *s, char *old, char *new)
{
	char *ptr;
	int ret;

	/* check pointers */
	if (!sa || !s || !old || !new)
		return TBOX_API_INVALID;

	if (strlen(s) == 0)
		return TBOX_API_INVALID;
	if (strlen(old) == 0)
		return TBOX_API_INVALID;
	/* new is allowed to be 0-length */

	/* search for old */
	ptr = strstr(s, old);
	if (ptr == NULL) {
		/* not found */
		return TBOX_NOTFOUND;
	}

	/* check that ptr is within s*/
	if ((size_t)(ptr - s) > strlen(s))
		return TBOX_RANGE;

	/* copy the prefix */
	if ((ret = tbstrbcpy(sa, s, (ptr - s))))
		return ret;

	/* copy the new text */
	if ((ret = tbstrcat(sa, new)))
		return ret;

	/* copy the end */
	if ((ret = tbstrcat(sa, (ptr + strlen(old)))))
		return ret;

	return TBOX_SUCCESS;
}

/*
 * Split a string into "left" and "right" parts based on first position of
 * character c in string s.
 * If seperator is not found, loads entire string into (left)
 */
int
tbstrsplit2(const char *s, int c, tbstring *left, tbstring *right)
{
	int ret;
	size_t pos;

	if (!s || !left || !right)
		return TBOX_API_INVALID;

	pos = strpos(s, c);
	if (pos <= 0) {
		/* not found, load everything into (left) */
		if ((ret = tbstrcpy(left, s)))
			return ret;
		if ((tbstrbzero(right)))
			return ret;
		return TBOX_SUCCESS;
	}

	/* copy the left portion, adding the NULL */
	if ((ret = tbstrbcpy(left, s, pos)))
		return ret;
	if ((ret = tbstrbcat(left, "\0", 1)))
		return ret;

	/* pos is the EXACT placement in the string of the (c) */
	if ((ret = tbstrbcpy(right, (s + pos + 1), (strlen(s) - pos - 1))))
		return ret;
	if ((ret = tbstrbcat(right, "\0", 1)))
		return ret;

	return TBOX_SUCCESS;
}

/* (TODO)
 * special func for comparing hostnames
 *
 * returns 0 if success
 * returns 1 if really not alike
 * returns 2 if **could** be a match (host vs. fqdn compared ok)
 */
int
strcmp_host(const char *s1, const char *s2)
{
	int ret;

	ret = strcasecmp(s1, s2);
	if (ret == 0)
		return 0;

	/* first host/fqdn */
	ret = strncasecmp(s1, s2, strpos(s1, '.'));
	if (ret == 0)
		return 2;

	/* second fqdn/host */
	ret = strncasecmp(s1, s2, strpos(s2, '.'));
	if (ret == 0)
		return 2;

	return 1;
}

int
tbstrclean(const char *s, const char *charset, tbstring *new)
{
	ssize_t pos = 0;
	size_t abspos = 0;
	size_t len = strlen(s);
	int ret;
	const char *ptr;

	if (!s || !charset || !new)
		return TBOX_API_INVALID;

	/* increment pos past the character we stopped on */
	for (;abspos < len; abspos++) {

		/* increment absolute */
		abspos += pos;

		/* set ptr to last position */
		ptr = &s[abspos];

		/* see how many characters we can copy */
		pos = strcspn(s, charset);

		if (pos <= 0) {
			/* two chars in a row, just keep looping */
			if (abspos < len)
				continue;

			/* we are done */
			if ((ret = tbstrbcat(new, "\0", 1)))
				return ret;
			return TBOX_SUCCESS;
		}

		/* copy the left portion, adding the NULL */
		if ((ret = tbstrbcpy(new, ptr, pos)))
			return ret;
	}

	/* another form of done */
	if ((ret = tbstrbcat(new, "\0", 1)))
		return ret;

	return TBOX_SUCCESS;
}

/* Integer functions */
/*
 * n is the minimum number of chars to use
 */
int
tbstrcatul_p(tbstring *sa, unsigned long ul, size_t n)
{
	size_t len;
	unsigned long q;
	char *s;
	int ret;

	len = 1;
	q = ul;
	while (q > 9) {
		++len;
		q /= 10;
	}

	if (len < n)
		len = n;

	/* +NULL */
	if ((ret = tbstrchk(sa, sa->len + len + 1)))
		return ret;
	s = sa->s + sa->len;
	sa->len += len;

	/* this works backwards */
	while (len) {
		s[--len] = '0' + (ul % 10);
		ul /= 10;
	}

	return TBOX_SUCCESS;
}

/*
 * n is the minimum number of chars to use
 */
int
tbstrcatl_p(tbstring *sa, long l, size_t n)
{
	int ret;

	if (l < 0) {
		if ((ret = tbstrcat(sa, "-")))
			return ret;
		l = -l;
	}
	return tbstrcatul_p(sa, l, n);
}

int
tbstrprintf(tbstring *sa, const char *fmt, ...)
{
	char *s;
	ssize_t l;
	int ret;
	va_list ap;

	va_start(ap, fmt);
	l = vasprintf(&s, fmt, ap);
	va_end(ap);

	if (l == -1)
		return TBOX_TRUNCATED;

	if ((ret = tbstrcpy(sa, s))) {
		free(s);
		return ret;
	}

	free(s);

	return TBOX_SUCCESS;
}

int
tbstrprintfcat(tbstring *sa, const char *fmt, ...)
{
	char *s;
	ssize_t l;
	int ret;
	va_list ap;

	va_start(ap, fmt);
	l = vasprintf(&s, fmt, ap);
	va_end(ap);

	if (l == -1)
		return TBOX_TRUNCATED;

	if ((ret = tbstrcat(sa, s))) {
		free(s);
		return ret;
	}

	free(s);

	return TBOX_SUCCESS;
}
