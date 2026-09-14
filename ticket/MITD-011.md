
# MITD-011 — MVP 코드 리팩터링 및 안정화

**Priority:** Medium
**Phase:** Stabilization
**Type:** Refactoring
**Dependency:** MITD-001 ~ MITD-010

## 목적

새로운 기능을 추가하지 않고 MVP 구조를 정리한다.

## 검토 항목

```text
QObject Ownership
Signal / Slot
Memory Management
const correctness
불필요한 include
중복 코드
함수명
클래스 책임
오류 처리
UI 상태 관리
CMake 구성
Windows Path 처리
QString / UTF-8 처리
```

## 금지사항

다음 기능은 추가하지 않는다.

```text
Batch
Multi Document
Tab
Queue
Database
LLM
RAG
Project Library
```

## 완료 조건

* 기존 기능 동일하게 동작
* Build 성공
* 명백한 중복 제거
* 클래스 역할 명확화
* 새로운 기능 추가 없음
