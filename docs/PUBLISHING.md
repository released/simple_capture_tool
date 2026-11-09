# Updating the GitHub repository

The update entry is `github_push.bat` in the project root. It uses Windows PowerShell and Git, with the existing GitHub credentials configured for Git.

1. Run `github_push.bat -DryRun` to review the destination and changed paths.
2. Run `github_push.bat` and inspect the displayed changes.
3. Enter `PUSH` to commit and push, or another response to cancel.

- The destination is the public `released/simple_capture_tool` repository on branch `main`.
- The script checks the live repository identity, branch ancestry and exact staged content. A changed destination, remote-ahead state or unexpected staged work stops publication.
- Existing local settings, generated outputs and engineering records remain excluded. Existing tracked delivery policy retains `build/SimpleCaptureTool.exe`.
- An unchanged repository creates no duplicate commit. A failed commit or push retains local work for inspection.
- Application builds, test runs, cleanup, repository creation and history rewriting are separate operations.
- A verified branch update does not establish completion of GitHub Actions or Pages deployment.

The helpers in `scripts/github_push.ps1` and `scripts/github_publish_core.ps1` are included in the checkout; no sibling workspace is required.

[Project overview](../README.md) · [Build instructions](BUILDING.md)
