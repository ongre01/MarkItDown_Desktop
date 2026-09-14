# MITD-006 — Markdown Preview

## 구현 범위

- 중앙 `QSplitter`의 왼쪽은 `QPlainTextEdit`, 오른쪽은 `QTextBrowser`로 구성한다.
- MarkItDown 변환 결과를 편집기에 넣고 Qt 내장 Markdown 파서로 HTML을 만든 뒤 미리보기에 표시한다.
- 제목, 표, 목록 등 Qt가 지원하는 Markdown 요소를 렌더링한다.
- `QWebEngineView`와 외부 Markdown 라이브러리는 사용하지 않는다.
- Save/Save As 등 MITD-007 이후 기능은 구현하지 않는다.

## 변환 완료 흐름

```text
conversionFinished(markdown)
        │
        ├── Document.markdown 갱신
        ├── Document.modified = false
        ├── 편집기 텍스트를 이벤트 루프 단위로 나누어 삽입
        └── 작업 스레드에서 QTextDocument::setMarkdown()
                         │
                         ▼
                 QTextDocument::toHtml()
                         │
                         ▼
                  QTextBrowser::setHtml()
```

Markdown 파싱은 `MarkdownDocumentRenderer`가 별도 `QThread`에서 수행한다. 편집기에는
큰 변환 결과를 여러 청크로 나누어 넣는다. 두 작업이 모두 끝날 때까지 문서 상태를
`Rendering`으로 유지하고 상태 표시줄에 `Rendering preview...`를 표시한다. 완료 후
`Completed`로 바꾸므로 변환 및 렌더링 도중 UI 이벤트 루프가 계속 동작한다.

프로그램이 변환 결과를 편집기에 삽입하는 동안에는 편집기를 읽기 전용으로 두며,
이때 발생한 `textChanged`는 사용자 수정으로 처리하지 않는다. 삽입 완료 시 편집기의
내부 modified 플래그와 `Document.modified`를 모두 `false`로 유지한다.

## 사용자 편집 흐름

`QPlainTextEdit::textChanged`가 발생하고 문서 상태가 `Completed`이면 다음을 수행한다.

1. 편집기 전체 텍스트를 `Document.markdown`에 반영한다.
2. `Document.modified`를 `true`로 설정하고 창 제목에 `*`를 표시한다.
3. 150ms 단일 실행 타이머로 연속 입력을 모은다.
4. 최신 Markdown을 작업 스레드에서 다시 렌더링한다.
5. 요청 번호가 현재 요청과 같은 결과만 `QTextBrowser`에 반영한다.

변환이 시작되면 대기 중이거나 실행 중인 편집 미리보기 요청을 무효화한다. 따라서
`Converting` 상태에서는 이전 편집 요청의 결과가 미리보기에 반영되지 않는다.

## 의존성과 파일

- Qt 모듈: Core, Gui, Widgets
- UI: `mainwindow.ui`
- 상태 및 화면 연동: `mainwindow.h`, `mainwindow.cpp`
- 비동기 Markdown 파싱: `src/rendering/MarkdownDocumentRenderer.h`,
  `src/rendering/MarkdownDocumentRenderer.cpp`
- 렌더 요청 ID와 초기 완료 조건: `src/rendering/MarkdownRenderState.h`,
  `src/rendering/MarkdownRenderState.cpp`

WebEngine 모듈, 임시 HTML 파일, 외부 Markdown 라이브러리는 필요하지 않다.

## 확인 항목

1. Qt 6 MSVC 2022 64-bit 구성에서 qmake와 Debug 빌드가 성공하는지 확인한다.
2. `example/test.pdf`를 열고 별도 `Convert` 조작 없이 변환이 시작되는지 확인한다.
3. `Converting...`과 `Rendering preview...` 동안 창을 이동하고 반복 클릭해 응답성을 확인한다.
4. 변환 후 제목, 표, 목록이 오른쪽 미리보기에 표시되는지 확인한다.
5. 왼쪽 편집기의 Markdown을 수정하고 오른쪽 미리보기가 갱신되는지 확인한다.
6. 최초 변환 결과 주입 직후에는 창 제목에 `*`가 없고, 사용자 편집 뒤에는 `*`가 표시되는지 확인한다.
