# Antigravity permission lists

These files are **copy/paste templates for the Antigravity App permission editor**. They are not auto-loaded by Antigravity and are not a supported on-disk permission configuration format for the desktop app.

- `ALLOW.txt`: narrow read-only commands and documentation domains to reduce routine prompts.
- `ASK.txt`: commands with system, hardware, dependency, Git, browser, or MCP side effects that must remain approval-gated.
- `DENY.txt`: credential locations and direct Git metadata writes that should remain blocked.

`DENY` takes precedence over `ASK`, which takes precedence over `ALLOW`. Do not add broad wildcards such as `command(*)`, `read_file(*)`, `write_file(*)`, or `mcp(*)` to Allow.

## Apply in Antigravity App

1. Open the Acer repository as the active Antigravity workspace.
2. Open that project's Settings using the gear icon beside the project.
3. Under Project Settings → Agent Settings → Permission Settings, choose **Auto**. Do not choose Turbo. If Auto is unavailable because the Terminal Sandbox is unavailable or the account does not support it, use Default.
4. In Project Settings, open **Project-level permissions**. Add the rules from `ALLOW.txt`, `ASK.txt`, and the generated `DENY.txt` to the matching access lists if your installed build exposes the list editor there.
5. If granular lists are available only under Global Permissions on your build, do not paste these project-specific rules globally unless you intentionally want them applied to every workspace. Use project-scoped approvals (such as “Always allow in project”) for individual safe operations where supported.
6. Restart the agent session and verify the project preset and permissions.

Official documentation describes both project-level permissions and global fine-grained Allow/Ask/Deny lists, but the precise editing surface can vary with Antigravity version. Do not edit undocumented application settings files to automate this step.

## Important behavior

- Auto runs commands inside the Terminal Sandbox without prompting. Outside-sandbox commands are automatically evaluated; safe/reversible commands can proceed while risky/irreversible actions still require approval.
- Rules override the preset. An Ask rule will still prompt even if a command could otherwise run in Auto.
- `read_url(domain)` authorizes page reading and adds the domain to the sandbox network allowlist. It does **not** authorize the browser agent to navigate or interact with pages; browser navigation remains approval-gated unless separately allowed.
- MCP calls stay Ask-gated. Add a specific `mcp(server/tool)` Allow rule only after verifying the exact MCP server/tool and its side effects.
- `read_file(path)` Deny also blocks writes to that path. `write_file(.git/)` protects Git metadata from direct file writes.
- Terminal permission rules are not a substitute for Linux user permissions, Polkit, or root authorization.

Official documentation:
- https://antigravity.google/docs/agent-settings
- https://antigravity.google/docs/permissions
