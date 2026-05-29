#include "json.h"
#include "logger.h"
#include <string.h>

// ============================================================================
// JSON Wrapper Implementation
// ============================================================================

json_t *json_parse(const char *json_string) {
    if (!json_string) {
        return NULL;
    }

    json_error_t error;
    json_t *root = json_loads(json_string, 0, &error);

    if (!root) {
        log_error("JSON Parse error: %s (line %d)", error.text, error.line);
        return NULL;
    }

    return root;
}

const char *json_get_string(json_t *object, const char *key) {
    if (!object || !key) {
        return NULL;
    }

    json_t *value = json_object_get(object, key);
    if (!value) {
        return NULL;
    }

    if (!json_is_string(value)) {
        return NULL;
    }

    return json_string_value(value);
}

int json_get_integer(json_t *object, const char *key) {
    if (!object || !key) {
        return 0;
    }

    json_t *value = json_object_get(object, key);
    if (!value) {
        return 0;
    }

    if (!json_is_integer(value)) {
        return 0;
    }

    return (int)json_integer_value(value);
}

int json_get_boolean(json_t *object, const char *key) {
    if (!object || !key) {
        return 0;
    }

    json_t *value = json_object_get(object, key);
    if (!value) {
        return 0;
    }

    if (!json_is_boolean(value)) {
        return 0;
    }

    return json_boolean_value(value) ? 1 : 0;
}

json_t *json_get_array(json_t *object, const char *key) {
    if (!object || !key) {
        return NULL;
    }

    json_t *value = json_object_get(object, key);
    if (!value || !json_is_array(value)) {
        return NULL;
    }

    return value;
}

json_t *json_get_object(json_t *object, const char *key) {
    if (!object || !key) {
        return NULL;
    }

    json_t *value = json_object_get(object, key);
    if (!value || !json_is_object(value)) {
        return NULL;
    }

    return value;
}
