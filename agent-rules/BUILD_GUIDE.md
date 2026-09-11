# MarkItDown Desktop 개발 및 빌드 가이드

## 1. 목적과 적용 순서

이 문서는 개발 에이전트가 저장소의 실제 도구 체인으로 프로젝트를 빌드하고 검증하기 위한 작업 규칙이다.

작업을 시작할 때 다음 순서로 문서를 읽는다.

1. `agent-rules/PROJECT_GOAL.md`: 제품 목표, 범위, 아키텍처의 최우선 기준
2. `agent-rules/BUILD_GUIDE.md`: 현재 개발, 빌드, 실행, 검증 절차의 기준
3. 작업과 관련된 `ticket/` 및 `docs/` 문서

설계서의 예시와 현재 저장소가 다르면 다음 원칙을 적용한다.

- 제품 방향과 제약은 `PROJECT_GOAL.md`를 따른다.
- 현재 빌드 방식은 저장소 루트의 `MarkItDown_Desktop.pro`를 따른다.
- 현재 저장소에는 `CMakeLists.txt`가 없다. `PROJECT_GOAL.md`의 CMake 내용은 향후 구조 예시이며, CMake 전환 작업이 별도로 이루어지기 전에는 CMake로 빌드했다고 가정하지 않는다.
- `.qtcreator/*.pro.user`, `build/`, `Makefile*`, `ui_*.h`, `moc_*.cpp`는 사용자별 설정 또는 생성물이다. 소스의 기준으로 삼거나 직접 수정하지 않는다.

## 2. 현재 프로젝트 구성

| 항목 | 현재 기준 |
| --- | --- |
| 대상 플랫폼 | Windows 10/11 64-bit |
| 애플리케이션 종류 | Qt Widgets 데스크톱 GUI |
| 빌드 시스템 | qmake (`MarkItDown_Desktop.pro`) |
| C++ 표준 | C++17 |
| 권장 Qt 키트 | Qt 6.x, MSVC 2022 64-bit |
| Qt 모듈 | Core, Gui, Widgets |
| UI 컴파일 | `uic`가 `mainwindow.ui`에서 `ui_mainwindow.h` 생성 |
| 실행 파일 | `MarkItDown_Desktop.exe` |
| 변환 백엔드 | Python 3.10 이상 + Microsoft MarkItDown CLI (`QProcess` 비동기 실행) |

현재 `.pro` 파일에 등록된 입력은 다음과 같다.

- 소스: `main.cpp`, `mainwindow.cpp`, `src/controller/DocumentController.cpp`, `src/markitdown/MarkItDownManager.cpp`, `src/rendering/MarkdownDocumentRenderer.cpp`
- 헤더: `mainwindow.h`, `src/controller/DocumentController.h`, `src/model/ConversionError.h`, `src/model/Document.h`, `src/markitdown/MarkItDownManager.h`, `src/rendering/MarkdownDocumentRenderer.h`
- 폼: `mainwindow.ui`

Windows 빌드는 추가로 `scripts/install_markitdown_backend.ps1`와
`requirements-markitdown.txt`를 사용해 실행 파일 폴더의 `python-venv`에
MarkItDown 백엔드를 준비한다. 이 두 파일은 `.pro`의 `DISTFILES`에 등록되어 있다.

새 C++/헤더/UI/리소스 파일을 추가하면 반드시 `.pro` 파일의 `SOURCES`, `HEADERS`, `FORMS`, `RESOURCES` 중 해당 항목에도 등록한다.

현재 `MainWindow`, `DocumentController`, `MarkItDownManager`의 비동기 변환 흐름은 연결되어 있지만 자동화 테스트는 아직 없다. 따라서 빌드 성공만으로 UI 변환 기능까지 검증했다고 표현해서는 안 된다.

## 3. 권장 개발 환경

필수 구성은 다음과 같다.

1. Windows 10 또는 Windows 11 x64
2. Visual Studio 2022 또는 Visual Studio Build Tools 2022
   - `Desktop development with C++` 워크로드
   - MSVC v143 x64/x86 build tools
   - Windows SDK
3. Qt 6.x의 `MSVC 2022 64-bit` 구성
   - Qt Widgets 포함
   - Qt Creator는 선택 사항이지만 GUI 작업 시 권장
4. Git
5. 변환 기능 작업 시 Python 3.10 이상
   - 재현성과 의존성 호환성을 위해 Python 3.12 가상 환경을 권장
   - 시스템 전역 Python에 패키지를 설치하지 않는다.

Qt와 컴파일러 ABI 및 아키텍처를 혼합하지 않는다. 예를 들어 `Qt 6 MSVC 2022 64-bit`는 MSVC 2022 x64 도구 체인으로 빌드한다. 기존 `Qt 5.15.2 MSVC2019 64-bit` 빌드 폴더는 과거 호환 산출물이며 프로젝트의 권장 기준이 아니다.

## 4. 이 워크스테이션에서 확인된 환경

2026-09-11에 실제 파일과 명령으로 확인한 로컬 상태는 다음과 같다. 이 절은 재현에 도움을 주는 스냅샷이며 다른 PC에서 경로를 하드코딩할 근거가 아니다.

| 도구 | 확인된 값 |
| --- | --- |
| Qt | 6.11.0, `C:\Qt\6.11.0\msvc2022_64` |
| qmake | 3.1, Qt 6.11.0 사용 |
| Visual Studio | Visual Studio/Build Tools 2022 17.14 |
| MSVC | 19.44, x64 |
| Git | 2.55.0.windows.3 |
| Python 런처 | 3.12, 3.13, 3.14 발견; 기본값은 3.14.7 |
| MarkItDown CLI | PATH에서는 발견되지 않음. Debug 빌드 폴더의 앱 로컬 환경에 0.1.7 설치 확인 |

Qt 6.11.0 + MSVC 2022 x64 Debug 빌드는 이 문서의 절차로 성공했으며, 확인된 출력은 다음 경로였다.

```text
build\Agent_Qt_6_11_0_MSVC2022_64bit-Debug\debug\MarkItDown_Desktop.exe
```

이 경로는 검증 기록일 뿐 고정 출력 경로가 아니다.

## 5. 에이전트 사전 점검

저장소 루트의 PowerShell에서 먼저 실제 상태를 확인한다.

```powershell
$repoRoot = (Resolve-Path -LiteralPath '.').Path
$projectFile = Join-Path $repoRoot 'MarkItDown_Desktop.pro'

if (-not (Test-Path -LiteralPath $projectFile)) {
    throw 'MarkItDown_Desktop.pro not found'
}

git status --short
py -0p
Get-Command cl, nmake -ErrorAction SilentlyContinue
```

Qt 설치 경로는 설치된 버전에 맞게 정한 뒤 검증한다.

```powershell
$qtRoot = 'C:\Qt\6.11.0\msvc2022_64'
$qmake = Join-Path $qtRoot 'bin\qmake.exe'

if (-not (Test-Path -LiteralPath $qmake)) {
    throw "Qt qmake not found: $qmake"
}

& $qmake -v
```

`cl.exe`가 PATH에 보이는 것만으로 MSVC 환경 준비가 끝난 것은 아니다. 다음 값도 확인한다.

```powershell
if (-not $env:INCLUDE -or -not $env:LIB) {
    throw 'MSVC developer environment is not initialized'
}
```

`INCLUDE` 또는 `LIB`가 비어 있으면 일반 PowerShell 대신 `Developer PowerShell for VS 2022`를 열거나 다음 절의 초기화를 수행한다. 이 점검 없이 빌드하면 Qt 헤더를 읽는 중 `type_traits: No such file or directory` 같은 오류가 날 수 있다.

작업 트리에 기존 변경이 있으면 사용자 변경으로 간주하고 보존한다. 빌드 전후 `git status --short`를 비교해 생성물이 소스 변경으로 섞이지 않았는지 확인한다.

## 6. MSVC 개발자 환경 초기화

가장 간단한 방법은 시작 메뉴의 `Developer PowerShell for VS 2022`에서 작업하는 것이다.

일반 PowerShell 세션을 자동으로 준비해야 하면 다음을 사용한다.

```powershell
$vswhere = Join-Path ${env:ProgramFiles(x86)} `
    'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'vswhere.exe not found'
}

$env:Path = "$(Split-Path -Parent $vswhere);$env:Path"

$vsInstall = & $vswhere `
    -latest `
    -products '*' `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath |
    Select-Object -First 1

if (-not $vsInstall) {
    throw 'Visual Studio 2022 C++ toolchain not found'
}

$devShell = Join-Path $vsInstall 'Common7\Tools\Launch-VsDevShell.ps1'
& $devShell -Arch amd64 -HostArch amd64 -SkipAutomaticLocation

if (-not $env:INCLUDE -or -not $env:LIB) {
    throw 'Failed to initialize the MSVC developer environment'
}
```

초기화 후 `cl`, `nmake`, `link`가 같은 MSVC 2022 x64 환경을 가리키는지 확인한다.

## 7. 명령줄 빌드

항상 저장소 밖이 아니라 저장소의 무시된 `build/` 하위에서 shadow build를 수행한다. 소스 루트에 Makefile이나 오브젝트 파일을 생성하지 않는다.

### 7.1 Debug

MSVC 개발자 환경을 초기화한 동일한 PowerShell 세션에서 실행한다.

```powershell
$repoRoot = (Resolve-Path -LiteralPath '.').Path
$qtRoot = 'C:\Qt\6.11.0\msvc2022_64'
$qmake = Join-Path $qtRoot 'bin\qmake.exe'
$projectFile = Join-Path $repoRoot 'MarkItDown_Desktop.pro'
$buildDir = Join-Path $repoRoot `
    'build\Desktop_Qt_6_MSVC2022_64bit-Debug'

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
Push-Location $buildDir
try {
    & $qmake $projectFile -spec win32-msvc
    if ($LASTEXITCODE -ne 0) {
        throw "qmake failed: $LASTEXITCODE"
    }

    nmake /NOLOGO debug
    if ($LASTEXITCODE -ne 0) {
        throw "nmake failed: $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
```

예상 출력:

```text
build\Desktop_Qt_6_MSVC2022_64bit-Debug\debug\MarkItDown_Desktop.exe
```

### 7.2 Release

Debug와 분리된 빌드 디렉터리를 사용하고 `release` 대상을 빌드한다.

```powershell
$repoRoot = (Resolve-Path -LiteralPath '.').Path
$qtRoot = 'C:\Qt\6.11.0\msvc2022_64'
$qmake = Join-Path $qtRoot 'bin\qmake.exe'
$projectFile = Join-Path $repoRoot 'MarkItDown_Desktop.pro'
$buildDir = Join-Path $repoRoot `
    'build\Desktop_Qt_6_MSVC2022_64bit-Release'

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
Push-Location $buildDir
try {
    & $qmake $projectFile -spec win32-msvc
    if ($LASTEXITCODE -ne 0) {
        throw "qmake failed: $LASTEXITCODE"
    }

    nmake /NOLOGO release
    if ($LASTEXITCODE -ne 0) {
        throw "nmake failed: $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
```

예상 출력:

```text
build\Desktop_Qt_6_MSVC2022_64bit-Release\release\MarkItDown_Desktop.exe
```

증분 빌드는 해당 빌드 디렉터리에서 `nmake /NOLOGO debug` 또는 `nmake /NOLOGO release`를 다시 실행한다. `.pro` 변경 후에는 qmake를 먼저 다시 실행한다.

정리가 필요하면 대상이 의도한 `build/<kit>-<configuration>` 폴더인지 절대 경로로 확인한 후 그 폴더 안에서 `nmake /NOLOGO clean`을 사용한다. 사용자 소스나 저장소 전체를 대상으로 재귀 삭제하지 않는다.

## 8. Qt Creator 빌드

1. Qt Creator에서 `MarkItDown_Desktop.pro`를 연다.
2. `Desktop Qt 6.x MSVC2022 64bit` 키트를 선택한다.
3. Build directory를 저장소의 `build/` 아래 shadow-build 경로로 지정한다.
4. Debug 또는 Release 구성을 선택해 `Run qmake`, `Build` 순서로 실행한다.

`.qtcreator/MarkItDown_Desktop.pro.user`는 사용자별 절대 경로와 키트 ID를 포함하고 `.gitignore` 대상이다. 이 파일을 공유 설정이나 다른 PC의 기준으로 사용하지 않는다.

Qt Creator가 Qt 5 키트를 자동 선택하더라도 프로젝트 목표의 기준은 Qt 6이다. Qt 5 호환성을 별도로 요구하는 티켓이 없다면 Qt 6 빌드로 검증한다.

## 9. Python 및 MarkItDown 개발 환경

### 9.1 빌드 결과 폴더 자동 설치

Windows에서 qmake가 실행 파일 링크를 마치면 `QMAKE_POST_LINK`가
`scripts/install_markitdown_backend.ps1`를 실행한다. 설치 대상은 현재 구성의
실행 파일과 같은 폴더 아래 `python-venv`이다.

```text
debug\
├── MarkItDown_Desktop.exe
└── python-venv\
    └── Scripts\markitdown.exe
```

설치 스크립트는 Python 3.12, Python Launcher의 기본 Python 3, `python`,
`python3` 순서로 Python 3.10 이상을 찾는다. 앱 로컬 가상환경이 없으면 생성하고,
`requirements-markitdown.txt`의 고정 버전을 설치한 뒤 `markitdown --help`가
성공하는지 확인한다. Python을 찾지 못하거나 패키지 설치 또는 CLI 검증이
실패하면 빌드도 실패한다.

현재 고정된 직접 의존성은 다음과 같다.

```text
markitdown[all]==0.1.7
```

첫 빌드에는 Python 3.10 이상과 패키지 인덱스에 접근할 네트워크가 필요하다.
이후 링크에서는 기존 가상환경을 재사용하며 pip가 요구사항을 다시 확인한다.

### 9.2 수동 개발 환경

MarkItDown 백엔드 작업 전에는 Python 런처로 사용 가능한 버전을 확인한다.

```powershell
py -0p
```

저장소에서 현재 확인된 Python 3.12를 사용해 `build/` 아래에 격리 환경을 만드는 예시는 다음과 같다. 가상 환경을 활성화하지 않고 실행 파일을 직접 호출하므로 에이전트 세션 간 차이가 줄어든다.

```powershell
$repoRoot = (Resolve-Path -LiteralPath '.').Path
$venvDir = Join-Path $repoRoot 'build\python-venv'

py -3.12 -m venv $venvDir
$python = Join-Path $venvDir 'Scripts\python.exe'
$markitdown = Join-Path $venvDir 'Scripts\markitdown.exe'

& $python -m pip install --upgrade pip
& $python -m pip install 'markitdown[all]'
& $markitdown --help
```

전체 형식 지원이 필요하지 않은 작업에서는 필요한 extra만 설치할 수 있다. 그러나 프로젝트의 목표가 MarkItDown 지원 형식을 폭넓게 제공하는 것이므로 기본 개발 환경은 `[all]`을 기준으로 한다.

`MarkItDownManager`는 `QProcess`로 `markitdown <입력 파일>`을 비동기 실행한다. CLI는 `MARKITDOWN_EXECUTABLE` 환경 변수, 실행 파일과 같은 폴더의 `python-venv`, 실행 환경의 `PATH`, 실행 파일 또는 현재 작업 디렉터리 상위에 있는 개발용 `python-venv`/`build\python-venv` 순서로 찾는다. 개발 및 후속 연동 작업에서는 다음을 지킨다.

- 전역 PATH에 MarkItDown이 있다고 가정하지 않는다.
- 빌드 결과에 준비된 앱 로컬 CLI를 PATH보다 우선해 빌드에서 검증한 버전을 사용한다.
- 기본 개발용 `build\python-venv`는 앱이 자동으로 찾는다. 다른 위치의 환경은 `MARKITDOWN_EXECUTABLE`로 CLI 절대 경로를 지정하거나 해당 `Scripts` 디렉터리를 `PATH` 앞에 추가한다.
- 입력 파일과 출력은 한 번에 하나만 처리한다.
- 변환 프로세스를 동기 대기해 UI 스레드를 막지 않는다.
- 사용자 입력 경로를 셸 문자열로 결합하지 않고 `QProcess` 프로그램과 인자 목록으로 분리한다.

개발 실행 세션에서 PATH를 준비하는 예시는 다음과 같다.

```powershell
$repoRoot = (Resolve-Path -LiteralPath '.').Path
$qtRoot = 'C:\Qt\6.11.0\msvc2022_64'
$venvScripts = Join-Path $repoRoot 'build\python-venv\Scripts'
$env:Path = "$venvScripts;$(Join-Path $qtRoot 'bin');$env:Path"
```

`requirements-markitdown.txt`의 직접 의존성 버전을 변경하면 실제 설치, 대표 입력 변환 및 이 문서를 같은 작업에서 갱신한다.

## 10. 실행과 검증

개발 빌드는 Qt DLL을 찾을 수 있도록 Qt `bin`을 PATH에 둔 같은 세션에서 실행한다.

```powershell
$qtRoot = 'C:\Qt\6.11.0\msvc2022_64'
$env:Path = "$(Join-Path $qtRoot 'bin');$env:Path"
& '.\build\Desktop_Qt_6_MSVC2022_64bit-Debug\debug\MarkItDown_Desktop.exe'
```

현재 자동화 테스트가 없으므로 구현 작업 후 최소 검증은 다음과 같다.

1. qmake 성공
2. 해당 구성의 nmake 성공
3. 실행 파일과 같은 폴더의 `python-venv\Scripts\markitdown.exe` 생성 및 `--help` 성공 확인
4. GUI를 실행해 작업 티켓의 수동 확인 항목 점검
   - 큰 문서는 `Converting...`과 `Rendering preview...` 단계 모두에서 창 이동과 클릭에 응답하는지 확인한다.
   - 미리보기의 비동기 로드가 끝난 뒤에만 상태가 `Converted`로 바뀌고 `Open`, `Convert`, `Save`가 다시 활성화되는지 확인한다.
5. 종료 후 `git status --short`로 예상한 소스와 문서만 변경되었는지 확인

실행 파일이 생성되었다는 사실만으로 GUI 동작을 확인했다고 주장하지 않는다. GUI를 실제로 실행하지 못한 환경이면 빌드 검증만 완료했다고 명시한다. 변환 기능은 MarkItDown CLI와 대표 입력 파일을 실제로 실행한 경우에만 검증 완료로 기록한다.

## 11. Windows 배포 준비

일반 사용자 PC에서 실행할 Release 폴더를 만들 때는 Qt의 `windeployqt`를 사용한다.

```powershell
$repoRoot = (Resolve-Path -LiteralPath '.').Path
$qtRoot = 'C:\Qt\6.11.0\msvc2022_64'
$releaseExe = Join-Path $repoRoot `
    'build\Desktop_Qt_6_MSVC2022_64bit-Release\release\MarkItDown_Desktop.exe'
$stageDir = Join-Path $repoRoot 'build\deploy\MarkItDown_Desktop'

New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
Copy-Item -LiteralPath $releaseExe -Destination $stageDir

$stagedExe = Join-Path $stageDir 'MarkItDown_Desktop.exe'
& (Join-Path $qtRoot 'bin\windeployqt.exe') `
    --release `
    --dir $stageDir `
    $stagedExe
```

`windeployqt`는 Qt 의존성만 수집한다. 배포 스테이징 폴더에도 앱 로컬 백엔드를
준비하려면 최종 스테이징 경로를 대상으로 설치 스크립트를 다시 실행한다.

```powershell
& (Join-Path $repoRoot 'scripts\install_markitdown_backend.ps1') `
    -Destination (Join-Path $stageDir 'python-venv') `
    -RequirementsFile (Join-Path $repoRoot 'requirements-markitdown.txt')
```

Python 표준 `venv`는 이동 가능한 독립 Python 배포물이 아니다. 따라서 이 자동
설치는 빌드 및 동일 PC의 스테이징 결과를 위한 것이며, Python이 없는 일반 사용자
PC로 폴더를 복사하는 독립 배포를 의미하지 않는다. 독립 배포에는 Python 런타임을
포함하는 별도 패키징 작업이 필요하다.

## 12. 자주 발생하는 문제

| 증상 | 원인과 조치 |
| --- | --- |
| `qmake`를 찾을 수 없음 | qmake가 전역 PATH에 없을 수 있다. 선택한 Qt 키트의 `bin\qmake.exe`를 절대 경로로 호출한다. |
| `type_traits` 등 표준 헤더를 찾을 수 없음 | `cl.exe` 경로만 있고 MSVC `INCLUDE`/`LIB`가 없다. VS 2022 개발자 셸을 초기화한다. |
| 링크 시 machine type 또는 라이브러리 불일치 | Qt 키트와 MSVC의 버전/아키텍처를 맞춘다. Qt 6 MSVC2022 x64와 x64 컴파일러를 사용한다. |
| `Qt6Widgets.dll` 또는 `qwindows.dll` 누락 | 개발 시 Qt `bin`을 PATH에 추가한다. 배포 시 `windeployqt`를 실행한다. |
| UI 변경이 반영되지 않음 | 소스의 `mainwindow.ui`를 수정했는지 확인하고 qmake/nmake를 다시 실행한다. 생성된 `ui_mainwindow.h`는 수정하지 않는다. |
| 새 소스가 컴파일되지 않음 | 파일을 `.pro`의 해당 목록에 등록한 후 qmake를 다시 실행한다. |
| 빌드 중 앱 로컬 MarkItDown 설치 실패 | Python 3.10 이상과 네트워크 연결을 확인하고, 출력된 pip 오류를 해결한 뒤 다시 링크한다. 설치 대상은 실행 파일 폴더의 `python-venv`이다. |
| 실행 시 `markitdown`을 찾을 수 없음 | 실행 파일 옆 `python-venv\Scripts\markitdown.exe`가 존재하는지 확인한다. 없으면 qmake 후 다시 빌드하거나 설치 스크립트를 해당 실행 파일 폴더에 직접 실행한다. 다른 위치를 사용하려면 `MARKITDOWN_EXECUTABLE`에 CLI 절대 경로를 지정한다. |
| Qt 5 빌드가 선택됨 | 별도 호환성 요구가 없다면 Qt 6 MSVC2022 64-bit 키트와 별도 빌드 폴더로 다시 구성한다. |

## 13. 변경 시 문서화 규칙

빌드나 런타임 구성을 바꾸는 구현은 같은 작업에서 다음 문서를 함께 갱신한다.

- `agent-rules/BUILD_GUIDE.md`: 에이전트 절차, 필수 도구, 명령, 검증 기준
- `docs/`: 해당 구현 티켓 또는 기능 문서
- 필요 시 `MarkItDown_Desktop.pro`: 새 소스, 모듈, 리소스, 컴파일 옵션

Qt 버전, Python 버전 또는 패키지 버전을 단순히 로컬 설치 상태만 보고 프로젝트 요구사항으로 올리지 않는다. 요구사항 변경은 실제 구성 파일 및 구현과 함께 이루어져야 한다.

## 14. 공식 참고 자료

- Qt qmake 실행: <https://doc.qt.io/qt-6/qmake-running.html>
- Qt Windows 배포: <https://doc.qt.io/qt-6/windows-deployment.html>
- Microsoft MarkItDown 설치 및 CLI: <https://github.com/microsoft/markitdown#readme>
- MSVC 명령줄 빌드 환경: <https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line>
