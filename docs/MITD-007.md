# MITD-007 — Markdown Save / Save As

## 구현 범위

- 변환과 초기 미리보기 렌더링이 끝난 Markdown을 File 메뉴와 툴바의 `Save`,
  `Save As`로 저장한다.
- `Save As`의 기본 경로는 원본 문서와 같은 폴더의 `<원본명>.md`이다.
- 저장 파일은 BOM 없는 UTF-8로 기록한다.
- 저장 성공 후 `Document.markdownFilePath`를 실제 절대 경로로 유지하고
  `Document.modified`와 편집기의 modified 플래그를 해제한다.
- 저장 성공 상태는 `Saved: <파일명>`으로 표시한다.
- 저장 파일을 열거나 쓰거나 확정하지 못하면 오류 대화상자를 표시하고 문서 상태와
  기존 파일을 보존한다.

수정 문서를 둔 채 다른 문서를 열거나 프로그램을 종료할 때의 확인 절차 등
MITD-007 이후 티켓의 기능은 이 구현에 포함하지 않는다.

## 저장 흐름

```text
Save
  │
  ├── markdownFilePath 있음 ──► 해당 경로에 UTF-8 저장
  │
  └── markdownFilePath 없음 ──► Save As
                                  │
                                  ├── 취소 ──► 변경 없음
                                  └── 경로 선택 ──► UTF-8 저장
```

`Save As`에서 확장자를 입력하지 않으면 `.md`를 붙인다. 한 번 저장한 뒤 `Save`를
다시 실행하면 저장 대화상자 없이 `Document.markdownFilePath`에 덮어쓴다.

## 파일 안정성과 오류 처리

저장은 `QSaveFile`을 사용한다. Markdown 전체를 임시 파일에 기록한 뒤 commit이
성공할 때만 대상 파일을 교체하므로 쓰기 중 실패했을 때 기존 파일이 부분적으로
덮어써지는 것을 방지한다. 열기, 쓰기, commit 중 하나라도 실패하면 `Save Failed`
대화상자에 대상 경로와 Qt가 제공한 오류 내용을 표시하며 modified 플래그를
해제하지 않는다.

## 관련 파일

- 저장 액션과 화면 연결: `mainwindow.ui`
- Save/Save As 및 UTF-8 파일 기록: `mainwindow.h`, `mainwindow.cpp`
- 저장 경로와 수정 상태: `src/model/Document.h`

새 소스 파일이나 Qt 모듈은 추가하지 않는다.

## 확인 항목

1. Qt 6 MSVC 2022 64-bit 구성에서 qmake와 Debug 빌드가 성공하는지 확인한다.
2. `example/test.pdf`를 변환하고 `Save`를 누르면 기본 경로가
   `example/test.md`로 제안되는지 확인한다.
3. 한글과 비 ASCII 문자가 포함된 Markdown을 저장하고 UTF-8로 다시 읽히는지
   확인한다.
4. 저장 성공 후 창 제목의 `*`가 사라지고 상태 표시줄에 `Saved: test.md`가
   표시되는지 확인한다.
5. 편집한 뒤 `Save`를 다시 누르면 대화상자 없이 기존 Markdown 파일이 갱신되는지
   확인한다.
6. `Save As`로 다른 파일을 선택한 뒤 다음 `Save`가 새 경로를 사용하는지 확인한다.
7. 쓸 수 없는 경로로 저장을 시도했을 때 오류 대화상자가 나타나고 modified 상태가
   유지되는지 확인한다.
