# Miniaudio Audio Plugin

Plugin audio pour R-Type basé sur la bibliothèque [miniaudio](https://miniaud.io/).

## Caractéristiques

- **Léger** : Miniaudio est une bibliothèque header-only, pas de dépendances externes
- **Multi-plateforme** : Supporte Linux, Windows, macOS
- **Effets sonores** : Chargement et lecture de sons avec contrôle de volume et pitch
- **Musique** : Streaming de musique avec boucle automatique
- **Thread-safe** : Protection par mutex pour utilisation multi-thread
- **Contrôles globaux** : Volume master, mute, volume par piste

## Formats supportés

Miniaudio supporte nativement :
- **WAV**
- **MP3**
- **FLAC**
- **Vorbis/OGG**

## Compilation

Le plugin se compile automatiquement lors du build du projet :

```bash
mkdir build && cd build
cmake ..
cmake --build . --target miniaudio_audio
```

Le fichier `miniaudio_audio.so` sera généré dans `build/plugins/`.

## Utilisation

### Exemple basique

```cpp
#include "PluginManager.hpp"
#include "IAudioPlugin.hpp"

int main() {
    rtype::PluginManager manager;

    // Charger le plugin
    auto* audio = manager.load_plugin<rtype::IAudioPlugin>(
        "./plugins/miniaudio_audio.so",
        "create_audio_plugin"
    );

    // Initialiser
    audio->initialize();

    // Charger et jouer un son
    auto sound = audio->load_sound("assets/shoot.wav");
    audio->play_sound(sound, 0.8f, 1.0f); // volume 0.8, pitch normal

    // Charger et jouer de la musique
    auto music = audio->load_music("assets/background.mp3");
    audio->play_music(music, true, 0.5f); // loop, volume 0.5

    // Contrôles
    audio->set_master_volume(0.7f);
    audio->pause_music();
    audio->resume_music();
    audio->stop_music();

    // Nettoyage
    audio->unload_sound(sound);
    audio->unload_music(music);
    audio->shutdown();

    return 0;
}
```

### API complète

#### Initialisation
- `bool initialize()` - Initialise le moteur audio
- `void shutdown()` - Arrête et nettoie tout
- `bool is_initialized()` - Vérifie l'état d'initialisation

#### Sons (Sound Effects)
- `SoundHandle load_sound(const std::string& path)` - Charge un son
- `void unload_sound(SoundHandle handle)` - Décharge un son
- `bool play_sound(SoundHandle, float volume, float pitch)` - Joue un son
- `void stop_sound(SoundHandle)` - Arrête un son
- `bool is_sound_playing(SoundHandle)` - Vérifie si un son joue

#### Musique
- `MusicHandle load_music(const std::string& path)` - Charge une musique (streaming)
- `void unload_music(MusicHandle)` - Décharge une musique
- `bool play_music(MusicHandle, bool loop, float volume)` - Joue une musique
- `void stop_music()` - Arrête la musique actuelle
- `void pause_music()` - Met en pause
- `void resume_music()` - Reprend la lecture
- `bool is_music_playing()` - Vérifie si la musique joue
- `void set_music_volume(float)` - Règle le volume de la musique
- `float get_music_volume()` - Obtient le volume actuel

#### Contrôles globaux
- `void set_master_volume(float)` - Volume général (0.0 - 1.0)
- `float get_master_volume()` - Obtient le volume général
- `void set_muted(bool)` - Active/désactive le mute
- `bool is_muted()` - Vérifie si muté

## Tests

Pour tester le plugin :

```bash
cd build
./tests/test_miniaudio_plugin
```

Le test vérifie :
- Chargement du plugin
- Initialisation
- Contrôles de volume
- Chargement et lecture de sons/musique
- Pause/Resume
- Clamping des valeurs
- Déchargement

**Note** : Le test utilisera les fichiers `assets/test_sound.wav` et `assets/music.mp3` s'ils existent, sinon il sautera ces tests.

## Sécurité

Le plugin inclut plusieurs protections :

- **Wrappers C** : Les fonctions factory sont protégées contre les exceptions C++
- **Validation** : Tous les paramètres sont validés (chemins, handles, etc.)
- **Clamping** : Les volumes sont automatiquement limités à [0.0, 1.0]
- **Thread-safety** : Mutex sur toutes les opérations critiques
- **Gestion d'erreurs** : Exceptions claires avec messages d'erreur

## Architecture

```
miniaudio/
├── include/
│   ├── MiniaudioPlugin.hpp  # Interface du plugin
│   └── miniaudio.h           # Bibliothèque miniaudio (téléchargée auto)
├── src/
│   └── MiniaudioPlugin.cpp   # Implémentation
├── CMakeLists.txt            # Configuration de build
└── README.md                 # Cette documentation
```

## Dépendances

- **CMake** 3.20+
- **C++20**
- **Bibliothèques système** :
  - Linux : `pthread`, `m`, `dl`
  - macOS : CoreAudio, AudioToolbox, CoreFoundation
  - Windows : winmm

## Notes techniques

- La bibliothèque miniaudio est téléchargée automatiquement lors du build
- Les sons utilisent le chargement complet en mémoire
- La musique utilise le streaming pour économiser la RAM
- Un seul morceau de musique peut jouer à la fois
- Les sons multiples peuvent jouer simultanément

## Références

- [Miniaudio GitHub](https://github.com/mackron/miniaudio)
- [Documentation IAudioPlugin](../../plugin_manager/include/IAudioPlugin.hpp)
- [Guide création de plugin](../../plugin_manager/HOW_TO_CREATE_PLUGIN.md)

