
# MITD-010 — UI 상태 관리 통합

**Priority:** Medium
**Phase:** Stabilization
**Type:** Refactoring
**Dependency:** MITD-005, MITD-007

## 목적

각 함수에 흩어진 Action enable/disable 코드를 중앙화한다.

## 함수

```cpp
void MainWindow::updateUiState();
```

## Empty

```text
Open      Enabled
Convert   Disabled
Save      Disabled
```

## Ready

```text
Open      Enabled
Convert   Enabled
Save      Disabled
```

## Converting

```text
Open      Disabled
Convert   Disabled
Save      Disabled
```

## Completed

```text
Open      Enabled
Convert   Enabled
Save      Enabled
```

## Failed

```text
Open      Enabled
Convert   Enabled
Save      Disabled
```

## 요구사항

각 Slot에서 반복적으로:

```cpp
action->setEnabled(...)
```

하는 구조를 최대한 제거한다.

## 완료 조건

Document 상태 변경 후 `updateUiState()` 호출만으로 UI 상태가 일관되게 갱신된다.
