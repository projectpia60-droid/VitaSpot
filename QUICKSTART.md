# VitaSpot Project Structure & Build Guide

This folder contains the complete VitaSpot project source code.

## Quick Start

### Build
```bash
chmod +x build.sh
./build.sh
```

### Deploy to Vita
```bash
# Via FTP (replace with your Vita IP)
ftp 192.168.1.105 1337
put build/VitaSpot.vpk ux0:data/
quit
```

Then install via VitaShell.

## Project Structure

```
VitaSpot/
├── CMakeLists.txt          ← Build configuration
├── build.sh                ← Build script
├── README.md               ← Full documentation
│
├── src/
│   ├── main.c              ← Entry point
│   ├── message_bus/        ← Central message queue
│   ├── agents/             ← 5 independent agents
│   │   ├── auth/           ← OAuth 2.0 handling
│   │   ├── api/            ← Spotify API calls
│   │   ├── playback/       ← State machine
│   │   ├── ui/             ← Rendering + input
│   │   └── cache/          ← Data persistence
│   ├── spotify/            ← Spotify models & helpers
│   └── utils/              ← HTTP, JSON, logging, crypto
│
├── assets/                 ← Images, fonts (to be added)
└── sce_sys/                ← PS Vita metadata (to be added)
```

## Important: Register Your Spotify App

Before building, you MUST:

1. Go to [developer.spotify.com/dashboard](https://developer.spotify.com/dashboard)
2. Create a new app and get your CLIENT_ID
3. Edit `src/agents/auth/auth_agent.c` and replace:
   ```c
   #define SPOTIFY_CLIENT_ID "YOUR_CLIENT_ID_HERE"
   ```

## Architecture

- **5 Independent Agents** running in separate threads
- **Central Message Bus** for all inter-agent communication
- **Thread-safe** queue with mutex & condition variables
- **Event-driven** design

## Build Requirements

- macOS 10.13+ with Xcode Command Line Tools
- Homebrew
- vitasdk
- Spotify Premium account

See `README.md` for complete setup instructions.

## Next Steps

1. ✅ Register Spotify app (see above)
2. ✅ Run `./build.sh`
3. ✅ Copy VPK to Vita
4. ✅ Install via VitaShell
5. ✅ Authorize app on your Spotify account
6. ✅ Enjoy!

---

For full documentation, see [README.md](README.md)
