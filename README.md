# VitaSpot — PS Vita Spotify Controller  

**VitaSpot** es un cliente de Spotify para PlayStation Vita que funciona como controlador remoto de reproducción mediante Spotify Connect.

## Características

- ✅ Autenticación OAuth 2.0 Device Authorization Flow
- ✅ Control de reproducción (play/pause/siguiente/anterior)
- ✅ Toggle shuffle y repeat
- ✅ Visualización de canción actual con metadatos
- ✅ Caché de tokens y metadatos
- ✅ Arquitectura basada en agentes (5 agentes independientes + message bus)

## Limitaciones Importantes

⚠️ **La Vita NO puede decodificar streams de Spotify directamente** (DRM). VitaSpot actúa como **controlador remoto** solamente. Necesitás un dispositivo Spotify activo (celular, PC, altavoz) que sea el reproductor.

## Requisitos Previos

### Hardware
- PlayStation Vita (OLED o Slim)
- CFW **HENkaku** o **Ensō** instalado
- Memoria suficiente (~30MB para la app)
- Conexión Wi-Fi

### Software
- macOS (10.13+) con Xcode Command Line Tools
- Homebrew
- vitasdk compilado
- Cliente Spotify Premium

## Setup del Entorno

### 1. Instalar vitasdk (macOS)

```bash
# Instalar dependencias base
brew install cmake git curl zip unzip python3 pkg-config

# Clonar e instalar vitasdk (tarda ~20 minutos)
git clone https://github.com/vitasdk/vdpm
cd vdpm
./bootstrap-vitasdk.sh

# Configurar PATH (macOS zsh)
echo 'export VITASDK=/usr/local/vitasdk' >> ~/.zshrc
echo 'export PATH=$VITASDK/bin:$PATH' >> ~/.zshrc
source ~/.zshrc

# Verificar instalación
arm-vita-eabi-gcc --version
```

### 2. Instalar librerías necesarias

```bash
vdpm libcurl jansson vita2d libpng libjpeg freetype2
```

### 3. Registrar aplicación en Spotify

1. Ir a [developer.spotify.com/dashboard](https://developer.spotify.com/dashboard)
2. Crear una aplicación nueva
3. Obtener el `CLIENT_ID`
4. Ir a "Edit Settings" y agregar OAuth Redirect URI: `http://localhost:8080/callback`
5. Copiar el `CLIENT_ID` en `src/agents/auth/auth_agent.c`:

```c
#define SPOTIFY_CLIENT_ID "tu_client_id_aqui"
```

## Compilación

### Build automático

```bash
chmod +x build.sh
./build.sh
```

El script generará `build/VitaSpot.vpk`.

### Build manual

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake
make -j$(sysctl -n hw.ncpu)
```

## Instalación en PS Vita

### Método 1: FTP (desarrollo)

```bash
# En la Vita: VitaShell → Select → Habilitar FTP
# Anotar IP (ej: 192.168.1.105:1337)

# En macOS:
ftp 192.168.1.105 1337
put build/VitaSpot.vpk ux0:data/VitaSpot.vpk
quit

# En VitaShell: ux0:data/ → VitaSpot.vpk → [Cross] → Instalar
```

### Método 2: USB

1. Conectar Vita por USB
2. VitaShell → Select → USB
3. Copiar `VitaSpot.vpk` a la raíz
4. Instalar desde VitaShell

### Crear directorios necesarios (primera vez)

```
VitaShell → ux0:data/
- Crear carpeta: vitaspot
- Dentro crear: art (para caché de carátulas)
```

## Uso

### Primera Vez (Login)

1. Abre VitaSpot en la Vita
2. Verás una pantalla con:
   - URL: `https://www.spotify.com/pair?uri=...`
   - Código de 8 caracteres (ej: `ABCD1234`)
3. En tu celular/PC, abre esa URL
4. Ingresa el código
5. Aprueba acceso a VitaSpot
6. La app se sincroniza automáticamente

### Controles

| Botón | Acción |
|---|---|
| ✕ (Cross) | Play/Pause |
| D-Pad ←/→ | Canción anterior/siguiente |
| D-Pad ↑/↓ | Navegar menús |
| L (LTrigger) | Toggle Shuffle |
| R (RTrigger) | Cambiar Repeat Mode |
| ○ (Circle) | Ir a Playlists |
| SELECT | Selector de dispositivos |
| START | Ir a Now Playing |

## Arquitectura

VitaSpot utiliza una arquitectura de **5 agentes independientes** que se comunican via un message bus thread-safe centralizado:

```
┌─────────────┐
│ Auth Agent  │ ← OAuth 2.0, token refresh
├─────────────┤
│ API Agent   │ ← Llamadas a Spotify Web API
├─────────────┤
│Playback Agn │ ← Gestión de estado local
├─────────────┤
│  UI Agent   │ ← Renderizado + input
├─────────────┤
│ Cache Agn   │ ← Persistencia de datos
└─────────────┘
       ↓
  Message Bus
  (thread-safe)
```

### Archivos Principales

```
src/
├── main.c              ← Entry point, inicializa agentes
├── message_bus/
│   └── bus.{h,c}      ← Message queue centralizada
├── agents/
│   ├── auth/          ← OAuth, token management
│   ├── api/           ← Spotify API calls
│   ├── playback/      ← State machine local
│   ├── ui/            ← Rendering + input
│   └── cache/         ← Data persistence
├── spotify/
│   ├── spotify_models.h   ← Data structs
│   ├── spotify_auth.{h,c} ← OAuth helpers
│   └── spotify_api.{h,c}  ← API wrappers
└── utils/
    ├── http.{h,c}     ← libcurl wrapper
    ├── json.{h,c}     ← jansson wrapper
    ├── logger.{h,c}   ← File logging
    ├── base64.{h,c}   ← Encoding
    └── sha256.{h,c}   ← Hashing
```

## Logging

Los logs se guardan en: `ux0:data/vitaspot/vitaspot.log`

Para ver logs en tiempo real:
```bash
# Via FTP/VitaShell: Descargar el archivo
# O desde terminal si tenés acceso SSH a la Vita
```

## Troubleshooting

### "Failed to request device code"
- Verificar conexión Wi-Fi
- Verificar que CLIENT_ID está correcto en `auth_agent.c`
- Verificar que la app está registrada en Spotify Dashboard

### "Authorization pending" se queda pegado
- Verificar que ingresaste el código correcto en el celular
- El código expira en 10 minutos
- Reintentar cerrando y abriendo la app

### No hay dispositivos Spotify activos
- VitaSpot necesita un dispositivo REPRODUCTOR activo
- Abre Spotify en tu celular/PC y que esté activamente reproduciendo
- En VitaSpot, ir a SELECT → Dispositivos

### Lag en la UI
- Esto es normal en la Vita por limitaciones de hardware
- Reducir frecuencia de polling de API (cambiar POLL_PLAYBACK_INTERVAL_MS)

## Roadmap

### v0.2
- [ ] Interfaz gráfica con vita2d (reemplazar terminal)
- [ ] Visualización de carátulas descargadas
- [ ] Navegación de playlists

### v0.3
- [ ] Búsqueda de canciones
- [ ] Historial local
- [ ] Animaciones suaves

### v1.0
- [ ] Publicar en VitaDB
- [ ] Soporte multi-idioma
- [ ] Temas personalizables

## Limitaciones Técnicas

### Hardware
- **512MB RAM** compartida → limitar caché de imágenes
- **ARM Cortex-A9** → sin aceleración 3D
- **FreeBSD SceKernel** → limitado a 128 threads

### Software
- **Sin DRM de Spotify** → solo control remoto (Spotify Connect)
- **SSL/TLS en PSVita** → requiere bundling de certificados
- **Pantalla OLED 960x544** → interfaz compacta

## Desarrollo

### Cambiar CLIENT_ID

Editar `src/agents/auth/auth_agent.c`:

```c
#define SPOTIFY_CLIENT_ID "your_new_id_here"
```

Recompilar y reinstalar VPK.

### Agregar nuevas Llamadas API

1. Agregar función a `src/spotify/spotify_api.{h,c}`
2. Definir nuevo tipo de mensaje en `src/message_bus/bus.h`
3. Manejar el mensaje en el agente correspondiente

## Referencias

- [Spotify Web API](https://developer.spotify.com/documentation/web-api/)
- [PSVita SDK Documentation](https://github.com/vitasdk)
- [vita2d Graphics Library](https://github.com/Rinnegatamante/vita2d)

## Licencia

MIT License - Ver LICENSE para detalles

## Créditos

Desarrollado como parte del proyecto VitaSpot.
Basado en arquitectura de agentes para máxima modularidad.

---

**¿Preguntas?** Abre un issue en el repositorio.
