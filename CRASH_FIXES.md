# VitaSpot Error C2-12828-1 — Crash Fixes

## Root Cause Analysis

El error **C2-12828-1** es una excepción no manejada en PSVita, típicamente causada por:
- Acceso inválido a memoria
- Stack overflow en threads
- Prioridades de thread inválidas
- Módulos del kernel no cargados

## Problemas Identificados y Corregidos

### 1. **Thread Priorities INVÁLIDAS** ✅ FIXED
**Problema:**
- Todos los agentes creaban threads con prioridad `0x10000100`
- PSVita requiere prioridades entre **0-191** para user threads
- Valor inválido causaba crash inmediato

**Archivos afectados:**
- `src/agents/auth/auth_agent.c` (línea 193)
- `src/agents/api/api_agent.c` (línea 131)
- `src/agents/playback/playback_agent.c` (línea 116)
- `src/agents/ui/ui_agent.c` (línea 220)
- `src/agents/cache/cache_agent.c` (línea 161)

**Corrección:**
```c
// ANTES (INCORRECTO):
SceUID thread = sceKernelCreateThread("agent_name", agent_thread_func, 0x10000100, 4096, 0, 0, NULL);

// DESPUÉS (CORRECTO):
SceUID thread = sceKernelCreateThread("agent_name", agent_thread_func, 100, 16*1024, 0, 0, NULL);
```

---

### 2. **Stack Size Insuficiente** ✅ FIXED
**Problema:**
- Thread stacks eran muy pequeños (4KB-8KB)
- PSVita requiere mínimo 16KB para threads complejos
- UI Agent necesita 32KB por procesamiento de entrada

**Corrección:**
- Auth/API/Cache/Playback: **16KB cada una**
- UI Agent: **32KB** (maneja input + rendering)

---

### 3. **Módulos de SceKernel No Cargados** ✅ FIXED
**Problema:**
- libcurl necesita módulo de red (`SCE_SYSMODULE_NET`)
- Sin cargar el módulo, `curl_easy_init()` puede fallar

**Archivo:** `src/main.c` (función `init_system()`)

**Corrección:**
```c
static int init_system(void) {
    // Cargar módulos de SceKernel requeridos
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);  // ← AGREGADO
    
    // Resto de inicialización...
    if (logger_init(...) != 0) { ... }
```

**Includes agregados:**
```c
#include <psp2/sysmodule.h>
```

---

### 4. **Directorios de Data No Creados** ✅ FIXED
**Problema:**
- Logger intenta escribir a `ux0:data/vitaspot/vitaspot.log`
- Auth intenta guardar tokens en `ux0:data/vitaspot/tokens.bin`
- Sin directorios, `sceIoOpen()` falla con error negativo

**Archivo:** `src/utils/logger.c` (función `logger_init()`)

**Corrección:**
```c
int logger_init(const char *log_file, LogLevel level) {
    if (!log_file) return -1;

    // Crear directorios necesarios
    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir("ux0:data/vitaspot", 0777);
    sceIoMkdir("ux0:data/vitaspot/art", 0777);

    // Luego abrir el archivo...
    g_log_fd = sceIoOpen(log_file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0644);
```

---

## Resumen de Cambios

| Archivo | Cambio | Razón |
|---------|--------|-------|
| `src/agents/auth/auth_agent.c` | Priority: 0x10000100→100, Stack: 4KB→16KB | Validar parámetros de thread |
| `src/agents/api/api_agent.c` | Priority: 0x10000100→100, Stack: 4KB→16KB | Idem |
| `src/agents/playback/playback_agent.c` | Priority: 0x10000100→100, Stack: 4KB→16KB | Idem |
| `src/agents/ui/ui_agent.c` | Priority: 0x10000100→100, Stack: 8KB→32KB | UI necesita más stack |
| `src/agents/cache/cache_agent.c` | Priority: 0x10000100→100, Stack: 8KB→16KB | Validar parámetros |
| `src/main.c` | `+ sceSysmoduleLoadModule(SCE_SYSMODULE_NET)` | Cargar módulo de red |
| `src/main.c` | `+ #include <psp2/sysmodule.h>` | Include requerido |
| `src/utils/logger.c` | `+ sceIoMkdir()` x3 | Crear directorios de datos |

---

## ¿Por qué C2-12828-1 ocurría?

**Secuencia del crash:**

1. **Al inicio:** Main thread llama `auth_agent_start()`
2. **sceKernelCreateThread() intenta crear thread:**
   - Prioridad `0x10000100` es inválida
   - PSVita kernel valida parámetro
   - **CRASH**: Excepción no manejada → **Error C2-12828-1**
   
O alternativamente:

1. **Thread se crea pero con stack pequeño**
2. **Primer malloc() en Agent falla silenciosamente**
3. **Se intenta acceder a NULL pointer**
4. **CRASH**: Segmentation fault → **Error C2-12828-1**

---

## Cómo Recompilar

```bash
cd /Users/soygabimartina/Documents/Psvita/VitaSpot

# Limpiar build anterior
rm -rf build/

# Compilar
./build.sh

# El archivo VPK estará en: build/VitaSpot.vpk
```

---

## Instalación en PS Vita

```bash
# 1. Transferir VPK a Vita via FTP
ftp <IP_VITA> 1337
put build/VitaSpot.vpk ux0:data/VitaSpot.vpk

# 2. En VitaShell: instalar el VPK
# Navegar a ux0:data/ → seleccionar VitaSpot.vpk → presionar ✕
```

---

## Testing

Después de instalar el VPK corregido:

1. **La app debería iniciar sin crash**
2. **Ver pantalla de login con código de Spotify**
3. **Autorizar en Spotify desde otro dispositivo**
4. **Verificar en el log:** `ux0:data/vitaspot/vitaspot.log`

---

## Cambios No Implementados (Futuro)

- [ ] Vita2d graphics library initialization
- [ ] Album art rendering
- [ ] Full Spotify API integration
- [ ] Performance optimization

---

*Fixes applied: 2024*  
*Target: PS Vita / SceKernel / vitasdk*
