# Contributing

## Commit messages

This repository uses [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <short summary in the imperative, lower case, no period>

<optional body: what and why, wrapped at 72 columns>
```

**Types**: `feat` (new feature), `fix` (bug fix), `docs`, `test`,
`refactor`, `perf`, `build` (build system, dependencies), `chore`.

**Scopes**: `firmware`, `drivers`, `server`, `hardware`, `tools`, `docs`.

Examples:

```
feat(drivers): add VL53L0X distance sensor driver
fix(server): ignore mic frames while the robot is speaking
docs(hardware): explain track tension adjustment
```

Keep one logical change per commit, and make sure these pass before
committing:

```bash
cd firmware && pio run
```

```bash
cd server && python -m pytest
```

After changing `hardware/3d/*.scad`, regenerate the STLs with
`hardware/3d/export.sh` in the same commit.
