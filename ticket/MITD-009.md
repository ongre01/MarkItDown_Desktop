
# MITD-009 — Error Handling 정리

**Priority:** High
**Phase:** Stabilization
**Type:** Improvement
**Dependency:** MITD-005

## 목적

사용자 오류와 Backend 오류를 구분하여 처리한다.

## 오류 종류

```text
Source file not found

Unsupported extension

MarkItDown executable not found

QProcess failed to start

Conversion returned non-zero exit code

Conversion output is empty

Markdown save failed
```

## 역할 분리

### MarkItDownManager

기술적인 실행 오류 처리.

### MainWindow

사용자 메시지 표시.

## 요구사항

오류 발생 시 프로그램이 종료되지 않아야 한다.

가능하면 stderr 내용을 보존한다.

## 완료 조건

각 주요 오류 상황이 사용자에게 구분되어 표시된다.