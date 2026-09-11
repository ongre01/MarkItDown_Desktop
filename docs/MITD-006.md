# MITD-006 — 빌드 결과 폴더에 MarkItDown CLI 자동 설치

## 구현 내용

- Windows qmake 빌드의 링크 후처리에서 `scripts/install_markitdown_backend.ps1`를 실행한다.
- Debug와 Release 각각 `MarkItDown_Desktop.exe`가 생성되는 폴더 아래에 `python-venv`를 만들고 MarkItDown CLI를 설치한다.
- `requirements-markitdown.txt`에서 `markitdown[all]==0.1.7`을 고정해 직접 의존성 버전을 명시한다.
- 설치 스크립트는 Python 3.12를 우선 사용하고, 없으면 사용 가능한 Python 3.10 이상을 찾는다.
- 가상환경 생성, pip 설치 또는 `markitdown --help` 검증이 실패하면 빌드를 실패시켜 CLI가 없는 불완전한 산출물이 성공한 것처럼 남지 않게 한다.
- 기존 앱 로컬 환경은 다시 사용하므로 증분 링크마다 전체 환경을 새로 만들지 않는다.

## 런타임 탐색 순서

1. `MARKITDOWN_EXECUTABLE`로 명시한 실행 파일 또는 명령
2. `MarkItDown_Desktop.exe` 옆 `python-venv\Scripts\markitdown.exe`
3. 실행 환경의 `PATH`에 있는 `markitdown`
4. 상위 디렉터리에서 발견되는 수동 개발용 `python-venv` 또는 `build\python-venv`

환경 변수는 명시적 운영자 설정이므로 가장 우선한다. 자동 설치한 앱 로컬 CLI는 PATH보다 우선해 빌드에서 준비한 고정 버전을 사용한다.

## 빌드 결과

```text
<build>\debug\
├── MarkItDown_Desktop.exe
└── python-venv\
    └── Scripts\
        └── markitdown.exe
```

Release 구성에서는 같은 구조가 `<build>\release` 아래 생성된다.

## 요구사항과 제한

- 최초 설치 시 Python 3.10 이상과 Python 패키지 인덱스에 접근할 네트워크가 필요하다.
- Python 표준 가상환경은 이동 가능한 독립 Python 배포물이 아니다. 이 기능은 빌드 결과 폴더와 동일 PC의 스테이징을 위한 것이며, Python 런타임까지 포함하는 독립 배포는 별도 패키징 범위다.
- 백엔드 설치 크기와 시간은 `markitdown[all]`의 전체 형식 지원 의존성에 따라 증가한다.

## 확인 방법

1. qmake를 다시 실행한다.
2. Qt 6 MSVC 2022 64-bit Debug 또는 Release를 빌드한다.
3. 실행 파일 옆 `python-venv\Scripts\markitdown.exe`가 존재하는지 확인한다.
4. 앱 로컬 CLI의 `--help`가 성공하는지 확인한다.
5. 앱 로컬 CLI로 `example/test.docx`를 변환해 Markdown stdout이 생성되는지 확인한다.
6. 전역 PATH에 `markitdown`이 없는 상태로 GUI에서 같은 파일을 변환해 기존 `MarkItDown CLI was not found` 오류가 발생하지 않는지 확인한다.
