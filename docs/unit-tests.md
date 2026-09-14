# MarkItDown Desktop 자동 테스트 가이드

## 테스트 구조

현재 프로젝트의 빌드 기준은 CMake가 아니라 qmake이다. `tests/tests.pro`가 세 카테고리
프로젝트를 ordered subdirs로 집계하고, 각 카테고리 프로젝트가 독립 Qt Test 실행 파일을
등록한다.

```text
tests/
├── tests.pro
├── run-tests.ps1
├── unit/
│   ├── unit.pro
│   ├── unit_test.pri
│   ├── tst_DocumentFileOperations.cpp/.pro
│   ├── tst_MarkdownRenderState.cpp/.pro
│   ├── tst_ConversionErrorPresentation.cpp/.pro
│   └── tst_MarkdownDocumentRenderer.cpp/.pro
├── component/
│   ├── component.pro
│   ├── component_test.pri
│   ├── tst_MarkItDownManager.cpp/.pro
│   └── tst_DocumentController.cpp/.pro
└── ui/
    ├── ui.pro
    ├── ui_test.pri
    └── tst_MainWindow.cpp/.pro
```

각 실행 파일의 `.pro`에는 `CONFIG += testcase`가 적용된다. 따라서 카테고리 빌드
디렉터리와 전체 테스트 빌드 루트에서 qmake 기본 `check` target도 사용할 수 있다.

## 카테고리와 suite 목적

| Category | Test executable | 목적 |
| --- | --- | --- |
| unit | `tst_DocumentFileOperations` | source validation, Markdown 경로 및 UTF-8 저장의 성공/실패 분기 |
| unit | `tst_MarkdownRenderState` | request lifecycle, initial render readiness 및 stale result 거부 |
| unit | `tst_ConversionErrorPresentation` | 모든 `ConversionError`의 사용자 표시 mapping |
| unit | `tst_MarkdownDocumentRenderer` | Markdown heading, emphasis, list, link, code, quote, Unicode 및 큰 문서 렌더링 의미 |
| component | `tst_MarkItDownManager` | process lifecycle, stdout/stderr, 모든 변환 오류 및 terminal signal 중복 방지 |
| component | `tst_DocumentController` | manager 요청 위임, 실행 상태 및 성공/실패 signal 중계 |
| ui | `tst_MainWindow` | Open → Convert → Editor/Preview → Save와 control/status/stale render 동작 |

Filesystem test는 `QTemporaryDir`/`QTemporaryFile`을 사용한다. 비동기 검증은 signal과
event loop 조건을 사용하며 고정 sleep을 사용하지 않는다.

## Fake와 controlled dependency

```text
MarkItDownManager
  ├── FakeProcessRunner
  └── FakeExecutableResolver

DocumentController
  └── FakeMarkItDownManager

MainWindow
  ├── DocumentController → FakeMarkItDownManager
  ├── ControlledMarkdownRenderer
  └── FakeMainWindowDialogs
```

Component/UI test는 실제 MarkItDown process, Python 환경, native file dialog, modal message
box 및 network를 사용하지 않는다. production 기본 생성자는 실제 adapter를 사용하고,
테스트 생성자에만 동일한 공개 interface의 Fake/controlled 구현을 주입한다.

## Configure 및 build

Qt 6 MSVC 2022 x64 Developer PowerShell에서 실행한다. MSVC 초기화와 Qt 설치 확인의 전체
절차는 `agent-rules/BUILD_GUIDE.md`를 따른다.

```powershell
$repoRoot = (Resolve-Path -LiteralPath '.').Path
$qtRoot = 'C:\Qt\6.11.0\msvc2022_64'
$qmake = Join-Path $qtRoot 'bin\qmake.exe'
$testBuildDir = Join-Path $repoRoot `
    'build\Desktop_Qt_6_MSVC2022_64bit-UnitTests'

New-Item -ItemType Directory -Force -Path $testBuildDir | Out-Null
$env:Path = "$(Join-Path $qtRoot 'bin');$env:Path"

Push-Location $testBuildDir
try {
    & $qmake (Join-Path $repoRoot 'tests\tests.pro') -spec win32-msvc
    if ($LASTEXITCODE -ne 0) { throw 'Test qmake failed' }

    nmake /NOLOGO debug
    if ($LASTEXITCODE -ne 0) { throw 'Test build failed' }
}
finally {
    Pop-Location
}
```

Application normal build는 별도 shadow-build 디렉터리에서
`MarkItDown_Desktop.pro`를 qmake한 뒤 `nmake /NOLOGO debug`로 수행한다. 정확한 Debug/
Release 명령과 앱 로컬 backend 준비 절차는 `BUILD_GUIDE.md` 7절을 따른다.

## 실행

표준 실행기는 `tests/run-tests.ps1`이다. `BuildDirectory`는 `tests/tests.pro`를 configure/
build한 루트이며 상대 경로와 절대 경로를 모두 허용한다.

```powershell
$runner = Join-Path $repoRoot 'tests\run-tests.ps1'

& $runner -BuildDirectory $testBuildDir -Category unit
& $runner -BuildDirectory $testBuildDir -Category component
& $runner -BuildDirectory $testBuildDir -Category ui
& $runner -BuildDirectory $testBuildDir -Category all
```

실행기는 선택한 모든 target을 실행한다. 각 Qt Test text log를 콘솔에 그대로 출력하므로
suite/test case, source 위치, `Actual`/`Expected` 및 Qt diagnostic을 확인할 수 있다.
로그는 고유한 OS 임시 파일로 만들고 출력 후 삭제한다. 실패한 target이 있어도 선택된
나머지 target을 실행한 뒤 실패 목록을 표시하고 0이 아닌 종료 코드를 반환한다.

qmake 기본 실행도 지원한다.

```powershell
# Unit/Component/UI 중 하나만 실행
Push-Location (Join-Path $testBuildDir 'unit')
nmake /NOLOGO check
Pop-Location

# 전체 실행
Push-Location $testBuildDir
nmake /NOLOGO check
Pop-Location
```

자동화 로그의 일관된 Qt Test 상세 출력이 필요할 때는 `run-tests.ps1`을 사용한다.

## 표준 full verification 순서

1. 기존 산출물을 재사용하지 않는 test/application shadow-build 디렉터리를 준비한다.
2. `tests/tests.pro`를 configure하고 모든 test executable을 build한다.
3. `MarkItDown_Desktop.pro`를 configure하고 application을 build한다.
4. `run-tests.ps1 -Category unit`을 실행한다.
5. `run-tests.ps1 -Category component`를 실행한다.
6. `run-tests.ps1 -Category ui`를 실행한다.
7. `run-tests.ps1 -Category all`을 실행한다.
8. test build 루트의 `nmake /NOLOGO check`를 실행해 qmake aggregate 진입점도 확인한다.
9. 앱 로컬 `python-venv\Scripts\markitdown.exe --help`와 `git status --short`를 확인한다.

실제 실행하지 않은 단계는 PASS로 기록하지 않는다. 실행 파일 생성만으로 interactive GUI나
실제 문서 변환을 검증했다고 간주하지 않는다.

## CTest 및 coverage 정책

저장소에는 `CMakeLists.txt`가 없고 현재 build source of truth는
`MarkItDown_Desktop.pro`이므로 CTest를 추가하지 않았다. CMake 전환 티켓이 구현되기
전까지 qmake `testcase`/`check`와 카테고리 subdirs가 테스트 등록 기준이다.

기존 `.pro`, `.pri`, PowerShell build script에는 coverage tool 설정이 없다. UT-007에서는
새 coverage infrastructure를 도입하지 않았으며 coverage percentage를 acceptance criterion으로
사용하지 않는다.

## 알려진 제한

- 모든 process-dependent test는 Fake를 사용한다. 실제 문서의 MarkItDown 변환은 별도
  integration/manual verification 대상이다.
- `tst_MainWindow`는 controlled dependency 기반이다. native dialog와 interactive GUI의
  실제 화면 동작을 대체하지 않는다.
- Windows 임시 파일 시스템에서 이식성 있게 만들기 어려운 `SourceDocumentError::NotFile`,
  disk-full 및 `QSaveFile::commit()` 운영체제 장애는 자동화 범위에서 제외한다.
- Qt 6.11 Debug renderer test에는 `QFont::setPixelSize` warning이 출력되지만 assertion과
  종료 코드는 성공한다.
- test target을 추가하거나 이름을 바꾸면 해당 카테고리 `.pro`와
  `tests/run-tests.ps1`의 target 목록을 함께 갱신해야 한다.

## 최근 검증 결과

2026-09-14에 Qt 6.11.0 / MSVC 2022 x64 Debug 환경에서 기존 산출물을 사용하지 않는
`build/UT007-TestSuite`와 `build/UT007-App-Debug`로 검증했다.

| 단계 | 결과 |
| --- | --- |
| test qmake configure 및 7개 executable build | PASS |
| application qmake configure 및 Debug build | PASS |
| unit category | 73 passed, 0 failed, 0 skipped |
| component category | 35 passed, 0 failed, 0 skipped |
| ui category | 10 passed, 0 failed, 0 skipped |
| all category | 118 passed, 0 failed, 0 skipped |
| test build root `nmake /NOLOGO check` | PASS, exit 0 |
| app local `markitdown.exe --help` | PASS, exit 0 |

첫 clean test build 호출은 자동화 명령의 60초 제한을 초과해 성공으로 기록하지 않았다.
그 시점에 7개 실행 파일이 모두 생성된 것을 확인한 뒤 같은 shadow-build에서
`nmake /NOLOGO debug`를 다시 실행했고 종료 코드 0을 확인했다. Application fresh build는
새 shadow-build에서 종료 코드 0으로 완료됐다.

이번 검증에서는 interactive GUI smoke test와 실제 입력 문서 변환을 실행하지 않았다.
