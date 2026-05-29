#include "http.h"
#include "logger.h"
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <psp2/io/fcntl.h>

// ============================================================================
// libcurl Helper Structures
// ============================================================================

typedef struct {
    char  *data;
    size_t size;
    size_t capacity;
} ResponseBuffer;

// ============================================================================
// Callbacks for libcurl
// ============================================================================

static size_t curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    ResponseBuffer *buf = (ResponseBuffer *)userp;

    // Redimensionar buffer si es necesario
    if (buf->size + realsize + 1 > buf->capacity) {
        size_t new_capacity = buf->capacity + realsize + 1024;
        char *ptr = realloc(buf->data, new_capacity);
        if (!ptr) {
            log_error("HTTP: No memory for response buffer");
            return 0;
        }
        buf->data = ptr;
        buf->capacity = new_capacity;
    }

    // Copiar datos
    memcpy(&(buf->data[buf->size]), contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = 0; // Null-terminate

    return realsize;
}

static int curl_progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                                   curl_off_t ultotal, curl_off_t ulnow) {
    // Callback para mostrar progreso, por ahora no hacer nada
    (void)clientp;
    (void)dltotal;
    (void)dlnow;
    (void)ultotal;
    (void)ulnow;
    return 0; // Retornar 0 para continuar, non-zero para abortar
}

// ============================================================================
// HTTP Implementation
// ============================================================================

int http_init(void) {
    // libcurl se auto-inicializa en la mayoría de casos
    // En PSVita, asegurarse de que el stack de red está listo
    return 0;
}

void http_cleanup(void) {
    // libcurl se limpia automáticamente al final
}

char *http_get(const char *url, const char *extra_headers) {
    if (!url) {
        return NULL;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        log_error("HTTP: Failed to init curl");
        return NULL;
    }

    ResponseBuffer response = {0};
    response.data = malloc(1024);
    if (!response.data) {
        curl_easy_cleanup(curl);
        return NULL;
    }
    response.capacity = 1024;
    response.size = 0;

    // Configurar curl
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_progress_callback);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    // SSL/TLS en PSVita a menudo necesita deshabilitación de verificación
    // En producción, usar bundle de certificados
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // Headers adicionales
    struct curl_slist *headers = NULL;
    if (extra_headers) {
        // Parsear headers (formato: "Header1: Value1\r\nHeader2: Value2")
        headers = curl_slist_append(headers, extra_headers);
    }

    // HTTP/1.1
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Realizar request
    CURLcode res = curl_easy_perform(curl);

    // Limpiar
    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        log_error("HTTP GET failed: %s", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    return response.data;
}

char *http_post(const char *url, const char *body, const char *content_type,
                 const char *extra_headers) {
    if (!url || !body) {
        return NULL;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        log_error("HTTP: Failed to init curl");
        return NULL;
    }

    ResponseBuffer response = {0};
    response.data = malloc(1024);
    if (!response.data) {
        curl_easy_cleanup(curl);
        return NULL;
    }
    response.capacity = 1024;
    response.size = 0;

    // Configurar curl
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // Headers
    struct curl_slist *headers = NULL;
    if (content_type) {
        char content_type_header[256];
        snprintf(content_type_header, sizeof(content_type_header),
                 "Content-Type: %s", content_type);
        headers = curl_slist_append(headers, content_type_header);
    }
    if (extra_headers) {
        headers = curl_slist_append(headers, extra_headers);
    }

    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Realizar request
    CURLcode res = curl_easy_perform(curl);

    // Limpiar
    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        log_error("HTTP POST failed: %s", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    return response.data;
}

int http_put(const char *url, const char *body, const char *extra_headers) {
    if (!url || !body) {
        return -1;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        log_error("HTTP: Failed to init curl");
        return -1;
    }

    ResponseBuffer response = {0};
    response.data = malloc(1024);
    if (!response.data) {
        curl_easy_cleanup(curl);
        return -1;
    }
    response.capacity = 1024;
    response.size = 0;

    // Configurar curl
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // Headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (extra_headers) {
        headers = curl_slist_append(headers, extra_headers);
    }

    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Realizar request
    CURLcode res = curl_easy_perform(curl);

    // Limpiar
    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    if (response.data) {
        free(response.data);
    }

    if (res != CURLE_OK) {
        log_error("HTTP PUT failed: %s", curl_easy_strerror(res));
        return -1;
    }

    return 0;
}

int http_download_file(const char *url, const char *dest_path) {
    if (!url || !dest_path) {
        return -1;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        log_error("HTTP: Failed to init curl");
        return -1;
    }

    // Abrir archivo destino
    SceUID fd = sceIoOpen(dest_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0644);
    if (fd < 0) {
        log_error("HTTP: Failed to open file: %s", dest_path);
        curl_easy_cleanup(curl);
        return -1;
    }

    // Configurar curl
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // Callback de escritura directa a archivo
    auto write_to_file = [](void *contents, size_t size, size_t nmemb, void *userp) -> size_t {
        size_t realsize = size * nmemb;
        SceUID *pfd = (SceUID *)userp;
        sceIoWrite(*pfd, contents, realsize);
        return realsize;
    };

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void *contents, size_t size,
                                                       size_t nmemb, void *userp) -> size_t {
        size_t realsize = size * nmemb;
        SceUID *pfd = (SceUID *)userp;
        sceIoWrite(*pfd, contents, realsize);
        return realsize;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&fd);

    // Realizar request
    CURLcode res = curl_easy_perform(curl);

    // Limpiar
    sceIoClose(fd);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        log_error("HTTP Download failed: %s", curl_easy_strerror(res));
        sceIoRemove(dest_path);
        return -1;
    }

    return 0;
}
