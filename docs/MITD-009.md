# MITD-009 — Error Handling 정리

## 구현 범위

- 원본 파일 없음과 지원하지 않는 확장자를 서로 다른 제목과 안내문으로 표시한다.
- 파일을 연 뒤 원본이 삭제된 경우에도 변환 직전에 다시 확인하고 MarkItDown을
  실행하지 않는다.
- `MarkItDownManager`가 실행 파일 없음, 프로세스 시작 실패, 비정상 종료,
  non-zero 종료 코드, 빈 변환 출력 및 그 밖의 프로세스 오류를
  `ConversionError`로 구분한다.
- `DocumentController`는 오류 종류와 기술 세부 정보를 UI에 그대로 중계한다.
- `MainWindow`는 오류 종류별 사용자 안내문을 만들고, 제공된 프로세스 오류와
  `stderr`는 `Technical details` 영역에 함께 표시한다.
- Markdown 저장의 open, write, commit 실패는 저장 대상 경로 및 파일 시스템 오류와
  함께 `Markdown Save Failed`로 표시한다.

## 역할 분리

```text
MarkItDownManager
  ├── 실행 파일 탐색 및 QProcess 오류 분류
  ├── 종료 코드와 빈 stdout 검사
  └── QProcess 오류 문자열 및 stderr 수집
                │
                ▼
DocumentController
  └── ConversionError + 기술 세부 정보 중계
                │
                ▼
MainWindow
  ├── 오류별 사용자 메시지와 제목 선택
  ├── 상태 표시줄 및 QMessageBox 갱신
  └── 원본 파일·확장자·저장 오류 처리
```

## 변환 오류 분류

| `ConversionError` | 사용자에게 구분되는 상황 | 보존되는 기술 정보 |
| --- | --- | --- |
| `ExecutableNotFound` | MarkItDown 실행 파일 없음 | 설정된 `MARKITDOWN_EXECUTABLE` 값 |
| `FailedToStart` | `QProcess`가 프로그램을 시작하지 못함 | `QProcess::errorString()`, `stderr` |
| `Crashed` | 실행 중 프로세스 비정상 종료 | `QProcess::errorString()`, `stderr` |
| `NonZeroExit` | 정상 종료했지만 종료 코드가 0이 아님 | 종료 코드, `stderr` |
| `EmptyOutput` | 종료 코드는 0이지만 stdout이 비어 있음 | `stderr` |
| `ProcessFailure` | 기타 프로세스 read/write 오류 | `QProcess::errorString()`, `stderr` |

`AlreadyRunning`은 중복 변환 요청에 대한 방어 분류다. 단일 문서 UI는 변환 중 관련
액션을 비활성화하므로 정상 사용자 흐름에서는 발생하지 않는다.

## 실패 후 상태

- 변환 오류 시 현재 문서는 `Failed`가 되며 `Open`과 `Convert`를 다시 사용할 수 있다.
- 빈 stdout은 완료된 Markdown으로 처리하지 않는다.
- 저장 오류 시 현재 Markdown과 수정 상태를 유지하므로 사용자가 다른 경로로 다시
  저장할 수 있다.
- 어떤 오류 경로에서도 애플리케이션을 종료하거나 동기 대기하지 않는다.

## 확인 항목

1. 존재하지 않는 경로를 열거나, 파일을 연 뒤 삭제하고 Convert를 눌렀을 때
   `Source File Not Found`가 표시되는지 확인한다.
2. 지원하지 않는 확장자의 파일을 열거나 드롭했을 때 `Unsupported File Type`과 해당
   확장자가 표시되는지 확인한다.
3. `MARKITDOWN_EXECUTABLE`을 존재하지 않는 경로로 설정한 뒤 변환하면
   `MarkItDown Not Found`가 표시되는지 확인한다.
4. 존재하지만 실행할 수 없는 파일을 `MARKITDOWN_EXECUTABLE`로 지정하면
   `MarkItDown Start Failed`와 `QProcess` 오류가 표시되는지 확인한다.
5. non-zero 종료 코드와 `stderr`를 반환하는 대체 실행 파일을 지정하면
   `Conversion Command Failed`, 종료 코드 및 `stderr`가 모두 표시되는지 확인한다.
6. 종료 코드 0과 빈 stdout을 반환하는 대체 실행 파일을 지정하면
   `Empty Conversion Output`이 표시되는지 확인한다.
7. 쓰기 권한이 없는 위치에 저장하면 `Markdown Save Failed`, 저장 경로 및 파일 시스템
   오류가 표시되고 현재 Markdown이 유지되는지 확인한다.
8. 각 오류 메시지를 닫은 뒤 `Open` 또는 `Convert`로 작업을 계속할 수 있는지 확인한다.
