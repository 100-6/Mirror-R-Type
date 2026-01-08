---
allowed-tools: Bash(git:*), Read, Glob
description: Create a clean commit following project conventions (no Claude footer)
---

# Clean Commit Command

Analyze the current changes and create a commit that follows the project's conventions.

## Instructions

1. Run `git status` to see all changes
2. Run `git diff` for unstaged changes and `git diff --staged` for staged changes
3. Run `git log --oneline -10` to understand the project's commit message style
4. Analyze the changes and determine:
   - The type (feat, fix, refactor, docs, style, test, chore)
   - The scope (game, network, ui, etc.)
   - A clear, concise description
5. Add relevant files with `git add`
6. Create the commit using this format ONLY:

```
type(scope): description

Optional detailed explanation if needed.
```

## Commit Types
- `feat`: New feature
- `fix`: Bug fix
- `refactor`: Code restructuring
- `docs`: Documentation
- `style`: Formatting, whitespace
- `test`: Tests
- `chore`: Build, dependencies

## IMPORTANT RULES
- **NEVER** add "Generated with Claude Code" footer
- **NEVER** add "Co-Authored-By: Claude" footer
- Keep subject line under 72 characters
- Use imperative mood ("add" not "added")
- Only commit what's necessary
- Do NOT commit .env, credentials, or config files like .claude/ or .codex/

Execute the commit with:
```bash
git commit -m "type(scope): description"
```

If a detailed explanation is needed, use:
```bash
git commit -m "$(cat <<'EOF'
type(scope): description

Detailed explanation here.
EOF
)"
```
