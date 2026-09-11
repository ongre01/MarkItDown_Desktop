# MITD-004 — MarkItDownManager 구현

## 구현 내용

- `MarkItDownManager`가 자식 `QProcess` 하나를 소유하고 `markitdown <입력 파일>` 형식으로 CLI를 실행한다.
- 프로그램과 인자를 분리해 `QProcess::start()`에 전달하므로 공백이 포함된 파일 경로도 하나의 인자로 처리된다.
- `QProcess`의 `started`, `finished`, `errorOccurred`, `readyReadStandardError` 신호를 사용하며 동기 대기 함수는 사용하지 않는다.
- 프로세스가 실제 시작되면 `started()`를 전달하고, 정상 종료(`NormalExit`이면서 종료 코드 0) 시 UTF-8 stdout을 `finished(markdown)`로 반환한다.
- stderr는 실행 중 비동기적으로 누적한다. 시작 오류나 비정상 종료 시 누적된 stderr를 `failed(message)`로 전달하고, stderr가 없으면 `QProcess` 오류 또는 종료 상태를 설명하는 메시지를 전달한다.
- 하나의 변환이 시작 중이거나 실행 중이면 추가 요청을 시작하지 않고 `Another conversion is already running.` 실패 신호를 보낸다.
- 동일한 프로세스 오류에 대해 `errorOccurred`와 `finished`가 모두 발생해도 실패 신호를 중복 전달하지 않는다.

## MarkItDown CLI 탐색

`MarkItDownManager`는 다음 순서로 실행 파일을 찾는다.

1. `MARKITDOWN_EXECUTABLE` 환경 변수로 지정한 파일 또는 명령
2. 실행 환경의 `PATH`에 등록된 `markitdown`
3. 애플리케이션 디렉터리와 현재 작업 디렉터리의 상위 경로에서 발견되는 개발용 `python-venv` 또는 `build/python-venv`

어느 경로에서도 찾지 못하면 설치, `PATH`, 환경 변수 설정 방법을 포함한 오류를 반환한다. 세 번째 단계는 저장소의 격리 환경을 Qt Creator 또는 빌드 출력 디렉터리에서 바로 사용할 수 있게 하는 개발 편의 기능이며 배포 패키징을 대신하지 않는다.

이 티켓은 CLI 실행 계층만 구현한다. `MainWindow`의 Convert 액션과 문서 상태를 이 클래스에 연결하는 작업은 컨트롤러/UI 통합 티켓의 범위다.

## 확인 방법

1. qmake를 다시 실행해 새 소스와 헤더가 프로젝트에 등록되는지 확인한다.
2. Qt 6 MSVC 2022 64-bit Debug 구성을 빌드한다.
3. `markitdown`을 찾을 수 있는 실행 환경에서 입력 파일을 변환해 `started()`와 `finished(markdown)`가 순서대로 전달되는지 확인한다.
4. 존재하지 않는 CLI 또는 변환에 실패하는 입력으로 `failed(message)`가 전달되는지 확인한다.
5. 실행 중 `convert()`를 다시 호출해 두 번째 프로세스가 시작되지 않는지 확인한다.
