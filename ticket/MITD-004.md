# MITD-004 — MarkItDownManager 구현

* Priority: Critical
* Phase: MVP
* Type: Feature
* Dependency: MITD-001

## 목적

Qt UI와 MarkItDown CLI 실행을 분리한다.

## 구현 파일
```
src/markitdown/MarkItDownManager.h
src/markitdown/MarkItDownManager.cpp
```

## 클래스 인터페이스
```
class MarkItDownManager : public QObject
{
    Q_OBJECT

public:
    explicit MarkItDownManager(
        QObject *parent = nullptr);

    void convert(const QString &filePath);

    bool isRunning() const;

signals:
    void started();

    void finished(
        const QString &markdown);

    void failed(
        const QString &message);

private:
    QProcess *m_process;
};
```

## 실행
```
markitdown "<input file>"
```

## 요구사항

QProcess를 비동기로 사용한다.
```
사용:

started
finished
errorOccurred
readyReadStandardError

금지:

waitForFinished()
waitForStarted()

정상 종료 조건:

NormalExit
AND
exitCode == 0

stdout:

Markdown

stderr:

오류 정보
```

## 완료 조건
* MarkItDown CLI 실행 가능
* UI Thread Blocking 없음
* stdout 반환 가능
* 오류 Signal 전달 가능
* 동시 실행 차단