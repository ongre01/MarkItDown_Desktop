# MITD-007 — Markdown Save / Save As 구현

**Priority:** High
**Phase:** MVP
**Type:** Feature
**Dependency:** MITD-006

## 목적

변환 또는 수정한 Markdown을 파일로 저장한다.

## 기본 파일 이름

```text
test.pdf
    ↓
test.md
```

## Save As

사용:

```cpp
QFileDialog::getSaveFileName()
```

## 파일 인코딩

```text
UTF-8
```

## Save

`markdownFilePath`가 존재하면 해당 경로에 저장한다.

없으면 Save As 수행.

## 성공 후

```cpp
Document.modified = false;
```

StatusBar:

```text
Saved: test.md
```

## 완료 조건

* `.md` 저장 가능
* UTF-8 정상 저장
* Save 지원
* Save As 지원
* 저장 오류 처리