# MITD-001 — MainWindow 기본 UI 구성

* Priority: High
* Phase: MVP
* Type: Feature

## 목적
MarkItDown Viewer의 기본 화면을 구성한다.

## UI 구성
┌─────────────────────────────────────────────┐
│ File  Edit  View  Help                      │
├─────────────────────────────────────────────┤
│ [Open] [Convert] [Save]                     │
├─────────────────────┬───────────────────────┤
│ Markdown Editor     │ Markdown Preview      │
│                     │                       │
├─────────────────────┴───────────────────────┤
│ Ready                                       │
└─────────────────────────────────────────────┘

## 요구사항

사용 Widget:
```
QMenuBar
QToolBar
QSplitter
QPlainTextEdit
QTextBrowser
QStatusBar
```
Toolbar Action:
```
Open
Convert
Save
```
초기 StatusBar:
```
Ready
```
## 제외 범위
* 버튼의 실제 기능은 구현하지 않는다.

## 완료 조건
* UI가 정상 표시
* Splitter 크기 조정 가능
* 모든 Action 표시
* 프로젝트 빌드 성공