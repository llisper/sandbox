#ifndef SHM_CRC32_H
#define SHM_CRC32_H

#include <stdlib.h>

/*
 * calculate the crc32 checksum for bytes_count of bytes
 *
 * check this to learn crc concept and algorithm:
 * http://www.repairfaq.org/filipg/LINK/F_crc_v31.html
 * and finally, this implementation refers crc32 implementation from zlib
 */
unsigned int shm_crc32(char* bytes, size_t bytes_count);

#endif // SHM_CRC32_H

