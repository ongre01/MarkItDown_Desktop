# MITD-002 — 단일 Document 모델 구현

* Priority: High
* Phase: MVP
* Type: Feature
* Dependency: MITD-001

## 목적
현재 열려 있는 하나의 문서 상태를 관리한다.

## 구현 파일
```
src/model/Document.h
```

## 데이터 구조
```
enum class DocumentStatus
{
    Empty,
    Ready,
    Converting,
    Completed,
    Failed
};

struct Document
{
    QString sourceFilePath;
    QString markdown;
    QString markdownFilePath;

    bool modified = false;

    DocumentStatus status =
        DocumentStatus::Empty;
};
```
## 요구사항

MainWindow는 Document 객체 하나만 가진다.

## 제외 범위

다음 형태의 복수 문서 Collection을 만들지 않는다.
```
QList<Document>
QVector<Document>
std::vector<Document>

```
## 완료 조건
* Document 모델 추가
* MainWindow와 연결
* 기존 UI 동작 유지
* Build 성공
