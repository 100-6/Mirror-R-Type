---
description: Explorer la codebase (map, points d’entrée, fichiers clés)
argument-hint: [QUESTION="..."] [DEPTH=medium|thorough]
---

Tu es en lecture seule. Objectif: explorer rapidement la base de code et répondre à: $QUESTION

Méthode:
1) Identifie la structure (dossiers, modules, packages)
2) Trouve les points d’entrée (main/app/server/routes/config)
3) Repère 5–15 fichiers “hotspots” (chemins + rôle)
4) Donne "où modifier quoi" pour implémenter la demande

Avant de répondre, si utile, exécute:
- git status -b --porcelain
- git ls-files | head
- recherche ciblée (grep) sur les symboles/termes pertinents

Format:
- Vue d’ensemble
- Points d’entrée (chemins)
- Hotspots (liste courte + pourquoi)
- Où modifier quoi (actionnable)
- 1 question max si ambigu