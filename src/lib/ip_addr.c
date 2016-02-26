#include "gallus_apis.h"
#include "gallus_ip_addr.h"

#ifndef EAI_ADDRFAMILY
#define EAI_ADDRFAMILY EAI_FAMILY
#endif /* EAI_ADDRFAMILY */





struct ip_address {
  char addr_str[GALLUS_ADDR_STR_MAX];
  bool is_ipv4;
  struct sockaddr_storage saddr;
};

gallus_result_t
gallus_ip_address_create(const char *name, bool is_ipv4_addr,
                          gallus_ip_address_t **ip) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  struct addrinfo hints;
  struct addrinfo *addr = NULL;
  size_t len = 0;
  int rc;
  int primary;
  int secondary;

  if (IS_VALID_STRING(name) == true && ip != NULL) {
    *ip = (gallus_ip_address_t *) malloc(sizeof(gallus_ip_address_t));
    if (*ip != NULL) {
      len = strlen(name);
      if (len < GALLUS_ADDR_STR_MAX) {
        memset(*ip, 0, sizeof(gallus_ip_address_t));
        strncpy((*ip)->addr_str, name, len);
        (*ip)->addr_str[len] = '\0';

        if (is_ipv4_addr == false) {
          primary = AF_INET6;
          secondary = AF_INET;
        } else {
          primary = AF_INET;
          secondary = AF_INET6;
        }
	memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = 0;
        hints.ai_family = primary;
        hints.ai_flags = AI_PASSIVE;
        hints.ai_protocol = 0;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        rc = getaddrinfo((*ip)->addr_str, NULL, &hints, &addr);
        if (rc == EAI_ADDRFAMILY) {
          hints.ai_family = secondary;
          rc = getaddrinfo((*ip)->addr_str, NULL, &hints, &addr);
        }

        if (rc == 0) {
          rc = getnameinfo(addr->ai_addr, addr->ai_addrlen,
                           (*ip)->addr_str, GALLUS_ADDR_STR_MAX,
                           NULL, 0, NI_NOFQDN | NI_NUMERICHOST);
        }

        if (rc == 0) {
          if (addr->ai_family == AF_INET) {
            (*ip)->is_ipv4 = true;
          } else {
            (*ip)->is_ipv4 = false;
          }

          /*
           *  The ai_addrlen, which is a length of sockaddr,
           *  is shorter than a length of sockaddr_storage.
           */
          memcpy(&((*ip)->saddr), addr->ai_addr, addr->ai_addrlen);

          freeaddrinfo((void *) addr);
          return GALLUS_RESULT_OK;
        } else if (rc == EAI_MEMORY) {
          ret = GALLUS_RESULT_NO_MEMORY;
        } else if (rc == EAI_SYSTEM) {
          ret = GALLUS_RESULT_POSIX_API_ERROR;
        } else {
          ret = GALLUS_RESULT_ADDR_RESOLVER_FAILURE;
        }
      } else {
        ret = GALLUS_RESULT_OUT_OF_RANGE;
      }
    } else {
      ret = GALLUS_RESULT_NO_MEMORY;
    }

    if (addr != NULL) {
      freeaddrinfo((void *) addr);
    }
    free((void *) *ip);
    *ip = NULL;
    return ret;
  }

  return GALLUS_RESULT_INVALID_ARGS;
}

void
gallus_ip_address_destroy(gallus_ip_address_t *ip) {
  free((void *) ip);
}

gallus_result_t
gallus_ip_address_copy(const gallus_ip_address_t *src,
                        gallus_ip_address_t **dst) {
  if (src != NULL && dst != NULL) {
    if (*dst == NULL) {
      *dst = (gallus_ip_address_t *) malloc(sizeof(gallus_ip_address_t));
    }
    if (*dst != NULL) {
      memcpy(*dst, src, sizeof(gallus_ip_address_t));
      return GALLUS_RESULT_OK;
    } else {
      return GALLUS_RESULT_NO_MEMORY;
    }
  }
  return GALLUS_RESULT_INVALID_ARGS;
}

bool
gallus_ip_address_equals(const gallus_ip_address_t *ip1,
                          const gallus_ip_address_t *ip2) {
  struct sockaddr *sa1 = NULL;
  struct sockaddr *sa2 = NULL;

  if (ip1 != NULL && ip2 != NULL) {
    if (ip1->is_ipv4 != ip2->is_ipv4 ||
        ip1->saddr.ss_family != ip2->saddr.ss_family) {
      return false;
    }

    sa1 = (struct sockaddr *) &(ip1->saddr);
    sa2 = (struct sockaddr *) &(ip2->saddr);
    if (ip1->saddr.ss_family == AF_INET) {
      if (memcmp(sa1, sa2, sizeof(struct sockaddr)) == 0 &&
          memcmp(&((struct sockaddr_in *) sa1)->sin_addr,
                 &((struct sockaddr_in *) sa2)->sin_addr,
                 sizeof(struct in_addr)) == 0) {
        return true;
      }
    } else if (ip1->saddr.ss_family == AF_INET6) {
      if (memcmp(sa1, sa2, sizeof(struct sockaddr)) == 0 &&
          memcmp(&((struct sockaddr_in6 *) sa1)->sin6_addr,
                 &((struct sockaddr_in6 *) sa2)->sin6_addr,
                 sizeof(struct in6_addr)) == 0) {
        return true;
      }
    } else {
      return false;
    }
  }
  return false;
}

gallus_result_t
gallus_ip_address_str_get(const gallus_ip_address_t *ip,
                           char **addr_str) {
  if (ip != NULL && addr_str != NULL) {
    *addr_str = strndup(ip->addr_str, GALLUS_ADDR_STR_MAX + 1);
    if (IS_VALID_STRING(*addr_str) == true) {
      return GALLUS_RESULT_OK;
    } else {
      *addr_str = NULL;
      return GALLUS_RESULT_NO_MEMORY;
    }
  }
  return GALLUS_RESULT_INVALID_ARGS;
}

gallus_result_t
gallus_ip_address_sockaddr_get(const gallus_ip_address_t *ip,
                                struct sockaddr **saddr) {
  if (ip != NULL && saddr != NULL) {
    if (*saddr == NULL) {
      /* For avoiding build-scan warnings. */
      struct sockaddr_storage *ss = malloc(sizeof(struct sockaddr_storage));
      *saddr = (struct sockaddr *) ss;
    }
    if (*saddr != NULL) {
      memcpy(*saddr, &(ip->saddr), sizeof(struct sockaddr_storage));
      return GALLUS_RESULT_OK;
    } else {
      return GALLUS_RESULT_NO_MEMORY;
    }
  }
  return GALLUS_RESULT_INVALID_ARGS;
}
