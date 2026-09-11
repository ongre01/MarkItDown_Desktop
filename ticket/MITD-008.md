# MITD-008 — 단일 파일 Drag & Drop

**Priority:** Medium
**Phase:** UX
**Type:** Feature
**Dependency:** MITD-003

## 목적

Windows Explorer에서 문서를 프로그램으로 바로 열 수 있도록 한다.

## 처리

```text
Explorer
   │
   │ test.pdf
   ▼
MainWindow
```

## 하나의 파일

정상적으로 Open 처리.

기존 File Open 함수의 공통 로직을 재사용한다.

## 여러 파일

Drop을 거부한다.

메시지:

```text
한 번에 하나의 파일만 열 수 있습니다.
```

## 요구사항

지원하지 않는 확장자도 거부한다.

## 완료 조건

* 단일 파일 Drop 정상
* 복수 파일 Drop 거부
* Folder Drop 거부
* 지원하지 않는 파일 거부