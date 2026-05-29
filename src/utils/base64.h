#pragma once

#include <stddef.h>

// ============================================================================
// Base64 Encoding/Decoding
// ============================================================================

/**
 * Codifica datos a base64
 * @param src Datos fuente
 * @param src_len Longitud de src
 * @param dst Buffer destino
 * @param dst_len Tamaño del buffer destino
 * @return Longitud de datos codificados, <0 si error
 */
int base64_encode(const unsigned char *src, size_t src_len,
                  char *dst, size_t dst_len);

/**
 * Decodifica datos desde base64
 * @param src String base64
 * @param src_len Longitud de src
 * @param dst Buffer destino
 * @param dst_len Tamaño del buffer destino
 * @return Longitud de datos decodificados, <0 si error
 */
int base64_decode(const char *src, size_t src_len,
                  unsigned char *dst, size_t dst_len);

#endif // BASE64_H
