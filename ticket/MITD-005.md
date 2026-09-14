# MITD-005 — Convert 기능 연결

* Priority: Critical
* Phase: MVP
* Type: Feature
* Dependency: MITD-002, MITD-003, MITD-004

## 목적
* Open된 문서를 MarkItDown으로 실제 변환한다.


## 처리 흐름
```
Open File
    ↓
Convert
    ↓
MarkItDownManager::convert()
    ↓
MarkItDown
    ↓
Markdown
    ↓
Document.markdown
    ↓
Markdown Editor
```

## 변환 시작

```
Document.status =
    DocumentStatus::Converting;
```

UI:

Open     Disabled
Convert  Disabled
Save     Disabled

StatusBar:

Converting...


## 변환 성공
```
Document.markdown = markdown;

Document.status =
    DocumentStatus::Completed;
```

Markdown Editor에 결과 표시.

## 변환 실패
```
Document.status =
    DocumentStatus::Failed;
```
QMessageBox 표시
StatusBar 오류 표시

## Test Files
* 테스트 파일은 `example/` 디렉토리를 참고한다.


## 완료 조건
* PDF 또는 지원 파일 하나를 실제 변환 가능


