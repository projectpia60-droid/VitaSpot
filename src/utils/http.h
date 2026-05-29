#pragma once

#include <stddef.h>

// ============================================================================
// HTTP Utilities — Wrappers de libcurl para PSVita
// ============================================================================

/**
 * Realiza un GET HTTP/HTTPS
 * @param url URL completa (ej: "https://api.spotify.com/v1/me")
 * @param extra_headers Headers adicionales separados por \r\n (puede ser NULL)
 * @return Puntero a string con la respuesta (heap-allocated), NULL si error
 *         El llamador es responsable de liberar la respuesta con free()
 */
char *http_get(const char *url, const char *extra_headers);

/**
 * Realiza un POST HTTP/HTTPS
 * @param url URL completa
 * @param body Cuerpo del POST
 * @param content_type Content-Type header (ej: "application/json")
 * @param extra_headers Headers adicionales (puede ser NULL)
 * @return Puntero a string con la respuesta, NULL si error
 */
char *http_post(const char *url, const char *body, const char *content_type,
                 const char *extra_headers);

/**
 * Realiza un PUT HTTP/HTTPS
 * @param url URL completa
 * @param body Cuerpo del PUT
 * @param extra_headers Headers adicionales (puede ser NULL)
 * @return 0 si OK, <0 si error
 */
int http_put(const char *url, const char *body, const char *extra_headers);

/**
 * Descarga un archivo desde una URL a un archivo local
 * @param url URL del archivo
 * @param dest_path Ruta de destino local
 * @return 0 si OK, <0 si error
 */
int http_download_file(const char *url, const char *dest_path);

/**
 * Inicializa libcurl (debe llamarse antes de usar HTTP)
 * @return 0 si OK, <0 si error
 */
int http_init(void);

/**
 * Limpia libcurl
 */
void http_cleanup(void);

#endif // HTTP_H
