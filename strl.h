/*
 * Copyright 2010 Nathan Phillip Brink <ohnobinki@ohnopublishing.net>
 *
 * This file is part of libstrl.
 *
 * libstrl is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * libstrl is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libstrl.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <stddef.h>
#include <string.h>

size_t strnlen(const char *s, size_t maxlen)
{
  size_t len;

  for (len = 0; len < maxlen && s[len] != '\0'; len ++)
    ;

  return len;
}

/* changes to this function should likely cause parallel changes in wcslcpy() */
size_t strlcpy(char *dst, const char *src, size_t len)
{
  size_t src_len;
  size_t copy_len;

  /* if len is zero, then len - 1 gets us into trouble. */
  if (!len)
    return strlen(src);

  src_len = strlen(src);

  copy_len = src_len;
  if (copy_len >= len)
    copy_len = len - 1;

  memcpy(dst, src, copy_len);
  dst[copy_len] = '\0';

  return src_len;
}

/* most changes to this function require parallel changes to wcslcat()! */
size_t strlcat(char *dst, const char *src, size_t len)
{
  size_t dst_len;
  size_t src_len;
  size_t copy_len;

  src_len = 0;

  /* len - 1 can't be -1 */
  if (!len)
    return strlen(src);

  /* len instead of len - 1 because of the special case below  */
  dst_len = strnlen(dst, len);

  /* src_len is the number of bytes we want to copy out */
  src_len = strlen(src);

  /*
   * Special case: if dst_len == len, don't touch dst; assume that
   * strlen(dst) would be len. Also, we're not allowed to replace the
   * last byte of dst with a '\0' in this case.
   */
  if (dst_len == len)
    return dst_len + src_len;

  /* maximum number of bytes we're willing to copy */
  copy_len = len - dst_len - 1;
  if (copy_len)
    {
      /* copy_len is the number of bytes to actually copy */
      if (copy_len > src_len)
	copy_len = src_len;

      memcpy(&dst[dst_len], src, copy_len);
    }
  dst[dst_len + copy_len] = '\0';

  return dst_len + src_len;
}
