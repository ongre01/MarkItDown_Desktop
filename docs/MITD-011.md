# MITD-011 — MVP 코드 리팩터링 및 안정화

## 구현 범위

새 기능을 추가하지 않고 MITD-001부터 MITD-010까지의 단일 문서 처리 흐름을 다음
책임 단위로 정리했다.

```text
MainWindow
  ├── 사용자 입력, 대화상자, 위젯 및 Document 상태 반영
  ├── DocumentFileOperations ── 경로 검증, 저장 경로, UTF-8 원자 저장
  ├── MarkdownRenderState ───── 렌더 요청 ID와 초기 렌더 완료 조건
  ├── ConversionErrorPresentation ─ 변환 오류의 UI 제목과 요약
  └── DocumentController
          └── MarkItDownManager ── QProcess 실행과 기술 오류 분류
```

Batch, 다중 문서, Tab, Queue, Database, LLM, RAG, Project Library는 추가하지 않았다.
현재 빌드 기준은 `MarkItDown_Desktop.pro`를 사용하는 qmake이며, 설계서의 향후 예시인
CMake 구성은 만들거나 변경하지 않았다.

## 파일 처리 책임

`src/io/DocumentFileOperations`를 추가해 다음 로직을 `MainWindow`에서 분리했다.

- 지원 확장자와 파일 대화상자 패턴 생성
- 폴더, 존재하지 않는 경로, 일반 파일이 아닌 경로, 미지원 확장자의 분류
- 원본 파일과 같은 폴더의 기본 Markdown 저장 경로 생성
- 확장자가 없는 Save As 경로의 `.md` 보정 및 절대 경로 정규화
- `QSaveFile` 기반의 BOM 없는 UTF-8 원자 저장

`MainWindow`는 검증 결과에 대응하는 기존 사용자 메시지를 표시하고, 성공한 절대
경로만 `Document`에 반영한다. 파일 시스템 세부 처리와 UI 표현을 분리하면서 Open과
재변환에서 같은 검증 경로를 사용하도록 했다.

## 렌더 상태 관리

초기 구현에서 `MainWindow`에 개별 필드로 있던 활성 요청 ID, 마지막 요청 ID, HTML
완료 여부, 편집기 삽입 완료 여부 및 초기 렌더 여부를 `MarkdownRenderState` 값 객체로
묶었다.

초기 변환 결과는 다음 두 작업이 모두 끝나야 `Completed`가 된다.

```text
Markdown 변환 완료
  ├── 편집기 청크 삽입 완료 ─┐
  └── 작업 스레드 HTML 생성 ─┴─► Preview 반영 및 Completed
```

편집 중 새 요청을 시작하거나 변환을 다시 시작하면 기존 요청을 무효화한다. 완료되거나
무효화된 요청 ID의 늦은 결과는 계속 폐기한다. 요청 ID가 정수 오버플로로 예약 값 0이
되면 다음 값으로 넘겨 비활성 상태와 충돌하지 않도록 했다.

## 소유권과 Signal/Slot

- `Ui::MainWindow`는 `std::unique_ptr`로 소유해 수동 `delete`를 제거했다.
- `DocumentController`, 두 `QTimer`, `MarkItDownManager` 및 `QProcess`는 기존처럼 부모
  `QObject`가 소유한다. 생성 후 다시 대입하지 않는 QObject 포인터는 const 포인터로
  표시했다.
- 작업 스레드로 이동하는 `MarkdownDocumentRenderer`는 부모를 두지 않고,
  `QThread::finished`에서 `deleteLater()`로 정리한다.
- `MainWindow` 종료 시 양방향 렌더 연결을 끊고 스레드에 `quit()`을 요청한 뒤
  `wait()`하여 렌더러보다 스레드가 먼저 파괴되지 않게 한다.
- 렌더 완료 연결은 `Qt::QueuedConnection`을 명시해 UI 변경이 UI 스레드에서만
  실행됨을 드러냈다.

## 오류 및 UI 상태

- `ConversionErrorPresentation`이 `ConversionError`별 제목과 사용자 요약을 담당한다.
  `MainWindow`는 상태 표시줄과 메시지 상자 표시만 담당한다.
- `MarkItDownManager::reportProcessFailure()`로 프로세스 오류의 중복 억제, 진단 조합,
  실패 signal 발생을 한 경로로 통합했다.
- `collectStandardError()`와 `decodedStandardError()`처럼 역할을 드러내는 함수명으로
  정리했다.
- `MainWindow::setDocumentStatus()`는 상태 변경 직후 `updateUiState()`가 누락되지 않게
  하며, 기존 `DocumentStatus`별 액션 활성화 정책을 그대로 사용한다.
- 저장 가능 조건은 `canSaveMarkdown()` 한 곳에서 확인한다.

## 경로 및 문자열 처리

- QProcess 프로그램과 입력 파일 인자는 계속 분리해 공백이 있는 Windows 경로를
  셸 해석 없이 전달한다.
- 내부 경로는 `QFileInfo`와 `QDir`로 절대 경로화하고, 사용자 표시에서만
  `QDir::toNativeSeparators()`를 사용한다.
- MarkItDown stdout/stderr는 `QString::fromUtf8()`로 읽고, Markdown 저장은
  `QString::toUtf8()`로 기록한다.
- 자식 프로세스의 `PYTHONUTF8=1`, `PYTHONIOENCODING=utf-8` 설정을 유지한다.

## 검증 기록

2026-09-14에 다음을 확인했다.

1. 변경 전 Qt 6.11.0 / MSVC 2022 x64 Debug 기준 빌드가 성공했다.
2. 변경 후 qmake와 Debug 빌드가 성공했으며 새 소스가 모두 컴파일·링크됐다.
3. 실행 파일 폴더의 `python-venv\Scripts\markitdown.exe` 준비가 성공했다.
4. 앱 로컬 MarkItDown으로 `example/test.pdf`를 실제 변환해 종료 코드 0과 646줄의
   Markdown 출력을 확인했다.
5. `build/MITD011-tests`의 임시 QtCore 하네스로 유효/누락/미지원 경로 분류, 기본
   `.md` 경로, BOM 없는 UTF-8 저장, 초기 렌더 완료 장벽, 완료 및 무효 요청 폐기를
   실행했고 종료 코드 0을 확인했다. 이 하네스는 무시된 빌드 산출물이며 저장소
   소스에는 포함하지 않는다.
6. 실제 GUI를 실행해 창 생성, 초기 제목 `MarkItDown Viewer`, 상태 `Ready`를
   접근성 트리에서 확인했다.

GUI 파일 선택 이후의 자동 변환·편집·저장은 데스크톱 자동화 런타임 연결이 종료되어
이번 작업에서 수동 확인하지 못했다. 따라서 빌드와 CLI 및 분리 모듈 검증 결과를 GUI
전체 동작 검증으로 확대해서 해석하지 않는다.

## 수동 회귀 확인 항목

1. 지원 파일 하나를 Open 또는 Drag & Drop하면 자동 변환이 시작되는지 확인한다.
2. `Converting...`과 `Rendering preview...` 중 네 액션이 비활성이고 창이 응답하는지
   확인한다.
3. 초기 렌더 완료 후 편집기와 미리보기가 표시되고 액션이 다시 활성화되는지 확인한다.
4. Markdown 편집 뒤 미리보기와 제목의 `*`가 갱신되는지 확인한다.
5. Save와 Save As가 BOM 없는 UTF-8을 기록하고 저장 경로를 재사용하는지 확인한다.
6. 복수 파일·폴더·미지원 확장자 드롭이 기존 메시지와 함께 거부되는지 확인한다.
7. MarkItDown 실행 실패와 저장 실패 뒤에도 앱을 계속 사용할 수 있는지 확인한다.
