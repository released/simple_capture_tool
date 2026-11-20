# Updating the GitHub repository

The tracked update helper is `scripts/github_push.ps1`. It uses Windows PowerShell and Git, with the existing GitHub credentials configured for Git. The project-root `github_push.bat` is an optional local entry and is excluded from the repository.

A checkout without the local batch can use `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/github_push.ps1 -DryRun` for preview, then the same command without `-DryRun` for interactive publication.

1. Run `github_push.bat -DryRun` to review the destination and changed paths.
2. Run `github_push.bat` and inspect the displayed changes.
3. Enter `PUSH` to commit and push, or another response to cancel.

- The destination is the public `released/simple_capture_tool` repository on branch `main`.
- The script checks the live repository identity, branch ancestry and exact staged content. A changed destination, remote-ahead state or unexpected staged work stops publication.
- Existing local settings, generated outputs and engineering records remain excluded. Existing tracked delivery policy retains `build/SimpleCaptureTool.exe`.
- An unchanged repository creates no duplicate commit. A failed commit or push retains local work for inspection.
- Application builds, test runs, cleanup, repository creation and history rewriting are separate operations.
- A verified branch update does not establish completion of GitHub Actions or Pages deployment.

For an already reviewed automated task, `-TaskScopeFile` accepts an ignored local JSON manifest containing schema `1`, the unchanged `base_head`, and explicit `files` entries with `path` and lowercase `sha256`. An owned deletion uses `deleted`. The manifest must match every pending public change, and the live branch must equal the baseline. Combine with `-DryRun` for preview or `-Yes` for an authorized noninteractive update.

The helpers in `scripts/github_push.ps1` and `scripts/github_publish_core.ps1` are included in the checkout; no sibling workspace is required.

[Project overview](../README.md) · [Build instructions](BUILDING.md)
