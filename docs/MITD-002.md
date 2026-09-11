# MITD-002 — 단일 Document 모델 구현

## 구현 내용

- 현재 문서 상태를 표현하는 `DocumentStatus` 열거형을 추가했다.
- 원본 파일 경로, Markdown 내용, Markdown 저장 경로, 수정 여부, 변환 상태를 보관하는 `Document` 구조체를 추가했다.
- `MainWindow`가 `Document` 값 객체 하나를 멤버로 소유하도록 연결했다.
- 새 문서 상태는 `modified = false`, `status = DocumentStatus::Empty`로 초기화된다.
- 헤더 전용 모델을 qmake 프로젝트의 `HEADERS` 목록에 등록했다.

## 단일 문서 정책

`MainWindow`는 `Document m_document` 하나만 관리한다. 복수 문서 컬렉션이나 변환 큐는 추가하지 않았다.

## 기존 UI 동작

MITD-001에서 구성한 메뉴, 툴바, 편집기, 미리보기, 상태 표시줄 동작은 변경하지 않았다. 파일 열기와 변환 과정에서 문서 상태를 갱신하는 로직은 후속 티켓 범위다.

## 확인 방법

1. Qt 6 MSVC 2022 64-bit 환경에서 qmake를 실행한다.
2. Debug 구성을 빌드해 `MarkItDown_Desktop.exe`가 생성되는지 확인한다.
3. 애플리케이션을 실행해 기존 MainWindow UI가 정상적으로 표시되는지 확인한다.
