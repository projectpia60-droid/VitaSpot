#include "base64.h"
#include <string.h>

// ============================================================================
// Base64 Lookup Tables
// ============================================================================

static const char base64_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

// ============================================================================
// Base64 Implementation
// ============================================================================

int base64_encode(const unsigned char *src, size_t src_len,
                  char *dst, size_t dst_len) {
    if (!src || !dst) {
        return -1;
    }

    // Calcular tamaño requerido
    size_t required = ((src_len + 2) / 3) * 4;
    if (dst_len < required + 1) {
        return -1; // Buffer muy pequeño
    }

    size_t dst_pos = 0;
    size_t src_pos = 0;

    while (src_pos < src_len) {
        unsigned char b1 = src[src_pos++];
        unsigned char b2 = (src_pos < src_len) ? src[src_pos++] : 0;
        unsigned char b3 = (src_pos < src_len) ? src[src_pos++] : 0;

        unsigned int n = (b1 << 16) | (b2 << 8) | b3;

        dst[dst_pos++] = base64_table[(n >> 18) & 0x3F];
        dst[dst_pos++] = base64_table[(n >> 12) & 0x3F];

        if (src_pos - 1 < src_len) {
            dst[dst_pos++] = base64_table[(n >> 6) & 0x3F];
        } else {
            dst[dst_pos++] = '=';
        }

        if (src_pos < src_len) {
            dst[dst_pos++] = base64_table[n & 0x3F];
        } else {
            dst[dst_pos++] = '=';
        }
    }

    dst[dst_pos] = '\0';
    return (int)dst_pos;
}

static int base64_decode_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return 26 + (c - 'a');
    if (c >= '0' && c <= '9') return 52 + (c - '0');
    if (c == '+') return 62;
    if (c == '/') return 63;
    if (c == '=') return -1;
    return -2; // Carácter inválido
}

int base64_decode(const char *src, size_t src_len,
                  unsigned char *dst, size_t dst_len) {
    if (!src || !dst) {
        return -1;
    }

    size_t dst_pos = 0;
    size_t src_pos = 0;

    while (src_pos < src_len && src[src_pos] != '\0') {
        // Obtener 4 caracteres base64
        int c1 = base64_decode_char(src[src_pos++]);
        int c2 = (src_pos < src_len) ? base64_decode_char(src[src_pos++]) : -1;
        int c3 = (src_pos < src_len) ? base64_decode_char(src[src_pos++]) : -1;
        int c4 = (src_pos < src_len) ? base64_decode_char(src[src_pos++]) : -1;

        if (c1 < 0 || c2 < 0) {
            return -1; // Carácter inválido
        }

        // Decodificar
        unsigned int n = (c1 << 18) | (c2 << 12);
        if (c3 >= 0) n |= (c3 << 6);
        if (c4 >= 0) n |= c4;

        // Extraer bytes
        dst[dst_pos++] = (n >> 16) & 0xFF;
        if (dst_pos >= dst_len) return -1;

        if (c3 >= 0) {
            dst[dst_pos++] = (n >> 8) & 0xFF;
            if (dst_pos >= dst_len) return -1;
        } else {
            break;
        }

        if (c4 >= 0) {
            dst[dst_pos++] = n & 0xFF;
            if (dst_pos >= dst_len) return -1;
        } else {
            break;
        }
    }

    return (int)dst_pos;
}
