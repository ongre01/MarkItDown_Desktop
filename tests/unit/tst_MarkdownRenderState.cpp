#include "src/rendering/MarkdownRenderState.h"

#include <QtTest>

class TestMarkdownRenderState : public QObject
{
    Q_OBJECT

private slots:
    void beginInitialRequest_newRequest_createsAcceptedInitialRequest();
    void beginUpdateRequest_afterInitialRequest_rejectsPreviousRequest();
    void beginUpdateRequest_consecutiveRequests_acceptsOnlyNewestRequest();
    void outOfOrderCompletion_newestAcceptedAndStaleRejected();
    void invalidate_activeRequest_resetsRequestState();
    void invalidate_followedByNewRequest_acceptsOnlyNewRequest();
    void completeRequest_activeRequest_rejectsFurtherResults();
    void completeRequest_followedByNewRequest_acceptsOnlyNewRequest();
    void storeRenderedHtml_repeatedCallback_replacesPreviousHtml();
    void initialRequest_completionConditions_reportExpectedReadiness_data();
    void initialRequest_completionConditions_reportExpectedReadiness();
    void beginUpdateRequest_allCompletionConditions_neverReportsInitialReady();
};

void TestMarkdownRenderState::
    beginInitialRequest_newRequest_createsAcceptedInitialRequest()
{
    // Arrange
    MarkdownRenderState state;

    // Act
    const quint64 requestId = state.beginInitialRequest();

    // Assert
    QVERIFY(requestId != 0);
    QVERIFY(state.accepts(requestId));
    QVERIFY(state.isInitialRequest());
    QVERIFY(!state.isInitialRequestReady());
    QVERIFY(state.renderedHtml().isEmpty());
}

void TestMarkdownRenderState::
    beginUpdateRequest_afterInitialRequest_rejectsPreviousRequest()
{
    // Arrange
    MarkdownRenderState state;
    const quint64 initialRequestId = state.beginInitialRequest();
    state.storeRenderedHtml(QStringLiteral("old html"));
    state.markEditorInsertionFinished();

    // Act
    const quint64 updateRequestId = state.beginUpdateRequest();

    // Assert
    QVERIFY(updateRequestId > initialRequestId);
    QVERIFY(!state.accepts(initialRequestId));
    QVERIFY(state.accepts(updateRequestId));
    QVERIFY(!state.isInitialRequest());
    QVERIFY(!state.isInitialRequestReady());
    QVERIFY(state.renderedHtml().isEmpty());
}

void TestMarkdownRenderState::
    beginUpdateRequest_consecutiveRequests_acceptsOnlyNewestRequest()
{
    // Arrange
    MarkdownRenderState state;

    // Act
    const quint64 firstRequestId = state.beginUpdateRequest();
    const quint64 secondRequestId = state.beginUpdateRequest();
    const quint64 thirdRequestId = state.beginUpdateRequest();

    // Assert
    QVERIFY(firstRequestId < secondRequestId);
    QVERIFY(secondRequestId < thirdRequestId);
    QVERIFY(!state.accepts(firstRequestId));
    QVERIFY(!state.accepts(secondRequestId));
    QVERIFY(state.accepts(thirdRequestId));
    QVERIFY(!state.isInitialRequest());
}

void TestMarkdownRenderState::
    outOfOrderCompletion_newestAcceptedAndStaleRejected()
{
    // Arrange: request #1 starts, followed by request #2.
    MarkdownRenderState state;
    const quint64 firstRequestId = state.beginUpdateRequest();
    const quint64 secondRequestId = state.beginUpdateRequest();

    // Act / Assert: only request #2 may complete.
    QVERIFY(state.accepts(secondRequestId));
    QVERIFY(!state.accepts(firstRequestId));
    state.completeRequest();

    // A late result from request #1 and any duplicate #2 result are both stale.
    QVERIFY(!state.accepts(firstRequestId));
    QVERIFY(!state.accepts(secondRequestId));
}

void TestMarkdownRenderState::invalidate_activeRequest_resetsRequestState()
{
    // Arrange
    MarkdownRenderState state;
    const quint64 requestId = state.beginInitialRequest();
    state.storeRenderedHtml(QStringLiteral("rendered html"));
    state.markEditorInsertionFinished();
    QVERIFY(state.isInitialRequestReady());

    // Act
    state.invalidate();

    // Assert
    QVERIFY(!state.accepts(requestId));
    QVERIFY(!state.isInitialRequest());
    QVERIFY(!state.isInitialRequestReady());
    QVERIFY(state.renderedHtml().isEmpty());
}

void TestMarkdownRenderState::
    invalidate_followedByNewRequest_acceptsOnlyNewRequest()
{
    // Arrange
    MarkdownRenderState state;
    const quint64 invalidatedRequestId = state.beginInitialRequest();
    state.storeRenderedHtml(QStringLiteral("stale html"));
    state.invalidate();

    // Act
    const quint64 newRequestId = state.beginUpdateRequest();

    // Assert
    QVERIFY(newRequestId > invalidatedRequestId);
    QVERIFY(!state.accepts(invalidatedRequestId));
    QVERIFY(state.accepts(newRequestId));
    QVERIFY(!state.isInitialRequest());
    QVERIFY(state.renderedHtml().isEmpty());
}

void TestMarkdownRenderState::
    completeRequest_activeRequest_rejectsFurtherResults()
{
    // Arrange
    MarkdownRenderState state;
    const quint64 requestId = state.beginUpdateRequest();

    // Act
    state.completeRequest();

    // Assert
    QVERIFY(!state.accepts(requestId));
    QVERIFY(!state.isInitialRequest());
    QVERIFY(state.renderedHtml().isEmpty());
}

void TestMarkdownRenderState::
    completeRequest_followedByNewRequest_acceptsOnlyNewRequest()
{
    // Arrange
    MarkdownRenderState state;
    const quint64 completedRequestId = state.beginUpdateRequest();
    state.completeRequest();

    // Act
    const quint64 newRequestId = state.beginInitialRequest();

    // Assert
    QVERIFY(newRequestId > completedRequestId);
    QVERIFY(!state.accepts(completedRequestId));
    QVERIFY(state.accepts(newRequestId));
    QVERIFY(state.isInitialRequest());
    QVERIFY(!state.isInitialRequestReady());
}

void TestMarkdownRenderState::
    storeRenderedHtml_repeatedCallback_replacesPreviousHtml()
{
    // Arrange
    MarkdownRenderState state;
    const quint64 requestId = state.beginUpdateRequest();
    state.storeRenderedHtml(QStringLiteral("<p>first</p>"));

    // Act
    state.storeRenderedHtml(QStringLiteral("<p>replacement</p>"));

    // Assert
    QVERIFY(state.accepts(requestId));
    QCOMPARE(state.renderedHtml(), QStringLiteral("<p>replacement</p>"));
}

void TestMarkdownRenderState::
    initialRequest_completionConditions_reportExpectedReadiness_data()
{
    QTest::addColumn<bool>("renderFinished");
    QTest::addColumn<bool>("editorFinished");
    QTest::addColumn<bool>("expectedReady");

    QTest::newRow("neither-finished") << false << false << false;
    QTest::newRow("render-only") << true << false << false;
    QTest::newRow("editor-only") << false << true << false;
    QTest::newRow("both-finished") << true << true << true;
}

void TestMarkdownRenderState::
    initialRequest_completionConditions_reportExpectedReadiness()
{
    // Arrange
    QFETCH(bool, renderFinished);
    QFETCH(bool, editorFinished);
    QFETCH(bool, expectedReady);
    MarkdownRenderState state;
    state.beginInitialRequest();

    // Act
    if (renderFinished) {
        state.storeRenderedHtml(QStringLiteral("<p>ready</p>"));
    }
    if (editorFinished) {
        state.markEditorInsertionFinished();
    }

    // Assert
    QCOMPARE(state.isInitialRequestReady(), expectedReady);
    QCOMPARE(state.renderedHtml(),
             renderFinished ? QStringLiteral("<p>ready</p>") : QString());
}

void TestMarkdownRenderState::
    beginUpdateRequest_allCompletionConditions_neverReportsInitialReady()
{
    // Arrange
    MarkdownRenderState state;
    const quint64 requestId = state.beginUpdateRequest();

    // Act
    state.storeRenderedHtml(QStringLiteral("<p>updated</p>"));
    state.markEditorInsertionFinished();

    // Assert
    QVERIFY(state.accepts(requestId));
    QVERIFY(!state.isInitialRequest());
    QVERIFY(!state.isInitialRequestReady());
    QCOMPARE(state.renderedHtml(), QStringLiteral("<p>updated</p>"));
}

QTEST_APPLESS_MAIN(TestMarkdownRenderState)

#include "tst_MarkdownRenderState.moc"
