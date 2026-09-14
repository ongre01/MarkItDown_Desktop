#include "mainwindow.h"
#include "src/controller/DocumentController.h"
#include "src/markitdown/IMarkItDownManager.h"
#include "src/rendering/MarkdownDocumentRenderer.h"
#include "src/ui/IMainWindowDialogs.h"

#include <QAction>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QSignalSpy>
#include <QStatusBar>
#include <QStringList>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QtTest>

#include <memory>
#include <utility>

namespace {

class FakeMarkItDownManager final : public IMarkItDownManager
{
public:
    void convert(const QString &filePath) override
    {
        m_running = true;
        m_sourcePaths.append(filePath);
        emit started();
    }

    bool isRunning() const override
    {
        return m_running;
    }

    void reportFinished(const QString &markdown)
    {
        m_running = false;
        emit finished(markdown);
    }

    void reportFailed(ConversionError error, const QString &details)
    {
        m_running = false;
        emit failed(error, details);
    }

    int convertCallCount() const
    {
        return m_sourcePaths.size();
    }

    QString sourcePath(int index) const
    {
        return m_sourcePaths.at(index);
    }

private:
    bool m_running = false;
    QStringList m_sourcePaths;
};

class ControlledMarkdownRenderer final : public MarkdownDocumentRenderer
{
    Q_OBJECT

public slots:
    void render(quint64 requestId, const QString &markdown) override
    {
        Q_UNUSED(requestId);
        Q_UNUSED(markdown);
    }

    void reportRendered(quint64 requestId, const QString &html)
    {
        emit rendered(requestId, html);
    }
};

class FakeMainWindowDialogs final : public IMainWindowDialogs
{
public:
    struct Message
    {
        QString title;
        QString text;
    };

    QString selectSourceDocument(QWidget *parent,
                                 const QString &filter) override
    {
        Q_UNUSED(parent);
        ++sourceSelectionCount;
        sourceFilters.append(filter);
        return nextSourcePath;
    }

    QString selectMarkdownDestination(
        QWidget *parent,
        const QString &suggestedFilePath) override
    {
        Q_UNUSED(parent);
        ++saveSelectionCount;
        saveSuggestions.append(suggestedFilePath);
        return nextSavePath;
    }

    void showWarning(QWidget *parent,
                     const QString &title,
                     const QString &message) override
    {
        Q_UNUSED(parent);
        warnings.append(Message{title, message});
    }

    void showCritical(QWidget *parent,
                      const QString &title,
                      const QString &message) override
    {
        Q_UNUSED(parent);
        criticals.append(Message{title, message});
    }

    QString nextSourcePath;
    QString nextSavePath;
    int sourceSelectionCount = 0;
    int saveSelectionCount = 0;
    QStringList sourceFilters;
    QStringList saveSuggestions;
    QList<Message> warnings;
    QList<Message> criticals;
};

class WindowHarness
{
public:
    WindowHarness()
    {
        auto managerOwner = std::make_unique<FakeMarkItDownManager>();
        manager = managerOwner.get();

        auto controller =
            std::make_unique<DocumentController>(std::move(managerOwner));

        auto rendererOwner =
            std::make_unique<ControlledMarkdownRenderer>();
        renderer = rendererOwner.get();

        auto dialogsOwner = std::make_unique<FakeMainWindowDialogs>();
        dialogs = dialogsOwner.get();

        window = std::make_unique<MainWindow>(
            std::move(controller),
            std::move(rendererOwner),
            std::move(dialogsOwner));
    }

    QString createSourceDocument(const QString &fileName)
    {
        const QString filePath = temporaryDir.filePath(fileName);
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            return QString();
        }

        if (file.write("source document") < 0) {
            return QString();
        }

        file.close();
        return filePath;
    }

    QAction *action(const char *name) const
    {
        return window->findChild<QAction *>(name);
    }

    QPlainTextEdit *editor() const
    {
        return window->findChild<QPlainTextEdit *>("markdownEditor");
    }

    QTextBrowser *preview() const
    {
        return window->findChild<QTextBrowser *>("markdownPreview");
    }

    QStatusBar *statusBar() const
    {
        return window->findChild<QStatusBar *>("statusbar");
    }

    bool reportRendered(quint64 requestId, const QString &html)
    {
        return QMetaObject::invokeMethod(
            renderer,
            "reportRendered",
            Qt::BlockingQueuedConnection,
            Q_ARG(quint64, requestId),
            Q_ARG(QString, html));
    }

    bool waitForMainThreadBarrier()
    {
        QEventLoop eventLoop;
        if (!QMetaObject::invokeMethod(
                window.get(),
                [&eventLoop]() { eventLoop.quit(); },
                Qt::QueuedConnection)) {
            return false;
        }

        eventLoop.exec();
        return true;
    }

    QTemporaryDir temporaryDir;
    FakeMarkItDownManager *manager = nullptr;
    ControlledMarkdownRenderer *renderer = nullptr;
    FakeMainWindowDialogs *dialogs = nullptr;
    std::unique_ptr<MainWindow> window;
};

quint64 requestIdAt(const QSignalSpy &spy, int index)
{
    return spy.at(index).at(0).toULongLong();
}

QString requestedMarkdownAt(const QSignalSpy &spy, int index)
{
    return spy.at(index).at(1).toString();
}

} // namespace

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void openAction_validDocument_selectsDocumentAndStartsConversion();
    void conversionStarted_activeRequest_disablesControlsAndRejectsDuplicate();
    void conversionFinished_markdownResult_updatesEditorPreviewAndControls();
    void conversionFailed_backendError_restoresControlsAndPresentsError();
    void editorChanged_afterConversion_requestsAndAppliesPreviewUpdate();
    void renderCompleted_outOfOrder_keepsNewestPreview();
    void saveAction_completedMarkdown_writesSelectedTemporaryFile();
    void openAction_newDocument_invalidatesPreviousRenderResult();
};

void TestMainWindow::initTestCase()
{
    qRegisterMetaType<ConversionError>();
}

void TestMainWindow::
    openAction_validDocument_selectsDocumentAndStartsConversion()
{
    // Arrange
    WindowHarness harness;
    QVERIFY(harness.temporaryDir.isValid());
    const QString sourcePath =
        harness.createSourceDocument(QString::fromUtf8(u8"선택 문서.txt"));
    QVERIFY(!sourcePath.isEmpty());
    harness.dialogs->nextSourcePath = sourcePath;

    QAction *openAction = harness.action("actionOpen");
    QVERIFY(openAction);
    QVERIFY(openAction->isEnabled());

    // Act
    openAction->trigger();

    // Assert
    QCOMPARE(harness.dialogs->sourceSelectionCount, 1);
    QVERIFY(harness.dialogs->sourceFilters.constFirst().contains(
        QStringLiteral("*.txt")));
    QCOMPARE(harness.manager->convertCallCount(), 1);
    QCOMPARE(harness.manager->sourcePath(0), QFileInfo(sourcePath).absoluteFilePath());
    QVERIFY(harness.window->windowTitle().contains(
        QFileInfo(sourcePath).fileName()));
    QVERIFY(harness.statusBar()->currentMessage().contains(
        QStringLiteral("Converting...")));
    QVERIFY(!harness.action("actionConvert")->isEnabled());
}

void TestMainWindow::
    conversionStarted_activeRequest_disablesControlsAndRejectsDuplicate()
{
    // Arrange
    WindowHarness harness;
    const QString sourcePath =
        harness.createSourceDocument(QStringLiteral("busy.txt"));
    QVERIFY(!sourcePath.isEmpty());
    harness.dialogs->nextSourcePath = sourcePath;

    // Act
    harness.action("actionOpen")->trigger();
    harness.action("actionConvert")->trigger();

    // Assert
    QCOMPARE(harness.manager->convertCallCount(), 1);
    QVERIFY(!harness.action("actionOpen")->isEnabled());
    QVERIFY(!harness.action("actionConvert")->isEnabled());
    QVERIFY(!harness.action("actionSave")->isEnabled());
    QVERIFY(!harness.action("actionSaveAs")->isEnabled());
    QVERIFY(harness.statusBar()->currentMessage().contains(
        QStringLiteral("Converting...")));
}

void TestMainWindow::
    conversionFinished_markdownResult_updatesEditorPreviewAndControls()
{
    // Arrange
    WindowHarness harness;
    const QString sourcePath =
        harness.createSourceDocument(QStringLiteral("success.txt"));
    QVERIFY(!sourcePath.isEmpty());
    harness.dialogs->nextSourcePath = sourcePath;
    QSignalSpy renderSpy(harness.window.get(),
                         &MainWindow::renderMarkdownRequested);
    const QString markdown = QString::fromUtf8(u8"# 결과\n\n본문");

    harness.action("actionOpen")->trigger();

    // Act: the controlled backend finishes, then the controlled renderer replies.
    harness.manager->reportFinished(markdown);

    // Assert: rendering remains busy until both editor insertion and HTML finish.
    QVERIFY(harness.statusBar()->currentMessage().contains(
        QStringLiteral("Rendering preview...")));
    QVERIFY(!harness.action("actionOpen")->isEnabled());
    QTRY_COMPARE_WITH_TIMEOUT(harness.editor()->toPlainText(), markdown, 5000);
    QCOMPARE(renderSpy.count(), 1);
    QCOMPARE(requestedMarkdownAt(renderSpy, 0), markdown);

    QVERIFY(harness.reportRendered(
        requestIdAt(renderSpy, 0),
        QString::fromUtf8(u8"<h1>결과</h1><p>본문</p>")));

    QTRY_VERIFY_WITH_TIMEOUT(
        harness.preview()->toPlainText().contains(QString::fromUtf8(u8"결과")),
        5000);
    QVERIFY(harness.preview()->toPlainText().contains(
        QString::fromUtf8(u8"본문")));
    QVERIFY(harness.statusBar()->currentMessage().contains(
        QStringLiteral("Converted")));
    QVERIFY(harness.action("actionOpen")->isEnabled());
    QVERIFY(harness.action("actionConvert")->isEnabled());
    QVERIFY(harness.action("actionSave")->isEnabled());
    QVERIFY(harness.action("actionSaveAs")->isEnabled());
}

void TestMainWindow::
    conversionFailed_backendError_restoresControlsAndPresentsError()
{
    // Arrange
    WindowHarness harness;
    const QString sourcePath =
        harness.createSourceDocument(QStringLiteral("failure.txt"));
    QVERIFY(!sourcePath.isEmpty());
    harness.dialogs->nextSourcePath = sourcePath;
    harness.action("actionOpen")->trigger();

    // Act
    const QString details = QString::fromUtf8(u8"backend 세부 오류");
    harness.manager->reportFailed(ConversionError::NonZeroExit, details);

    // Assert
    QCOMPARE(harness.dialogs->criticals.size(), 1);
    QVERIFY(!harness.dialogs->criticals.constFirst().title.isEmpty());
    QVERIFY(harness.dialogs->criticals.constFirst().text.contains(details));
    QVERIFY(harness.statusBar()->currentMessage().contains(
        QStringLiteral("Conversion Failed")));
    QVERIFY(harness.action("actionOpen")->isEnabled());
    QVERIFY(harness.action("actionConvert")->isEnabled());
    QVERIFY(!harness.action("actionSave")->isEnabled());
    QVERIFY(!harness.action("actionSaveAs")->isEnabled());
}

void TestMainWindow::
    editorChanged_afterConversion_requestsAndAppliesPreviewUpdate()
{
    // Arrange
    WindowHarness harness;
    const QString sourcePath =
        harness.createSourceDocument(QStringLiteral("edit.txt"));
    QVERIFY(!sourcePath.isEmpty());
    harness.dialogs->nextSourcePath = sourcePath;
    QSignalSpy renderSpy(harness.window.get(),
                         &MainWindow::renderMarkdownRequested);

    harness.action("actionOpen")->trigger();
    harness.manager->reportFinished(QStringLiteral("Initial"));
    QTRY_COMPARE_WITH_TIMEOUT(renderSpy.count(), 1, 5000);
    QVERIFY(harness.reportRendered(requestIdAt(renderSpy, 0),
                                   QStringLiteral("<p>Initial</p>")));
    QTRY_COMPARE_WITH_TIMEOUT(harness.preview()->toPlainText(),
                              QStringLiteral("Initial"),
                              5000);

    // Act
    const QString editedMarkdown =
        QString::fromUtf8(u8"## 수정됨\n\n새 본문");
    harness.editor()->setPlainText(editedMarkdown);

    // Assert
    QVERIFY(harness.window->windowTitle().endsWith(QStringLiteral(" *")));
    QTRY_COMPARE_WITH_TIMEOUT(renderSpy.count(), 2, 5000);
    QCOMPARE(requestedMarkdownAt(renderSpy, 1), editedMarkdown);

    QVERIFY(harness.reportRendered(
        requestIdAt(renderSpy, 1),
        QString::fromUtf8(u8"<h2>수정됨</h2><p>새 본문</p>")));
    QTRY_VERIFY_WITH_TIMEOUT(harness.preview()->toPlainText().contains(
                                 QString::fromUtf8(u8"수정됨")),
                             5000);
    QVERIFY(harness.preview()->toPlainText().contains(
        QString::fromUtf8(u8"새 본문")));
}

void TestMainWindow::renderCompleted_outOfOrder_keepsNewestPreview()
{
    // Arrange
    WindowHarness harness;
    const QString sourcePath =
        harness.createSourceDocument(QStringLiteral("stale.txt"));
    QVERIFY(!sourcePath.isEmpty());
    harness.dialogs->nextSourcePath = sourcePath;
    QSignalSpy renderSpy(harness.window.get(),
                         &MainWindow::renderMarkdownRequested);

    harness.action("actionOpen")->trigger();
    harness.manager->reportFinished(QStringLiteral("Initial"));
    QCOMPARE(renderSpy.count(), 1);
    QVERIFY(harness.reportRendered(requestIdAt(renderSpy, 0),
                                   QStringLiteral("<p>Initial</p>")));
    QTRY_COMPARE_WITH_TIMEOUT(harness.preview()->toPlainText(),
                              QStringLiteral("Initial"),
                              5000);

    harness.editor()->setPlainText(QStringLiteral("Markdown A"));
    QTRY_COMPARE_WITH_TIMEOUT(renderSpy.count(), 2, 5000);
    const quint64 requestA = requestIdAt(renderSpy, 1);

    harness.editor()->setPlainText(QStringLiteral("Markdown B"));
    QTRY_COMPARE_WITH_TIMEOUT(renderSpy.count(), 3, 5000);
    const quint64 requestB = requestIdAt(renderSpy, 2);
    QVERIFY(requestB != requestA);

    // Act: B completes first and A arrives late.
    QVERIFY(harness.reportRendered(requestB,
                                   QStringLiteral("<p>Preview B</p>")));
    QTRY_COMPARE_WITH_TIMEOUT(harness.preview()->toPlainText(),
                              QStringLiteral("Preview B"),
                              5000);
    QVERIFY(harness.reportRendered(requestA,
                                   QStringLiteral("<p>Stale Preview A</p>")));
    QVERIFY(harness.waitForMainThreadBarrier());

    // Assert
    QCOMPARE(harness.preview()->toPlainText(), QStringLiteral("Preview B"));
}

void TestMainWindow::
    saveAction_completedMarkdown_writesSelectedTemporaryFile()
{
    // Arrange
    WindowHarness harness;
    const QString sourcePath =
        harness.createSourceDocument(QStringLiteral("save-source.txt"));
    QVERIFY(!sourcePath.isEmpty());
    harness.dialogs->nextSourcePath = sourcePath;
    QSignalSpy renderSpy(harness.window.get(),
                         &MainWindow::renderMarkdownRequested);

    const QString markdown =
        QString::fromUtf8(u8"# 저장\n\nEnglish 한글 日本語");
    harness.action("actionOpen")->trigger();
    harness.manager->reportFinished(markdown);
    QCOMPARE(renderSpy.count(), 1);
    QVERIFY(harness.reportRendered(requestIdAt(renderSpy, 0),
                                   QString::fromUtf8(u8"<h1>저장</h1>")));
    QTRY_VERIFY_WITH_TIMEOUT(harness.action("actionSave")->isEnabled(), 5000);

    const QString selectedPath =
        harness.temporaryDir.filePath(QStringLiteral("saved-markdown"));
    const QString expectedPath = selectedPath + QStringLiteral(".md");
    harness.dialogs->nextSavePath = selectedPath;

    // Act
    harness.action("actionSave")->trigger();

    // Assert
    QCOMPARE(harness.dialogs->saveSelectionCount, 1);
    QCOMPARE(QFileInfo(harness.dialogs->saveSuggestions.constFirst()).fileName(),
             QStringLiteral("save-source.md"));
    QFile savedFile(expectedPath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(savedFile.readAll()), markdown);
    QVERIFY(!harness.window->windowTitle().endsWith(QStringLiteral(" *")));
    QVERIFY(harness.statusBar()->currentMessage().contains(
        QStringLiteral("saved-markdown.md")));
}

void TestMainWindow::
    openAction_newDocument_invalidatesPreviousRenderResult()
{
    // Arrange
    WindowHarness harness;
    const QString firstSource =
        harness.createSourceDocument(QStringLiteral("first.txt"));
    const QString secondSource =
        harness.createSourceDocument(QStringLiteral("second.txt"));
    QVERIFY(!firstSource.isEmpty());
    QVERIFY(!secondSource.isEmpty());
    QSignalSpy renderSpy(harness.window.get(),
                         &MainWindow::renderMarkdownRequested);

    harness.dialogs->nextSourcePath = firstSource;
    harness.action("actionOpen")->trigger();
    harness.manager->reportFinished(QStringLiteral("First document"));
    QCOMPARE(renderSpy.count(), 1);
    QVERIFY(harness.reportRendered(requestIdAt(renderSpy, 0),
                                   QStringLiteral("<p>First preview</p>")));
    QTRY_COMPARE_WITH_TIMEOUT(harness.preview()->toPlainText(),
                              QStringLiteral("First preview"),
                              5000);

    harness.editor()->setPlainText(QStringLiteral("Old pending edit"));
    QTRY_COMPARE_WITH_TIMEOUT(renderSpy.count(), 2, 5000);
    const quint64 oldRequest = requestIdAt(renderSpy, 1);

    // Act: a new Open starts before the previous edit render completes.
    harness.dialogs->nextSourcePath = secondSource;
    harness.action("actionOpen")->trigger();
    QCOMPARE(harness.manager->convertCallCount(), 2);
    QCOMPARE(harness.preview()->toPlainText(), QString());

    QVERIFY(harness.reportRendered(
        oldRequest,
        QStringLiteral("<p>Old result that must be ignored</p>")));
    QVERIFY(harness.waitForMainThreadBarrier());

    // Assert: the stale result cannot repopulate the cleared preview.
    QCOMPARE(harness.preview()->toPlainText(), QString());
    QVERIFY(harness.window->windowTitle().contains(
        QFileInfo(secondSource).fileName()));
    QVERIFY(harness.statusBar()->currentMessage().contains(
        QStringLiteral("Converting...")));

    harness.manager->reportFinished(QStringLiteral("Second document"));
    QCOMPARE(renderSpy.count(), 3);
    QVERIFY(harness.reportRendered(requestIdAt(renderSpy, 2),
                                   QStringLiteral("<p>Second preview</p>")));
    QTRY_COMPARE_WITH_TIMEOUT(harness.preview()->toPlainText(),
                              QStringLiteral("Second preview"),
                              5000);
}

QTEST_MAIN(TestMainWindow)

#include "tst_MainWindow.moc"
