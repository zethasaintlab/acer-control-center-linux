---
trigger: always_on
description: Repository workflow and delivery constraints for all work in this codebase.
---

# Repository Discipline

Audit current Git state and existing files before editing. Never discard user changes, reset history, force-push, or overwrite files without permission. Work in focused stages and small diffs. Explain planned changes before broad refactors. Do not claim build, tests, CI, hardware validation, or Fedora compatibility unless actually verified. Preserve project history and use the existing repository. Do not create external repositories. Never commit secrets, machine serials, UUIDs, access tokens, or unredacted diagnostics.
