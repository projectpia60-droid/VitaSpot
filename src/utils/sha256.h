#pragma once

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// SHA-256 Hashing
// ============================================================================

#define SHA256_DIGEST_SIZE 32

typedef struct {
    uint32_t h[8];
    uint64_t len;
    uint32_t w[64];
    size_t partial_len;
    uint8_t partial[64];
} SHA256_CTX;

/**
 * Inicializa contexto SHA-256
 * @param ctx Contexto a inicializar
 */
void sha256_init(SHA256_CTX *ctx);

/**
 * Actualiza hash con más datos
 * @param ctx Contexto SHA-256
 * @param data Datos a hashear
 * @param len Longitud de datos
 */
void sha256_update(SHA256_CTX *ctx, const void *data, size_t len);

/**
 * Finaliza hash y obtiene digest
 * @param ctx Contexto SHA-256
 * @param digest Buffer de salida (mínimo 32 bytes)
 */
void sha256_final(SHA256_CTX *ctx, uint8_t *digest);

/**
 * Calcula SHA-256 en una operación (convenience function)
 * @param data Datos a hashear
 * @param len Longitud de datos
 * @param digest Buffer de salida (mínimo 32 bytes)
 */
void sha256(const void *data, size_t len, uint8_t *digest);

#endif // SHA256_H
