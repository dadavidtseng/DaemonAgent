# Archived Specifications Index

This directory contains specifications that have been archived. Archived specs are preserved for historical reference and can be restored if needed.

## Archive Structure

Each archived specification includes:
- Original requirements, design, and tasks documents
- `ARCHIVE_INFO.md` - Archive metadata and restoration instructions
- Implementation logs (if any implementation was completed)

## Archived Specifications

### Command Queue Refactoring
- **Archived**: 2025-12-12
- **Status**: Planning complete, not implemented
- **Tasks**: 0/25 completed
- **Summary**: Unified Command Queue Architecture & SOLID Module Separation
- **Location**: `./command-queue-refactoring/`
- **Archive Info**: [ARCHIVE_INFO.md](./command-queue-refactoring/ARCHIVE_INFO.md)

## Restoration Process

To restore an archived specification:

1. **Move spec back to active directory**:
   ```bash
   mv ".spec-workflow/archived-specs/{spec-name}" ".spec-workflow/specs/{spec-name}"
   ```

2. **Verify status**:
   ```bash
   spec-status --specName {spec-name}
   ```

3. **Resume work** from the appropriate phase/task

## Archive Guidelines

Specifications should be archived when:
- Project direction changes and the spec is no longer needed
- Implementation is deferred indefinitely
- The spec needs significant rework before implementation
- User explicitly requests archival

Archived specs preserve:
- ✅ All planning documents (requirements, design, tasks)
- ✅ Implementation logs (if any)
- ✅ Archive metadata and context
- ✅ Full restoration capability

---

*Last updated: 2025-12-12*
