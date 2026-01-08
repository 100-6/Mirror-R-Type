---
name: parser
description: Explorer la codebase rapidement (map + points d'entrée + où modifier)
argument-hint: [quick|medium|very-thorough] [question]
model: claude-3-5-haiku-20241022
allowed-tools: Read, Grep, Glob, Bash(git status:*), Bash(git log:*), Bash(git diff:*), Bash(git ls-files:*), Bash(ls:*), Bash(find:*), Bash(head:*), Bash(tail:*)
disable-model-invocation: true
---

## Contexte (auto)
- Repo: !`git rev-parse --show-toplevel`
- Branche: !`git branch --show-current`
- Status: !`git status --porcelain=v1 -b`
- Racine (top-level): !`ls -1`
- Dossiers (maxdepth 2): !`find . -maxdepth 2 -type d -not -path '*/.git*' | head -n 120`

## Mission
Tu es en **lecture seule**.

1) Utilise le subagent **Explore** (thoroughness = $1, défaut: medium).
2) Réponds à : $ARGUMENTS

## Format de sortie
1) Vue d’ensemble (5–10 bullets)
2) Points d’entrée (fichiers + chemins)
3) Fichiers “hotspots” (liste courte, pourquoi)
4) “Où modifier quoi” (actionnable)
5) 1 seule question si nécessaire
