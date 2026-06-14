# Security Policy

## Supported versions

Security fixes are applied to the latest release and the `main` branch only.
Older versions do not receive backports.

## Scope

This policy covers **libapg** itself. Vulnerabilities in upstream dependencies
(libarchive, lmdb, yyjson, gpgme, libsodium) should be reported to their
respective maintainers.

libapg is part of the [NurOS](https://nuros.org) project. Issues that affect
NurOS at a broader level may be forwarded to the NurOS security team at their
discretion.

## Reporting a vulnerability

**Do not open a public GitHub issue for security vulnerabilities.**

Use one of the following channels:

- **GitHub Private Vulnerability Reporting** (preferred) — use the
  "Report a vulnerability" button on the
  [Security tab](../../security/advisories/new) of this repository. Reports
  remain private until resolved.
- **Direct contact** — reach out privately to the maintainers listed on
  [nuros.org](https://nuros.org). We recommend contacting multiple maintainers
  who are familiar with libapg or the NurOS project.

Include in your report:

- A description of the vulnerability and its potential impact
- Steps to reproduce or a proof-of-concept
- Affected versions or commits
- Any suggested fix, if you have one

## Response timeline

We aim to meet the following targets on a best-effort basis:

| Milestone | Target |
|-----------|--------|
| Acknowledgement of receipt | 72 hours |
| Confirmed or dismissed | 7 days |
| Fix released | 90 days |

If a deadline cannot be met, we will communicate this in the private report
thread and provide an updated estimate.

## Disclosure policy

Vulnerabilities are kept private until a fix is released. Once patched, a
public security advisory will be published via GitHub Security Advisories.

We do not set a hard public disclosure deadline before a fix is available.
If a vulnerability is already publicly known or actively exploited, we will
coordinate disclosure on a case-by-case basis.

## Recognition

We maintain no bug bounty program. However, reporters who responsibly disclose
valid vulnerabilities will be credited in the security advisory unless they
prefer to remain anonymous.
