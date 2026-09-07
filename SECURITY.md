# Security Policy

## Supported Versions

We support the latest version of the C++ Concurrency Benchmarking Suite:

| Version | Supported          |
| ------- | ------------------ |
| Latest  | :white_check_mark: |

## Reporting a Vulnerability

If you discover a security vulnerability within this benchmarking suite, please report it via one of:

1. **GitHub Security Advisories**: use [Report a vulnerability](https://github.com/harshilshah2501/cpp-concurrency-bench/security/advisories/new) on this repository (preferred).
2. **GitHub Issue**: open a private maintainer discussion or issue titled `[SECURITY]` if advisories are unavailable.

Do **not** post exploit details in public issues when the impact could affect systems that reuse these patterns.

### Response Timeline

- **Initial Response**: Within 48 hours
- **Assessment**: Within 1 week
- **Fix**: Based on severity (critical issues prioritized)

### Scope

This security policy covers:
- ✅ **Benchmark code safety** (memory safety, race conditions)
- ✅ **Build system security** (CMake, dependencies)
- ✅ **Script safety** (shell injection, file permissions)
- ❌ **External dependencies** (Google Benchmark library - report upstream)
- ❌ **Operating system vulnerabilities** (not our scope)

### Safe Usage Guidelines

For safe benchmarking:

```bash
# Run in isolated environment
docker run --rm -it benchmark-container

# Limit resource usage
ulimit -t 300  # 5 minute timeout
ulimit -v 4194304  # 4GB memory limit

# Use proper permissions
chmod +x scripts/*.sh  # Only executable scripts
```

### Vulnerability Disclosure

Once fixed:
1. **Credit** will be given to the reporter (if desired)
2. **Fix details** will be documented in CHANGELOG.md
3. **Advisory** will be published if impact is significant

Thank you for helping keep the C++ Concurrency Benchmarking Suite secure! 🔒