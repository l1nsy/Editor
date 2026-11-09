#include "codeeditor.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTextBlock>
#include <QTextFormat>

namespace {
const int kRightMargin = 8;          // 行号数字与编辑区之间的留白
const int kMinLineNumberWidth = 42;  // 行号栏最小宽度
}

// 行号栏本身只是一个“画布”，绘制工作交回 CodeEditor 完成，
// 这样可以方便地读取编辑器的字体、滚动位置等信息。
class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor)
        : QWidget(editor), m_editor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *m_editor;
};

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(nullptr)
    , m_lineNumberVisible(true)
{
    m_lineNumberArea = new LineNumberArea(this);

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() const
{
    // 根据最大行号的位数计算宽度，避免行数变多后数字被截断
    int digits = 1;
    int maxLines = qMax(1, blockCount());
    while (maxLines >= 10) {
        maxLines /= 10;
        ++digits;
    }
    const int digitWidth = fontMetrics().horizontalAdvance(QLatin1Char('9'));
    const int width = kRightMargin * 2 + digitWidth * digits;
    return qMax(width, kMinLineNumberWidth);
}

bool CodeEditor::isLineNumberVisible() const
{
    return m_lineNumberVisible;
}

void CodeEditor::setLineNumberVisible(bool visible)
{
    if (m_lineNumberVisible == visible)
        return;

    m_lineNumberVisible = visible;
    m_lineNumberArea->setVisible(visible);
    updateLineNumberAreaWidth(blockCount());
    viewport()->update();
}

void CodeEditor::refreshLineNumberArea()
{
    updateLineNumberAreaWidth(blockCount());
    m_lineNumberArea->update();
}

void CodeEditor::refreshCurrentLineHighlight()
{
    highlightCurrentLine();
}

void CodeEditor::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    // 用视口边距给行号栏让出位置，不显示行号时边距归零
    setViewportMargins(m_lineNumberVisible ? lineNumberAreaWidth() : 0, 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (!m_lineNumberVisible)
        return;

    if (dy != 0)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(blockCount());
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect area = contentsRect();
    m_lineNumberArea->setGeometry(QRect(area.left(), area.top(),
                                        lineNumberAreaWidth(), area.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!m_lineNumberVisible)
        return;

    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor("#f0f2f5"));

    // 逐块（段落）计算行号在屏幕上的位置，只绘制可见区域内的行号
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    painter.setFont(font());
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(QColor("#7a828e"));
            painter.drawText(0, top,
                             m_lineNumberArea->width() - kRightMargin,
                             fontMetrics().height(),
                             Qt::AlignRight,
                             QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::highlightCurrentLine()
{
    // 高亮光标所在行；背景色深浅不同时自动调整高亮色的明暗
    if (isReadOnly())
        return;

    const QColor base = palette().color(QPalette::Base);
    const QColor lineColor = base.lightness() > 128 ? base.darker(104)
                                                    : base.lighter(140);

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();

    QList<QTextEdit::ExtraSelection> selections;
    selections.append(selection);
    setExtraSelections(selections);
}
