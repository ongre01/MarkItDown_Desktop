# Unit Test Guide

## 1. Purpose

This document defines the rules for designing, implementing, and maintaining automated tests for the MarkItDown GUI application.

The primary goals of the test suite are:

* Detect regressions in core application behavior.
* Verify business logic independently from the GUI where possible.
* Verify error handling and boundary conditions.
* Verify asynchronous state transitions deterministically.
* Keep tests fast, repeatable, isolated, and suitable for CI execution.
* Avoid tests that depend unnecessarily on the developer machine environment.

Tests must verify observable behavior and documented contracts rather than internal implementation details whenever possible.

---

## 2. Required Context

Before creating or modifying tests:

1. Read `agent-rules/PROJECT_GOAL.md`.
2. Read `agent-rules/BUILD_GUIDE.md`.
3. Read this document.
4. Inspect the implementation under test.
5. Inspect existing tests for the same component before adding new tests.

Do not infer behavior from class or function names alone.

The implementation, project requirements, and existing documented behavior must be used to determine the expected result.

If expected behavior is ambiguous, do not invent a new contract silently.

Document the ambiguity under `docs/` or clearly identify it in the implementation report.

---

## 3. Test Framework

Use **Qt Test** as the default unit test framework.

Prefer Qt-native test utilities where appropriate:

```cpp
#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
```

For asynchronous Qt behavior, prefer:

```cpp
QSignalSpy
QTRY_VERIFY
QTRY_COMPARE
```

Do not use arbitrary sleeps such as:

```cpp
QThread::sleep(...)
QThread::msleep(...)
```

unless there is no deterministic alternative and the reason is documented.

---

## 4. Test Categories

Tests must be separated according to their responsibility.

```text
tests/
├── unit/
├── component/
└── ui/
```

### Unit Tests

Unit tests verify isolated application logic.

Typical targets include:

* file validation
* path normalization
* Markdown file writing
* render state management
* error mapping
* Markdown rendering logic
* deterministic utility functions

Unit tests must avoid real external processes, interactive dialogs, network resources, and machine-specific dependencies whenever possible.

### Component Tests

Component tests verify interactions between a small number of application components.

Typical targets include:

* `DocumentController`
* `MarkItDownManager`
* process execution abstraction
* signal forwarding
* conversion success and failure behavior

External dependencies should be replaced with a Fake or controlled test implementation where practical.

### UI Tests

UI tests verify user-visible workflows.

Typical targets include:

```text
Open
  ↓
Convert
  ↓
Edit
  ↓
Preview
  ↓
Save
```

Do not test private `MainWindow` helper functions individually merely to increase coverage.

Verify behavior through the public or user-visible interface.

---

## 5. Test Priority

Use the following priority when adding tests.

### P0 — Core deterministic logic

Tests must be added first for logic that:

* validates user input
* reads or writes files
* controls asynchronous state
* prevents stale results from being applied
* maps errors to application behavior
* has multiple branches or failure paths

Examples:

```text
DocumentFileOperations
MarkdownRenderState
ConversionErrorPresentation
```

### P1 — Component boundaries

Examples:

```text
MarkdownDocumentRenderer
DocumentController
MarkItDownManager
```

### P2 — GUI orchestration

Examples:

```text
MainWindow
dialogs
widget state
end-to-end GUI workflows
```

Do not begin with UI tests when the same behavior can be verified at a lower level.

---

## 6. Test Naming

Test names must describe behavior, not implementation mechanics.

Preferred style:

```text
<operation>_<condition>_<expectedResult>
```

Examples:

```cpp
validateSourceDocument_missingFile_returnsNotFound()
validateSourceDocument_directory_returnsDirectoryError()
normalizedMarkdownPath_withoutExtension_addsMdExtension()

beginUpdateRequest_newRequest_invalidatesPreviousRequest()
invalidate_activeRequest_rejectsOldResult()

render_heading_containsHeadingText()
writeMarkdownUtf8_unicodeText_preservesContent()
```

Avoid vague names such as:

```text
test1
basicTest
testConvert
testError
```

A developer should understand the failed behavior from the test name alone.

---

## 7. Test Structure

Each test should follow an Arrange / Act / Assert structure.

Example:

```cpp
void TestDocumentFileOperations::
    validateSourceDocument_missingFile_returnsNotFound()
{
    // Arrange
    const QString path =
        QDir::temp().filePath("file_that_does_not_exist.docx");

    // Act
    const auto result =
        DocumentFileOperations::validateSourceDocument(path);

    // Assert
    QCOMPARE(result.error, SourceDocumentError::NotFound);
    QVERIFY(!result.isValid());
}
```

Keep setup limited to what the test requires.

Do not share mutable state between unrelated tests.

---

## 8. Test Independence

Every test must be independently executable.

A test must not depend on:

* another test running first
* files generated by another test
* the current working directory unless explicitly configured
* developer-specific absolute paths
* installed MarkItDown executables unless the test is explicitly an integration test
* user configuration
* previously modified global state

Prefer:

```cpp
QTemporaryDir
QTemporaryFile
```

for filesystem tests.

Temporary test resources must be cleaned automatically whenever practical.

---

## 9. Filesystem Tests

Filesystem tests must use temporary locations.

Do not write Unit Test output into:

```text
project root
source directories
docs/
user Documents folder
desktop
fixed C:\ paths
```

Preferred:

```cpp
QTemporaryDir tempDir;
```

Tests involving Unicode must include non-ASCII data.

Example:

```text
English
한글
日本語
```

When testing UTF-8 output, write the content and read it back to verify round-trip correctness.

---

## 10. Error Paths

Success-path-only testing is insufficient.

For every operation capable of failing, identify and test meaningful failure branches.

For example, if an enum contains:

```text
None
Directory
NotFound
NotFile
UnsupportedExtension
```

tests should exercise each meaningful state.

If conversion errors contain:

```text
AlreadyRunning
ExecutableNotFound
FailedToStart
Crashed
NonZeroExit
EmptyOutput
ProcessFailure
```

each error should have an explicit test or data-driven test entry.

Do not add enum values or failure cases merely to make tests easier.

Tests must reflect production behavior.

---

## 11. Data-Driven Tests

Use Qt data-driven tests when multiple inputs verify the same behavior.

Example:

```cpp
void TestConversionErrorPresentation::presentation_data()
{
    QTest::addColumn<ConversionError>("error");

    QTest::newRow("AlreadyRunning")
        << ConversionError::AlreadyRunning;

    QTest::newRow("ExecutableNotFound")
        << ConversionError::ExecutableNotFound;

    QTest::newRow("FailedToStart")
        << ConversionError::FailedToStart;
}
```

Prefer data-driven testing over duplicated test functions when only the input and expected value differ.

---

## 12. Asynchronous Behavior

Asynchronous logic must be tested deterministically.

A critical scenario is stale result rejection.

Example:

```text
Request #1 starts
        │
Request #2 starts
        │
Request #2 finishes
        │
Request #1 finishes late
```

Expected behavior:

```text
Request #2 → accepted
Request #1 → rejected
```

Any state-management implementation responsible for asynchronous work must have tests covering:

* current request acceptance
* previous request rejection
* invalidation
* completion
* out-of-order completion

Do not assume asynchronous callbacks arrive in request order.

---

## 13. State Machine Tests

When a class manages multiple completion conditions, test the state combinations explicitly.

For example, if an initial preview requires both:

```text
Rendered HTML available
AND
Editor insertion completed
```

verify all combinations:

| Render Complete | Editor Complete | Ready |
| --------------- | --------------- | ----- |
| No              | No              | No    |
| Yes             | No              | No    |
| No              | Yes             | No    |
| Yes             | Yes             | Yes   |

Do not test only the final successful state.

---

## 14. External Process Boundary

Unit tests must not depend directly on launching the real MarkItDown executable unless the test is explicitly categorized as an integration or component test.

For process-dependent code, prefer an abstraction such as:

```text
MarkItDownManager
        │
        ▼
IProcessRunner
        ▲
        │
 ┌──────┴──────────┐
 │                 │
QProcessRunner   FakeProcessRunner
Production       Test
```

The Fake implementation should allow tests to simulate:

* successful completion
* startup failure
* crash
* non-zero exit code
* empty stdout
* stderr output
* delayed completion

Do not introduce unnecessary production abstractions only for coverage.

Add a seam when it meaningfully improves determinism, isolation, or maintainability.

---

## 15. Executable Resolution

Tests must not assume that the MarkItDown executable exists on the current machine.

Executable discovery logic should be testable separately from process execution whenever practical.

Machine-dependent values such as:

```text
application directory
developer PATH
Python environment
absolute executable paths
```

must not be embedded into Unit Test expectations.

If executable resolution requires environmental behavior, use a controlled resolver or component test.

---

## 16. Signal Tests

Use `QSignalSpy` when the observable contract is a Qt signal.

Example:

```cpp
QSignalSpy spy(
    &controller,
    &DocumentController::conversionFinished);

controller.convert(sourcePath);

QTRY_COMPARE(spy.count(), 1);
```

Verify signal arguments when they form part of the public contract.

Do not verify irrelevant internal signal ordering unless application correctness depends on it.

---

## 17. Assertions

Prefer the most specific assertion available.

Examples:

```cpp
QCOMPARE(actual, expected);
QVERIFY(condition);
QVERIFY2(condition, "reason");
```

Avoid assertions that provide little diagnostic information.

Prefer:

```cpp
QCOMPARE(result.error, SourceDocumentError::NotFound);
```

over:

```cpp
QVERIFY(result.error != SourceDocumentError::None);
```

when the exact result is part of the contract.

---

## 18. String and HTML Assertions

Do not compare generated HTML as one complete string unless exact serialization is itself the required behavior.

Qt versions may produce equivalent HTML with different formatting.

Prefer semantic assertions.

Example:

```cpp
QVERIFY(html.contains("Title"));
QVERIFY(html.contains("Hello"));
QVERIFY(html.contains("World"));
```

Do not make tests unnecessarily sensitive to:

* whitespace
* formatting
* attribute ordering
* implementation-generated boilerplate

unless those details are requirements.

---

## 19. Mocking Rules

Prefer, in order:

```text
Real deterministic object
        ↓
Fake
        ↓
Stub
        ↓
Mock
```

Use mocks only when interaction verification is actually required.

Do not mock simple deterministic value objects.

Do not mock Qt classes merely because they are Qt classes.

Avoid tests where most of the test code configures mocks instead of verifying application behavior.

---

## 20. Production Code Changes for Testing

Production code may be refactored to improve testability when behavior remains unchanged.

Acceptable examples:

* dependency injection
* extracting deterministic utility functions
* introducing a process runner interface
* separating file operations from UI
* separating executable resolution from process execution

Unacceptable examples:

* making private members public only for tests
* adding test-only branches to production logic
* changing behavior merely to satisfy a test
* exposing internal state that has no production purpose

Do not use:

```cpp
#define private public
```

or equivalent tricks.

---

## 21. Private Functions

Do not write tests directly against private functions.

Private behavior should normally be verified through the public interface that uses it.

If a private function contains sufficiently complex independent logic that requires direct testing, consider extracting that logic into a dedicated class or function with a clear responsibility.

---

## 22. Regression Tests

When fixing a bug:

1. Reproduce the bug.
2. Add a test that fails because of the bug whenever practical.
3. Apply the fix.
4. Verify the new test passes.
5. Run related existing tests.

The regression test name or nearby comment should make the protected behavior clear.

Do not remove a regression test after the defect is fixed.

---

## 23. Test Coverage

Coverage percentage is a diagnostic metric, not the primary goal.

Do not:

* create meaningless tests solely to increase percentage
* test trivial getters without behavioral value
* expose implementation details solely for coverage
* assert that code executed without validating results

Prioritize coverage of:

```text
branches
failure paths
state transitions
boundary values
race-condition prevention
file handling
external dependency boundaries
```

over raw line coverage.

---

## 24. Build Integration

Tests must be integrated into the project's existing build system.

Follow `agent-rules/BUILD_GUIDE.md` for the authoritative build commands.

Do not create an independent test build procedure that conflicts with the project's normal build workflow.

Where CTest is available, tests should be registered so they can be run through the standard test command.

Typical structure:

```cmake
enable_testing()

add_subdirectory(tests)
```

and:

```cmake
add_test(
    NAME tst_DocumentFileOperations
    COMMAND tst_DocumentFileOperations
)
```

Exact commands and paths must follow the actual project configuration.

---

## 25. Test Failure Policy

When a test fails:

1. Determine whether the implementation or the test expectation is wrong.
2. Inspect the implementation and documented requirement.
3. Do not automatically modify the test to match the current implementation.
4. Do not automatically modify production code merely to make the test pass.
5. Preserve existing intended behavior unless a requirement explicitly changes it.

If the correct behavior cannot be determined, report the ambiguity.

---

## 26. Existing Tests

Before adding a test:

* search for an existing test covering the same behavior
* extend an existing data-driven test where appropriate
* avoid duplicate scenarios
* preserve established naming and structure unless there is a reason to improve them

When refactoring tests, preserve existing behavioral coverage.

---

## 27. Documentation

For every test implementation task, create or update the corresponding documentation under `docs/`.

The documentation should describe, as appropriate:

```text
Test target
Test scenarios
New regression coverage
Required production refactoring
Known limitations
How the tests were executed
Result
```

Do not create test implementation documentation outside `docs/`.

Documentation must reflect what was actually implemented and executed.

Do not state that tests passed unless they were actually executed successfully.

---

## 28. Verification Before Completion

Before considering a Unit Test task complete:

```text
[ ] Production code under test was inspected.
[ ] Existing related tests were inspected.
[ ] Tests follow project naming conventions.
[ ] Tests are isolated from developer-specific paths.
[ ] Temporary resources are cleaned up.
[ ] Success paths are covered.
[ ] Relevant failure paths are covered.
[ ] Async behavior does not rely on arbitrary sleeps.
[ ] The test target builds.
[ ] The new tests were executed.
[ ] Related existing tests were executed where practical.
[ ] Test results are reported accurately.
[ ] docs/ documentation was created or updated.
```

If any verification step could not be performed, state that explicitly.

---

## 29. Initial Unit Test Targets

For the current MarkItDown GUI architecture, prioritize tests in this order:

```text
1. DocumentFileOperations
2. MarkdownRenderState
3. ConversionErrorPresentation
4. MarkdownDocumentRenderer
5. DocumentController
6. MarkItDownManager
7. MainWindow UI behavior
```

The first milestone should focus on deterministic core logic before process and GUI tests.

---

## 30. Core Principle

Tests must protect application behavior, not duplicate implementation.

Prefer:

```text
Given an input or state
        ↓
Perform an operation
        ↓
Verify an observable result
```

over:

```text
Call every function
        ↓
Check that every line executed
```

A test is valuable when its failure clearly indicates that a meaningful application behavior has changed.
