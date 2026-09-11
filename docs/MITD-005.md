# MITD-005 — Convert 기능 연결

## 구현 내용

- `DocumentController`를 추가해 `MainWindow`와 `MarkItDownManager` 사이의 변환 요청 및 결과 신호를 중계한다.
- `Convert` 액션을 현재 문서의 원본 경로에 대한 비동기 변환 요청과 연결했다.
- 변환 요청 시 `Document.status`를 `Converting`으로 변경하고 `Open`, `Convert`, `Save` 액션을 비활성화한다.
- 변환 성공 시 stdout Markdown을 `Document.markdown`에 저장하고 상태를 `Completed`로 변경한 뒤 편집기와 미리보기에 표시한다.
- 변환 실패 시 상태를 `Failed`로 변경하고 상태 표시줄과 오류 메시지 상자에 원인을 표시한다.
- 빈 문서, 변환 준비, 변환 중, 완료, 실패 상태에 맞춰 창 제목, 상태 표시줄 및 액션 활성화 상태를 일관되게 갱신한다.
- 새 파일을 열 때 이전 Markdown 편집기와 미리보기 내용을 지운다.

## 상태별 UI

| 문서 상태 | Open | Convert | Save | 상태 표시줄 |
| --- | --- | --- | --- | --- |
| `Empty` | 활성 | 비활성 | 비활성 | `Ready` |
| `Ready` | 활성 | 활성 | 비활성 | `<파일명> \| Ready` |
| `Converting` | 비활성 | 비활성 | 비활성 | `<파일명> \| Converting...` |
| `Completed` | 활성 | 활성 | 활성 | `<파일명> \| Converted \| UTF-8 \| Markdown` |
| `Failed` | 활성 | 활성 | 비활성 | `<파일명> \| Conversion Failed: <오류>` |

## 런타임 전제

앱은 `MARKITDOWN_EXECUTABLE` 환경 변수, `PATH`, 저장소의 `build\python-venv` 순서로 `markitdown` CLI를 찾는다. 따라서 가이드대로 생성한 개발용 가상환경은 Qt Creator 또는 빌드 출력 디렉터리에서 앱을 실행할 때 별도의 `PATH` 수정 없이 사용할 수 있다.

## 확인 방법

1. qmake를 다시 실행하고 Qt 6 MSVC 2022 64-bit Debug 구성을 빌드한다.
2. `markitdown`을 찾을 수 있는 환경에서 `example/`의 지원 파일을 연다.
3. `Convert`를 누르고 변환 중 세 액션이 비활성화되며 상태 표시줄에 `Converting...`이 표시되는지 확인한다.
4. 완료 후 Markdown이 편집기와 미리보기에 나타나고 `Save`가 활성화되는지 확인한다.
5. CLI를 찾을 수 없는 환경에서 변환해 오류 메시지 상자와 상태 표시줄이 실패 원인을 표시하고 `Open`과 `Convert`가 다시 활성화되는지 확인한다.
