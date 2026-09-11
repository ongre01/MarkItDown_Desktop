# MITD-003 — 단일 파일 Open 기능

* Priority: High
* Phase: MVP
* Type: Feature
* Dependency: MITD-001, MITD-002

## 목적

사용자가 문서 하나를 선택할 수 있도록 한다.

## 사용 API
```
QFileDialog::getOpenFileName()
```

## 지원 확장자
```
pdf
docx
pptx
xlsx
xls
html
htm
csv
json
xml
txt
```

## 처리 흐름
```
Open
 ↓
File Dialog
 ↓
파일 선택
 ↓
Document.sourceFilePath
 ↓
Document.status = Ready
 ↓
Window Title / StatusBar 갱신
```

## 요구사항

파일 Open 시 아직 파일 내용은 읽지 않는다.

Window Title 예:

MarkItDown Viewer - MC33774.pdf

StatusBar:

MC33774.pdf | Ready

## 제외 범위
* 자동 변환
* MarkItDown 실행
* Multi Select

## 완료 조건
* 파일 하나 선택 가능
* Cancel 정상 처리
* Document에 경로 저장
* Window Title 변경
* StatusBar 변경
