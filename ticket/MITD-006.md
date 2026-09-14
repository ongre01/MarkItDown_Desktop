
# MITD-006 — Markdown Preview 구현

**Priority:** High
**Phase:** MVP
**Type:** Feature
**Dependency:** MITD-005

## 목적

변환된 Markdown을 렌더링해서 보여준다.

## 구성

왼쪽:

```text
QPlainTextEdit
```

오른쪽:

```text
QTextBrowser
```

## 변환 성공 시

```cpp
editor->setPlainText(markdown);

preview->setMarkdown(markdown);
```

## Editor 수정 시

Preview도 갱신한다.

```text
Editor textChanged
       ↓
Preview Update
```

동시에:

```cpp
Document.modified = true;
```

## 주의사항

* 프로그램이 변환 결과를 처음 Editor에 넣을 때는 사용자 수정으로 처리하지 않는다.
* `Converting`중에는 Preview를 갱신하지 않는다.


## 제외 범위

* QWebEngineView
* 외부 Markdown Library

## 테스트 조건
* `example/test.pdf`로 테스트 한다.
* `Converting...` 및 `Rendering preview...` 중 창을 반복해서 클릭하거나 이동한다.
* 창이 `응답 없음` 상태로 바뀌지 않는지 확인한다.

## 완료 조건

* Markdown Preview 정상 표시
* Heading 표시
* Table 표시
* List 표시
* 사용자 편집 시 Preview 갱신
