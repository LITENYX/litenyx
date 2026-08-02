# Security Policy

## Reporting a vulnerability

Litenyx is an experimental research protocol built on Dogecoin Core 1.14.9. Security issues should be reported by email to the repository maintainer rather than public GitHub issues.

**Do not open public issues for security vulnerabilities.**

## Vulnerability classification

### Consensus vulnerabilities

A consensus vulnerability allows a participant to create valid blocks or transactions that violate the protocol's consensus rules. These are the highest priority. Report immediately.

### Implementation defects

An implementation defect causes the software to deviate from its specification without violating consensus rules. These may affect network health or node stability. Report as standard issues after verifying they are not exploitable.

### Specification ambiguities

A specification ambiguity exists when normative protocol documents (`litenyx-spec`) do not uniquely determine correct behavior across all valid inputs. Report these as they may lead to consensus divergence. These are resolved through the specification review process.

### Documentation errors

Documentation errors in `litenyx-walkthrough` or supporting documentation do not affect consensus or implementation. Report through standard issues.

### Evidence integrity issues

Evidence integrity issues in `Litenyx-oracle` or execution reports may affect provenance tracking. Report through standard issues.

## Response expectations

- Consensus vulnerabilities: acknowledgment within 72 hours, fix timeline based on severity
- Implementation defects: acknowledgment within 1 week, fix in next development cycle
- Specification ambiguities: routed to specification review process
- Documentation errors: fixed on next review cycle
- Evidence integrity issues: corrected on next scan

## Scope

This security policy covers all five Litenyx repositories:

| Repository | Scope |
|---|---|
| litenyx | Implementation, consensus code, build system |
| litenyx-spec | Normative protocol specifications |
| litenyx-plan | Planning, research, gate definitions |
| litenyx-walkthrough | Explanatory documentation |
| Litenyx-oracle | Provenance index, scanner tooling |

## Upstream dependency

Litenyx builds on Dogecoin Core 1.14.9. Security vulnerabilities in the upstream dependency should be reported to the Dogecoin Core project. Litenyx-specific modifications are in the `deploy/patches/` directory.

## Disclosure

Litenyx is experimental research software with no mainnet or real value. Coordinated disclosure is preferred but not strictly required for non-consensus issues. Consensus vulnerabilities should follow responsible disclosure practices.
