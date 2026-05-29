#include "sha256.h"
#include <string.h>

// ============================================================================
// SHA-256 Constants
// ============================================================================

static const uint32_t sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

// ============================================================================
// SHA-256 Helper Macros
// ============================================================================

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define SHR(x, n)  ((x) >> (n))

#define CH(x, y, z)    (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z)   (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

// ============================================================================
// SHA-256 Implementation
// ============================================================================

void sha256_init(SHA256_CTX *ctx) {
    if (!ctx) return;

    ctx->h[0] = 0x6a09e667;
    ctx->h[1] = 0xbb67ae85;
    ctx->h[2] = 0x3c6ef372;
    ctx->h[3] = 0xa54ff53a;
    ctx->h[4] = 0x510e527f;
    ctx->h[5] = 0x9b05688c;
    ctx->h[6] = 0x1f83d9ab;
    ctx->h[7] = 0x5be0cd19;

    ctx->len = 0;
    ctx->partial_len = 0;
}

static void sha256_process_block(SHA256_CTX *ctx, const uint8_t *data) {
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t t1, t2;
    int i;

    // Copiar bloque a palabras 32-bit (big-endian)
    for (i = 0; i < 16; i++) {
        w[i] = (data[i * 4] << 24) | (data[i * 4 + 1] << 16) |
               (data[i * 4 + 2] << 8) | data[i * 4 + 3];
    }

    // Expandir a 64 palabras
    for (i = 16; i < 64; i++) {
        w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
    }

    // Inicializar valores de trabajo
    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];
    f = ctx->h[5];
    g = ctx->h[6];
    h = ctx->h[7];

    // 64 rondas
    for (i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + sha256_k[i] + w[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    // Sumar con valores hash previos
    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
    ctx->h[5] += f;
    ctx->h[6] += g;
    ctx->h[7] += h;
}

void sha256_update(SHA256_CTX *ctx, const void *data, size_t len) {
    if (!ctx || !data) return;

    const uint8_t *bytes = (const uint8_t *)data;
    size_t i = 0;

    // Si tenemos datos parciales, completar el bloque
    if (ctx->partial_len > 0) {
        size_t to_copy = 64 - ctx->partial_len;
        if (to_copy > len) {
            to_copy = len;
        }
        memcpy(ctx->partial + ctx->partial_len, bytes, to_copy);
        ctx->partial_len += to_copy;
        bytes += to_copy;
        len -= to_copy;
        i += to_copy;

        if (ctx->partial_len == 64) {
            sha256_process_block(ctx, ctx->partial);
            ctx->partial_len = 0;
            ctx->len += 512;
        }
    }

    // Procesar bloques completos
    while (len >= 64) {
        sha256_process_block(ctx, bytes);
        bytes += 64;
        len -= 64;
        ctx->len += 512;
    }

    // Guardar datos restantes
    if (len > 0) {
        memcpy(ctx->partial + ctx->partial_len, bytes, len);
        ctx->partial_len += len;
    }
}

void sha256_final(SHA256_CTX *ctx, uint8_t *digest) {
    if (!ctx || !digest) return;

    uint8_t bits[8];
    uint64_t bit_len = ctx->len + (ctx->partial_len * 8);

    // Convertir longitud a bits big-endian
    for (int i = 0; i < 8; i++) {
        bits[7 - i] = (bit_len >> (i * 8)) & 0xFF;
    }

    // Añadir padding
    uint8_t padding = 0x80;
    sha256_update(ctx, &padding, 1);

    // Rellenar con ceros hasta que falten 8 bytes
    while (ctx->partial_len != 56) {
        uint8_t zero = 0;
        sha256_update(ctx, &zero, 1);
    }

    // Añadir longitud
    sha256_update(ctx, bits, 8);

    // Copiar hash final (big-endian)
    for (int i = 0; i < 8; i++) {
        uint32_t h = ctx->h[i];
        digest[i * 4] = (h >> 24) & 0xFF;
        digest[i * 4 + 1] = (h >> 16) & 0xFF;
        digest[i * 4 + 2] = (h >> 8) & 0xFF;
        digest[i * 4 + 3] = h & 0xFF;
    }
}

void sha256(const void *data, size_t len, uint8_t *digest) {
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, digest);
}
