# Contributing to SPARK

Thank you for your interest in contributing to SPARK!

This guide explains our contribution workflow, conventions, and review process.

---

## Before You Start

### Understanding Our Workflow

We use a **fork-first workflow**. This means:
- You work on your own copy (fork) of the repository
- Changes are proposed via Pull Requests from your fork
- This keeps the main repository clean and secure

### Why Fork-First?

1. **Security**: Only maintainers have write access to the main repo
2. **Experimentation**: You can freely experiment in your fork
3. **Learning**: Great practice for contributing to any open source project
4. **Backup**: Your fork serves as a backup of your work

---

## Fork-First Workflow

### Step 1: Fork the Repository

1. Click the "Fork" button on the repository page
2. This creates your personal copy at `github.com/YOUR-USERNAME/spark`

### Step 2: Clone Your Fork

```bash
# Clone your fork locally
git clone https://github.com/YOUR-USERNAME/spark.git
cd spark

# Add the original repo as "upstream" for syncing
git remote add upstream https://github.com/The-OASIS-Project/spark.git
```

### Step 3: Keep Your Fork Updated

```bash
# Fetch upstream changes
git fetch upstream

# Merge upstream main into your local main
git checkout main
git merge upstream/main

# Push updates to your fork
git push origin main
```

### Step 4: Create a Feature Branch

**Never work directly on `main`**. Always create a branch:

```bash
git checkout -b type/description
```

---

## Branch Naming Conventions

Use descriptive branch names following this pattern:

```
type/short-description
```

### Branch Types

| Type | Purpose | Example |
|------|---------|---------|
| `feat/` | New feature | `feat/gesture-recognition` |
| `fix/` | Bug fix | `fix/mqtt-reconnect` |
| `docs/` | Documentation only | `docs/wiring-guide` |
| `refactor/` | Code restructuring | `refactor/sensor-abstraction` |
| `test/` | Adding/updating tests | `test/espnow-validation` |
| `chore/` | Maintenance tasks | `chore/update-libraries` |

---

## Conventional Commits

We follow [Conventional Commits](https://www.conventionalcommits.org/) for clear, consistent history.

### Format

```
type(scope): description

[optional body]

[optional footer]
```

### Commit Types

| Type | When to Use |
|------|-------------|
| `feat` | Adding new functionality |
| `fix` | Fixing a bug |
| `docs` | Documentation changes only |
| `style` | Formatting (no code logic change) |
| `refactor` | Restructuring without behavior change |
| `test` | Adding or updating tests |
| `chore` | Maintenance (dependencies, configs) |

### Examples

```bash
# Feature
git commit -m "feat(sensor): add MPU6050 motion detection"

# Bug fix
git commit -m "fix(mqtt): handle broker disconnect gracefully"

# Documentation
git commit -m "docs(readme): add wiring diagram"
```

---

## Coding Standards

### Arduino/C++ Guidelines

- Use descriptive variable and function names
- Comment hardware-specific code and pin assignments
- Keep ISRs (Interrupt Service Routines) minimal
- Prefer non-blocking patterns over `delay()`
- Document magic numbers with defines or constants

### Hardware Considerations

- **Test on actual hardware** - Simulation can't catch everything
- **Document pin assignments** - Include in comments and README
- **Power efficiency** - Consider battery life implications
- **Latency** - Real-time gesture response is critical

### Credentials

- **Never commit credentials** - Use `arduino_secrets.h` template pattern
- Keep `arduino_secrets.h` in `.gitignore`
- Provide `arduino_secrets.h.template` for reference

---

## Pull Request Process

### Before Submitting

- [ ] Code compiles without warnings
- [ ] Tested on target hardware (if applicable)
- [ ] Branch is up-to-date with main
- [ ] Commit messages follow conventions
- [ ] Documentation updated if needed
- [ ] No credentials or secrets in code

### PR Description Template

```markdown
## Summary
Brief description of changes.

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Documentation
- [ ] Refactoring

## Hardware Tested
- [ ] XIAO ESP32-C3
- [ ] XIAO ESP32-S3
- [ ] Other: ___

## Checklist
- [ ] Compiles without warnings
- [ ] Tested on hardware
- [ ] Documentation updated
- [ ] No breaking changes (or documented)
```

---

## Code of Conduct

We are committed to providing a welcoming and inclusive environment.
Please be respectful and constructive in all interactions.

---

## Getting Help

- **Questions**: Open a GitHub Discussion or Issue
- **Bugs**: Open an issue with the bug report template
- **Features**: Open an issue with the feature request template
- **O.A.S.I.S. Ecosystem**: See [S.C.O.P.E.](https://github.com/malcolmhoward/the-oasis-project-meta-repo) for cross-project coordination
