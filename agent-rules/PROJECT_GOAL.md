# MarkItDown GUI Application 설계서

## 1. 개요

### 1.1 프로젝트명

**MarkItDown Desktop**

### 1.2 개발 목적

PDF, Word, Excel, PowerPoint등 MarkItDown가 지원하는 모든 문서를 사용자가 하나씩 선택하면 Microsoft MarkItDown을 이용하여 Markdown으로 변환하고, 변환 결과를 확인·편집·저장할 수 있는 Windows 데스크톱 애플리케이션을 개발한다.

본 애플리케이션은 **한 번에 하나의 문서만 처리한다.**

복수 파일 Batch 변환, 변환 Queue, 여러 문서 동시 처리는 지원하지 않는다.

향후 Markdown 결과를 기반으로 다음 기능으로 확장할 수 있도록 설계한다.

* 문서 검색
* 문서 구조 분석
* LLM 질의응답
* 문서 요약
* 프로토콜 문서 분석
* 레지스터 및 메시지 검색
* RAG 데이터 생성

---

# 2. 개발 환경

## 2.1 운영체제

Windows 10 / Windows 11 64-bit

## 2.2 개발 언어

UI/Application:

```text
C++17 이상
```

문서 변환:

```text
Python 3.10 이상
```

## 2.3 Framework

```text
Qt 6.x
Qt Widgets
```

사용 Qt Module:

```text
QtCore
QtGui
QtWidgets
```

필요 시:

```text
QtWebEngineWidgets
```

를 추가한다.

## 2.4 문서 변환 엔진

Microsoft MarkItDown을 문서 변환 Backend로 사용한다.

지원 대상 예:

```text
PDF
DOCX
PPTX
XLSX
XLS
HTML
CSV
JSON
XML
TXT
EPUB
Image
```

---

# 3. 핵심 설계 원칙

프로그램은 **Single Document Model**을 사용한다.

즉 프로그램 실행 중 현재 처리 대상 문서는 항상 하나이다.

```text
Current Document
      │
      ├── File Path
      ├── File Name
      ├── Markdown
      ├── Modified
      └── Conversion Status
```

새로운 파일을 열면 기존 문서는 닫히고 새로운 문서가 Current Document가 된다.

---

# 4. 시스템 구조

```text
┌──────────────────────────────────────────────┐
│                Qt Application                │
│                                              │
│ ┌──────────────────────────────────────────┐ │
│ │               MainWindow                 │ │
│ │                                          │ │
│ │ File Open                                │ │
│ │ Markdown Editor                          │ │
│ │ Markdown Preview                         │ │
│ │ Search                                   │ │
│ │ Save                                     │ │
│ └──────────────────┬───────────────────────┘ │
│                    │                         │
│                    ▼                         │
│ ┌──────────────────────────────────────────┐ │
│ │          DocumentController              │ │
│ └──────────────────┬───────────────────────┘ │
│                    │                         │
│                    ▼                         │
│ ┌──────────────────────────────────────────┐ │
│ │          MarkItDownManager               │ │
│ │                                          │ │
│ │              QProcess                    │ │
│ └──────────────────┬───────────────────────┘ │
└────────────────────┼─────────────────────────┘
                     │
                     ▼
             Python / MarkItDown
                     │
                     ▼
               Markdown Text
```

---

# 5. 파일 처리 흐름

```text
사용자 파일 선택
       │
       ▼
현재 문서 저장 여부 확인
       │
       ▼
파일 존재 여부 검사
       │
       ▼
지원 확장자 검사
       │
       ▼
Current Document 설정
       │
       ▼
MarkItDown 실행
       │
       ▼
Markdown 변환
       │
       ├── 실패 → 오류 표시
       │
       ▼
Markdown Editor 표시
       │
       ▼
Markdown Preview 표시
```

---

# 6. 단일 파일 처리 정책

프로그램은 하나의 파일만 처리한다.

예:

```text
document.pdf
     │
     ▼
MarkItDown
     │
     ▼
document.md
```

사용자가 다른 파일을 열면:

```text
document.pdf
     │
     X

protocol.docx
     │
     ▼
Current Document
```

기존 문서에 수정된 내용이 있다면 새 파일을 열기 전에 저장 여부를 확인한다.

```text
현재 문서가 수정되었습니다.

[저장]
[저장하지 않음]
[취소]
```

---

# 7. UI 설계

복수 문서 목록은 필요하지 않다.

따라서 UI를 다음처럼 단순화한다.

```text
┌────────────────────────────────────────────────────────────┐
│ File   Edit   View   Tools   Help                          │
├────────────────────────────────────────────────────────────┤
│ [Open] [Convert] [Save]           [Search              ]   │
├──────────────────────────┬─────────────────────────────────┤
│                          │                                 │
│ Markdown Editor          │ Markdown Preview                │
│                          │                                 │
│ # BMS Protocol           │ BMS Protocol                    │
│                          │                                 │
│ ## Message               │ Message                         │
│                          │                                 │
│ | ID | Name |            │ ID      Name                    │
│ |----|------|            │ 0x100   Status                  │
│                          │                                 │
├──────────────────────────┴─────────────────────────────────┤
│ document.pdf                     Converted | UTF-8         │
└────────────────────────────────────────────────────────────┘
```

---

# 8. Toolbar

기본 Toolbar는 다음만 제공한다.

```text
Open
Convert
Save
Save As
Search
```

필요 시:

```text
Reload
Copy
Open Folder
```

등을 추가한다.

---

# 9. 파일 열기

사용 Widget:

```cpp
QFileDialog
```

예:

```cpp
QString filePath =
    QFileDialog::getOpenFileName(
        this,
        tr("Open Document"),
        QString(),
        tr("Documents (*.pdf *.docx *.pptx *.xlsx *.xls *.html *.csv *.json *.xml *.txt);;All Files (*.*)")
    );
```

한 번에 하나의 파일만 선택한다.

`getOpenFileNames()`가 아니라 `getOpenFileName()`을 사용한다.

---

# 10. Drag & Drop

Drag & Drop도 한 파일만 허용한다.

사용자가 여러 파일을 동시에 Drop한 경우 첫 번째 파일만 처리하거나 오류 메시지를 표시한다.

권장 방식은 여러 파일 Drop을 거부하는 것이다.

```text
한 번에 하나의 파일만 열 수 있습니다.
```

처리 흐름:

```text
Windows Explorer
       │
       │ document.pdf
       ▼
MainWindow
       │
       ▼
Open Document
```

---

# 11. MainWindow

MainWindow는 전체 UI를 담당한다.

주요 기능:

```text
파일 열기
Drag & Drop
변환 요청
Markdown 표시
Preview
검색
저장
상태 표시
```

예상 인터페이스:

```cpp
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:

    void openFile();

    void convertFile();

    void saveMarkdown();

    void saveMarkdownAs();

    void onConversionStarted();

    void onConversionFinished(
        const QString &markdown);

    void onConversionFailed(
        const QString &error);

private:

    bool checkModifiedDocument();

    void updateWindowTitle();

private:

    DocumentController *m_controller;

    Document m_document;
};
```

---

# 12. Document

현재 하나의 문서 상태만 관리한다.

```cpp
struct Document
{
    QString sourceFilePath;

    QString markdown;

    bool modified = false;

    DocumentStatus status =
        DocumentStatus::Empty;
};
```

상태:

```cpp
enum class DocumentStatus
{
    Empty,

    Ready,

    Converting,

    Completed,

    Failed
};
```

복수 문서를 관리할 필요가 없기 때문에 List나 Vector 형태의 Document Collection은 사용하지 않는다.

---

# 13. DocumentController

UI와 MarkItDown Backend 사이를 중계한다.

```text
MainWindow
     │
     ▼
DocumentController
     │
     ▼
MarkItDownManager
```

예:

```cpp
class DocumentController : public QObject
{
    Q_OBJECT

public:

    explicit DocumentController(
        QObject *parent = nullptr);

    void convert(
        const QString &filePath);

    bool isConverting() const;

signals:

    void conversionStarted();

    void conversionFinished(
        const QString &markdown);

    void conversionFailed(
        const QString &error);

private:

    MarkItDownManager *m_markitdown;
};
```

---

# 14. MarkItDownManager

MarkItDown 실행을 담당한다.

Qt의:

```cpp
QProcess
```

를 사용한다.

```text
Input File
    │
    ▼
QProcess
    │
    ▼
MarkItDown
    │
    ▼
stdout
    │
    ▼
QString Markdown
```

---

# 15. MarkItDownManager 클래스

```cpp
class MarkItDownManager : public QObject
{
    Q_OBJECT

public:

    explicit MarkItDownManager(
        QObject *parent = nullptr);

    void convert(
        const QString &filePath);

    bool isRunning() const;

signals:

    void started();

    void finished(
        const QString &markdown);

    void failed(
        const QString &message);

private slots:

    void processFinished(
        int exitCode,
        QProcess::ExitStatus status);

    void processError(
        QProcess::ProcessError error);

private:

    QProcess *m_process;
};
```

---

# 16. 변환 동시 실행 제한

MarkItDownManager는 동시에 하나의 QProcess만 실행한다.

```cpp
void MarkItDownManager::convert(
    const QString &filePath)
{
    if (m_process->state() != QProcess::NotRunning)
    {
        emit failed(
            tr("Another conversion is already running."));

        return;
    }

    // Conversion
}
```

즉 다음 구조는 지원하지 않는다.

```text
file1.pdf ─┐
file2.pdf ─┼─ 동시에 변환
file3.pdf ─┘
```

항상:

```text
file1.pdf
   │
   ▼
변환 완료
```

후 다음 파일을 열어야 한다.

---

# 17. MarkItDown 실행

기본 CLI:

```text
markitdown document.pdf
```

Qt에서는:

```cpp
void MarkItDownManager::convert(
    const QString &filePath)
{
    QString program = "markitdown";

    QStringList arguments;

    arguments << filePath;

    m_process->start(
        program,
        arguments);
}
```

---

# 18. 변환 결과

변환이 완료되면 stdout을 읽는다.

```cpp
void MarkItDownManager::processFinished(
    int exitCode,
    QProcess::ExitStatus exitStatus)
{
    if (exitStatus != QProcess::NormalExit ||
        exitCode != 0)
    {
        const QString error =
            QString::fromUtf8(
                m_process->readAllStandardError());

        emit failed(error);

        return;
    }

    const QString markdown =
        QString::fromUtf8(
            m_process->readAllStandardOutput());

    emit finished(markdown);
}
```

---

# 19. 비동기 처리

UI Thread에서는 다음과 같은 코드를 사용하지 않는다.

```cpp
process.waitForFinished();
```

대신 QProcess Signal을 사용한다.

```cpp
connect(
    m_process,
    &QProcess::finished,
    this,
    &MarkItDownManager::processFinished);
```

변환 중에도 UI가 멈추지 않도록 한다.

---

# 20. 변환 중 UI

변환 중에는 Convert/Open 버튼을 일시적으로 비활성화할 수 있다.

예:

```text
Converting document.pdf...
```

UI:

```text
[Open]       Disabled
[Convert]    Disabled
[Save]       Disabled

Converting...
```

완료 후:

```text
[Open]       Enabled
[Convert]    Enabled
[Save]       Enabled
```

---

# 21. Markdown Editor

사용 Widget:

```cpp
QPlainTextEdit
```

역할:

```text
Markdown 표시
Markdown 편집
검색
복사
저장
```

편집 내용이 변경되면:

```cpp
Document::modified = true;
```

로 설정한다.

---

# 22. Markdown Preview

간단한 구현에서는:

```cpp
QTextBrowser
```

를 사용한다.

Markdown:

```cpp
m_preview->setMarkdown(markdown);
```

복잡한 CSS/HTML Preview가 필요해지면:

```text
Markdown
   │
   ▼
HTML
   │
   ▼
QWebEngineView
```

로 변경한다.

---

# 23. Splitter

Editor와 Preview 사이에는:

```cpp
QSplitter
```

를 사용하는 것이 좋다.

```text
┌──────────────────────────┬─────────────────────────┐
│                          │                         │
│ Markdown Editor          │ Preview                 │
│                          │                         │
│                          │                         │
└──────────────────────────┴─────────────────────────┘
                ▲
                │
             QSplitter
```

사용자가 좌우 영역 크기를 조절할 수 있다.

---

# 24. Markdown 저장

기본 출력 파일명:

```text
document.pdf

      ↓

document.md
```

Save:

```text
원본 파일명.md
```

Save As:

사용자가 직접 위치와 파일명을 선택한다.

---

# 25. 수정된 문서 관리

Markdown을 사용자가 수정했다면:

```text
document.pdf *
```

처럼 Window Title에 표시할 수 있다.

새 파일을 열거나 프로그램을 종료할 때:

```text
Markdown 내용이 수정되었습니다.
저장하시겠습니까?

[Save]
[Discard]
[Cancel]
```

을 표시한다.

---

# 26. 검색

Editor에서:

```text
Ctrl + F
```

검색을 지원한다.

예:

```text
MC33774
0x684
SYS_COM_CFG
VC14
```

현재 열린 하나의 문서에서만 검색한다.

프로젝트 전체 검색이나 여러 문서 검색은 MVP 범위에 포함하지 않는다.

---

# 27. 상태 표시

StatusBar:

```text
document.pdf | Converted | UTF-8 | Markdown
```

변환 중:

```text
document.pdf | Converting...
```

실패:

```text
document.pdf | Conversion Failed
```

---

# 28. 오류 처리

## 파일 관련

```text
File not found
Permission denied
Unsupported format
File locked
```

## Backend

```text
Python not found
MarkItDown not installed
MarkItDown execution failed
```

## Conversion

```text
Empty output
Conversion error
Invalid document
```

---

# 29. 프로그램 실행 구조

```text
MarkItDownViewer.exe
        │
        ▼
MainWindow
        │
        ▼
Open File
        │
        ▼
DocumentController
        │
        ▼
MarkItDownManager
        │
        ▼
QProcess
        │
        ▼
MarkItDown
        │
        ▼
Markdown
        │
        ├─────────────► Editor
        │
        └─────────────► Preview
```

---

# 30. Sequence Diagram

```text
User
 │
 │ Open File
 ▼
MainWindow
 │
 │ convert(file)
 ▼
DocumentController
 │
 │ convert(file)
 ▼
MarkItDownManager
 │
 │ QProcess.start()
 ▼
MarkItDown
 │
 │ Markdown stdout
 ▼
MarkItDownManager
 │
 │ finished(markdown)
 ▼
DocumentController
 │
 │ conversionFinished()
 ▼
MainWindow
 │
 ├── Document 업데이트
 ├── Editor 업데이트
 └── Preview 업데이트
```

---

# 31. 프로젝트 구조

```text
MarkItDownViewer/
│
├── CMakeLists.txt
│
└── src/
    │
    ├── main.cpp
    │
    ├── ui/
    │   ├── MainWindow.cpp
    │   └── MainWindow.h
    │
    ├── controller/
    │   ├── DocumentController.cpp
    │   └── DocumentController.h
    │
    ├── markitdown/
    │   ├── MarkItDownManager.cpp
    │   └── MarkItDownManager.h
    │
    └── model/
        └── Document.h
```

기존 설계에 있었던 별도의 Document List Model은 필요하지 않다.

---

# 32. CMake

```cmake
cmake_minimum_required(VERSION 3.21)

project(MarkItDownViewer)

set(CMAKE_CXX_STANDARD 17)

find_package(
    Qt6 REQUIRED
    COMPONENTS
    Core
    Gui
    Widgets
)

qt_add_executable(
    MarkItDownViewer

    src/main.cpp

    src/ui/MainWindow.cpp

    src/controller/DocumentController.cpp

    src/markitdown/MarkItDownManager.cpp
)

target_link_libraries(
    MarkItDownViewer

    PRIVATE

    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
```

---

# 33. MVP 범위

1차 버전에서는 다음 기능을 구현한다.

```text
단일 파일 Open
단일 파일 Drag & Drop
MarkItDown 실행
Markdown Editor
Markdown Preview
Markdown Save
Search
Error Handling
```

다음 기능은 구현하지 않는다.

```text
Batch Conversion
Multi File Conversion
Conversion Queue
Multi Document Tab
Document Library
Project Library
Multi Document Search
```

---

# 34. UI 최종 형태

MVP UI는 다음 구조를 권장한다.

```text
┌─────────────────────────────────────────────────────────────┐
│ MarkItDown Viewer - document.pdf                            │
├─────────────────────────────────────────────────────────────┤
│ File   Edit   View   Tools   Help                           │
├─────────────────────────────────────────────────────────────┤
│ [Open] [Convert] [Save] [Save As]      Search [          ] │
├──────────────────────────────┬──────────────────────────────┤
│                              │                              │
│ Markdown                     │ Preview                      │
│                              │                              │
│ # BMS Protocol               │ BMS Protocol                 │
│                              │                              │
│ ## CAN Message               │ CAN Message                  │
│                              │                              │
│ | ID | Name |                │ ID       Name                │
│ |----|------|                │ 0x684    RESPONSE            │
│                              │                              │
│                              │                              │
├──────────────────────────────┴──────────────────────────────┤
│ document.pdf                        Converted | UTF-8       │
└─────────────────────────────────────────────────────────────┘
```

---

# 35. 향후 확장

단일 파일 기반 구조는 유지하면서 분석 기능을 추가할 수 있다.

```text
Document
    │
    ▼
MarkItDown
    │
    ▼
Markdown
    │
    ├────► Search
    │
    ├────► Structure Parser
    │
    ├────► Protocol Analyzer
    │
    └────► LLM
```

예를 들어 하나의 Datasheet를 열고:

```text
MC33774.pdf
     │
     ▼
MarkItDown
     │
     ▼
Markdown
     │
     ▼
Firmware Document Analyzer
```

형태로 사용할 수 있다.

---

# 36. 설계 방향 요약

본 프로그램은 문서 관리 프로그램이 아니라 **현재 열려 있는 하나의 문서를 Markdown으로 변환하고 분석하는 Desktop Tool**을 목표로 한다.

따라서 핵심 구조는 다음과 같이 단순화한다.

```text
File
 │
 ▼
MarkItDown
 │
 ▼
Markdown
 │
 ├─ Editor
 ├─ Preview
 ├─ Search
 └─ Analyzer
```

Batch 처리나 여러 문서 상태 관리는 제외하고, 하나의 Current Document만 관리함으로써 프로그램 구조와 상태 관리를 단순하게 유지한다.
