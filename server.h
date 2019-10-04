#ifndef __STUDENT_CODE_H__
#define __STUDENT_CODE_H__

#include <stdint.h>

int get_sum_of_ints_udp(int sockfd, uint32_t *tab, size_t length, uint32_t *rep);

int server_udp(int sockfd);

#endif // __STUDENT_CODE_H__

