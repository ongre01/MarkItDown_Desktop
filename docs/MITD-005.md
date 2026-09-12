# MITD-005 — Convert 기능 연결

## 구현 내용

- `DocumentController`를 추가해 `MainWindow`와 `MarkItDownManager` 사이의 변환 요청 및 결과 신호를 중계한다.
- `Convert` 액션을 현재 문서의 원본 경로에 대한 비동기 변환 요청과 연결했으며, 지원 파일을 정상적으로 열 때도 같은 변환 경로를 자동 호출한다.
- 변환 요청 시 `Document.status`를 `Converting`으로 변경하고 `Open`, `Convert`, `Save` 액션을 비활성화한다.
- 변환 성공 시 stdout Markdown을 `Document.markdown`에 저장하고 상태를 `Rendering`으로 변경한 뒤, 응답성을 유지하는 렌더링 파이프라인으로 편집기와 미리보기에 표시한다.
- 변환 실패 시 상태를 `Failed`로 변경하고 상태 표시줄과 오류 메시지 상자에 원인을 표시한다.
- 빈 문서, 변환 준비, 변환 중, 완료, 실패 상태에 맞춰 창 제목, 상태 표시줄 및 액션 활성화 상태를 일관되게 갱신한다.
- 새 파일을 열 때 이전 Markdown 편집기 문서를 분리해 지연 삭제하고 새 문서로 교체하며, 미리보기 내용을 비운다.

## 변환 완료 렌더링의 UI 응답성

- `MarkdownDocumentRenderer`는 전용 `QThread`에서 `QTextDocument::setMarkdown()`으로 Markdown을 파싱하고 HTML을 생성한다.
- 작업 스레드에는 UI 객체를 전달하지 않는다. 렌더 요청 ID와 Markdown 문자열만 queued signal로 전달하고, 생성한 HTML 문자열을 UI 스레드로 돌려준다.
- 편집기에는 한 번에 전체 Markdown을 설정하지 않는다. 약 32 KiB 단위로 나누되 줄 끝을 우선해 청크 경계를 선택하고, 청크 사이에 1 ms single-shot 타이머를 두어 Windows 메시지 루프에 제어권을 돌려준다.
- 청크 삽입 중에는 편집기를 읽기 전용으로 전환하고 undo 기록과 화면 갱신을 보류한다. 삽입 완료 후 이를 복원하고 문서를 수정되지 않은 상태로 표시한다.
- 기존 편집기 문서를 교체할 때는 먼저 부모를 분리한다. `QPlainTextEdit::setDocument()`가 소유 중인 기존 문서를 즉시 삭제한 뒤 같은 포인터에 `deleteLater()`를 호출하는 use-after-free를 방지하기 위한 것이다.
- 미리보기는 `QTextBrowser`를 사용한다. 편집기 삽입과 작업 스레드의 Markdown 파싱이 모두 끝난 뒤 생성된 HTML을 표시한다.
- 요청마다 증가하는 렌더 ID를 사용한다. 현재 문서와 ID가 다른 완료 신호는 폐기한다.
- 편집기 삽입과 HTML 생성이 모두 끝난 뒤 문서 상태를 `Completed`로 바꾸고 액션을 다시 활성화한다.

## 상태별 UI

| 문서 상태 | Open | Convert | Save | 상태 표시줄 |
| --- | --- | --- | --- | --- |
| `Empty` | 활성 | 비활성 | 비활성 | `Ready` |
| `Ready` | 활성 | 활성 | 비활성 | `<파일명> \| Ready` |
| `Converting` | 비활성 | 비활성 | 비활성 | `<파일명> \| Converting...` |
| `Rendering` | 비활성 | 비활성 | 비활성 | `<파일명> \| Rendering preview...` |
| `Completed` | 활성 | 활성 | 활성 | `<파일명> \| Converted \| UTF-8 \| Markdown` |
| `Failed` | 활성 | 활성 | 비활성 | `<파일명> \| Conversion Failed: <오류>` |

## 런타임 전제

Windows 빌드는 실행 파일과 같은 폴더의 `python-venv`에 MarkItDown CLI를 자동 설치한다. 앱은 `MARKITDOWN_EXECUTABLE`, 앱 로컬 `python-venv`, `PATH`, 저장소의 개발용 `build\python-venv` 순서로 CLI를 찾는다. 따라서 정상 빌드 결과는 별도의 PATH 수정 없이 변환할 수 있다.

## 확인 방법

1. qmake를 다시 실행하고 Qt 6 MSVC 2022 64-bit Debug 구성을 빌드한다.
2. `markitdown`을 찾을 수 있는 환경에서 `example/`의 지원 파일을 열고, 별도 조작 없이 변환이 시작되는지 확인한다.
3. 변환 중 세 액션이 비활성화되며 상태 표시줄에 `Converting...`이 표시되는지 확인한다.
4. `Converting...`과 `Rendering preview...` 중 창을 반복해서 클릭하거나 이동하고 Windows가 창을 `응답 없음`으로 표시하지 않는지 확인한다.
5. 미리보기 로드 완료 후 Markdown이 편집기와 미리보기에 나타나고 상태가 `Converted`로 바뀌며 `Open`, `Convert`, `Save`가 활성화되는지 확인한다.
6. CLI를 찾을 수 없는 환경에서 파일을 열어 자동 변환 오류 메시지 상자와 상태 표시줄이 실패 원인을 표시하고 `Open`과 `Convert`가 다시 활성화되는지 확인한다.
7. 파일을 연 뒤 다른 파일을 다시 열거나 창을 닫아도 접근 위반으로 프로세스가 종료되지 않는지 확인한다.
