# Guide de Création de Releases

Ce document explique comment créer des releases Windows pour le jeu R-Type avec un installer automatique.

## Vue d'ensemble

Le projet utilise :
- **NSIS (Nullsoft Scriptable Install System)** pour créer l'installer Windows
- **GitHub Actions** pour automatiser la compilation et la création des releases
- **PowerShell scripts** pour la préparation locale des fichiers

## Méthode 1 : Release Automatique via GitHub Actions (Recommandé)

### Prérequis
- Accès push au repository GitHub
- Droits pour créer des tags

### Étapes

1. **Préparer votre code**
   ```bash
   git add .
   git commit -m "Prepare release v1.0.0"
   git push origin main
   ```

2. **Créer un tag de version**
   ```bash
   # Format: v[MAJOR].[MINOR].[PATCH]
   git tag v1.0.0
   git push origin v1.0.0
   ```

3. **Attendre la compilation**
   - GitHub Actions détecte automatiquement le tag
   - La compilation Windows démarre automatiquement
   - Durée estimée : 15-30 minutes

4. **Vérifier la release**
   - Allez sur `https://github.com/[votre-repo]/releases`
   - La release apparaît avec deux fichiers :
     - `RType-Setup.exe` : Installer Windows
     - `RType-Windows-x64.zip` : Version portable

### Déclencher Manuellement

Vous pouvez aussi déclencher la compilation manuellement :

1. Allez dans l'onglet "Actions" sur GitHub
2. Sélectionnez "Windows Release Build"
3. Cliquez sur "Run workflow"
4. Choisissez la branche et lancez

## Méthode 2 : Compilation Locale de l'Installer

### Prérequis

1. **Windows 10/11 avec Visual Studio 2022**
2. **NSIS** : Télécharger depuis [nsis.sourceforge.io](https://nsis.sourceforge.io/)
3. **vcpkg** : Déjà inclus dans le projet

### Étapes

1. **Compiler le projet**
   ```powershell
   # Depuis la racine du projet
   .\build.bat
   # Ou manuellement avec CMake
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```

2. **Préparer les fichiers pour l'installer**
   ```powershell
   cd installer
   .\prepare-installer.ps1
   ```

   Ce script :
   - Copie les exécutables (client et serveur)
   - Copie les plugins (.dll)
   - Copie les dépendances vcpkg
   - Copie les assets
   - Crée un dossier `build/installer-package`

3. **Vérifier le contenu**
   ```powershell
   Get-ChildItem build\installer-package -Recurse
   ```

   Vous devriez voir :
   ```
   ├── r_type_client.exe
   ├── r_type_server.exe
   ├── *.dll (dépendances)
   ├── plugins\
   │   ├── raylib_graphics.dll
   │   ├── asio_network.dll
   │   ├── miniaudio_audio.dll
   │   └── ...
   └── assets\
       └── fonts\
   ```

4. **Copier les fichiers vers le répertoire build**
   ```powershell
   Copy-Item build\installer-package\* build\ -Recurse -Force
   ```

5. **Créer l'installer avec NSIS**
   ```powershell
   # Depuis la racine du projet
   makensis installer\rtype-installer.nsi
   ```

6. **L'installer est créé**
   - Fichier : `installer/RType-Setup.exe`
   - Taille : ~50-100 MB (selon les dépendances)

## Structure de l'Installer

L'installer NSIS :
- ✅ Installe dans `C:\Program Files\R-Type` par défaut
- ✅ Crée des raccourcis dans le menu Démarrer
- ✅ Crée un raccourci sur le Bureau
- ✅ Ajoute une entrée "Ajout/Suppression de programmes"
- ✅ Inclut un désinstalleur
- ✅ Vérifie Windows 64-bit
- ✅ Support multilingue (Anglais/Français)

## Tester l'Installer

1. **Installation**
   ```powershell
   # Lancer l'installer
   .\installer\RType-Setup.exe
   ```

2. **Vérification**
   - Lancer R-Type depuis le menu Démarrer
   - Vérifier que le jeu se lance correctement
   - Tester le client et le serveur

3. **Désinstallation**
   - Via le menu Démarrer > R-Type > Uninstall
   - Ou via Paramètres Windows > Applications

## Versioning

Suivez [Semantic Versioning](https://semver.org/) :

- **MAJOR** : Changements incompatibles (v2.0.0)
- **MINOR** : Nouvelles fonctionnalités compatibles (v1.1.0)
- **PATCH** : Corrections de bugs (v1.0.1)

Exemples :
```bash
git tag v1.0.0  # Première release
git tag v1.0.1  # Correction de bug
git tag v1.1.0  # Nouvelle fonctionnalité
git tag v2.0.0  # Changement majeur
```

## Troubleshooting

### Problème : "DLL manquantes"

**Solution :** Vérifier que vcpkg a bien compilé les dépendances
```powershell
ls vcpkg\installed\x64-windows\bin\*.dll
```

### Problème : "Plugins non trouvés"

**Solution :** Vérifier que les plugins sont compilés
```powershell
ls build\plugins\*.dll
# Ou
ls build\Release\plugins\*.dll
```

### Problème : "NSIS ne trouve pas les fichiers"

**Solution :** Vérifier que les fichiers sont bien copiés dans `build/`
```powershell
# Re-exécuter le script de préparation
cd installer
.\prepare-installer.ps1

# Copier manuellement si nécessaire
Copy-Item build\installer-package\* build\ -Recurse -Force
```

### Problème : "GitHub Action échoue"

**Solutions possibles :**
1. Vérifier que `vcpkg.json` est à jour
2. Vérifier que le CMakeLists.txt compile correctement
3. Consulter les logs dans l'onglet "Actions"
4. Vérifier que tous les fichiers nécessaires existent

## Fichiers Importants

- [`installer/rtype-installer.nsi`](../installer/rtype-installer.nsi) : Script NSIS
- [`.github/workflows/windows-release.yml`](../.github/workflows/windows-release.yml) : GitHub Action
- [`installer/prepare-installer.ps1`](../installer/prepare-installer.ps1) : Script de préparation
- [`vcpkg.json`](../vcpkg.json) : Dépendances vcpkg
- [`CMakeLists.txt`](../CMakeLists.txt) : Configuration CMake

## Checklist de Release

Avant de créer une release, vérifier :

- [ ] Le code compile sans erreurs
- [ ] Les tests passent
- [ ] La version est mise à jour dans le code si nécessaire
- [ ] Le CHANGELOG est à jour (si vous en avez un)
- [ ] Les assets sont à jour
- [ ] Un tag git est créé avec le bon format (v1.0.0)
- [ ] La GitHub Action passe avec succès
- [ ] L'installer a été testé sur une machine propre

## Distribution

Une fois la release créée :

1. **GitHub Releases** : Automatique via GitHub Actions
2. **Partage direct** : Distribuer `RType-Setup.exe`
3. **Version portable** : Distribuer `RType-Windows-x64.zip`

## Support Multi-plateforme

Pour l'instant, seul Windows est supporté pour les releases automatiques.

Pour supporter Linux/macOS :
- Créer des GitHub Actions similaires pour Linux (.deb, .AppImage)
- Créer des GitHub Actions pour macOS (.dmg)

## Ressources

- [NSIS Documentation](https://nsis.sourceforge.io/Docs/)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Semantic Versioning](https://semver.org/)
- [vcpkg Documentation](https://vcpkg.io/)

---

Pour toute question, consultez la [documentation principale](README.md) ou ouvrez une issue sur GitHub.