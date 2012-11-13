#pragma once

#include <assert.h>

static inline ssize_t
xmsg_send(int sock, const std::string &s)
{
  assert(s.size() <= UINT_MAX);
  uint32_t l = htonl(s.size());
  write(sock, &l, 4);
  write(sock, s.data(), s.size());
  return s.size();
}

static inline ssize_t
xmsg_recv(int sock, std::string* s)
{
  union {
    char buf[4];
    uint32_t nl;
  } u;

  ssize_t n = 0;
  while (n < 4) {
    ssize_t cc = read(sock, &u.buf[n], 4-n);
    if (cc <= 0)
      return cc;
    n += cc;
  }

  uint32_t l = ntohl(u.nl);
  std::stringstream ss;
  while (ss.str().size() < l) {
    char buf[8192];
    ssize_t cc = read(sock, buf, sizeof(buf));
    if (cc <= 0)
      return cc;
    ss.write(buf, cc);
  }

  *s = ss.str();
  return s->size();
}
