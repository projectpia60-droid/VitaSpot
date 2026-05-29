#pragma once

#include <jansson.h>

// ============================================================================
// JSON Wrapper — Simplificaciones sobre jansson
// ============================================================================

/**
 * Parsea una string JSON
 * @param json_string String JSON
 * @return json_t root object, NULL si error
 */
json_t *json_parse(const char *json_string);

/**
 * Obtiene un string de un objeto JSON
 * @param object Objeto JSON
 * @param key Clave
 * @return Valor string o NULL
 */
const char *json_get_string(json_t *object, const char *key);

/**
 * Obtiene un integer de un objeto JSON
 * @param object Objeto JSON
 * @param key Clave
 * @return Valor integer o 0
 */
int json_get_integer(json_t *object, const char *key);

/**
 * Obtiene un booleano de un objeto JSON
 * @param object Objeto JSON
 * @param key Clave
 * @return 1 si true, 0 si false
 */
int json_get_boolean(json_t *object, const char *key);

/**
 * Obtiene un array de un objeto JSON
 * @param object Objeto JSON
 * @param key Clave
 * @return json_t array o NULL
 */
json_t *json_get_array(json_t *object, const char *key);

/**
 * Obtiene un objeto anidado
 * @param object Objeto JSON
 * @param key Clave
 * @return json_t object o NULL
 */
json_t *json_get_object(json_t *object, const char *key);

#endif // JSON_H
