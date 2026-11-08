---
trigger: model_decision
description: Apply when changing, reviewing, or testing code that reads or writes laptop hardware, ACPI/WMI, EC, fans, keyboard lighting, battery charge limits, CPU power, or NVIDIA settings.
---

# Hardware Safety Invariant

Use documented, model-verified interfaces only. Begin read-only. Never guess EC offsets or write undocumented registers. The GUI must remain unprivileged. Expose only typed, allowlisted control methods through a narrowly scoped privileged service where necessary. Check capabilities and ranges in the backend, verify readback, define safe rollback, preserve firmware protections, and use mocks for automated testing. Unsupported or unverified controls must stay disabled and explain why. No hardware write is allowed without explicit user approval during physical validation.
