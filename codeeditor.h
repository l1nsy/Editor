#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

class QPaintEvent;
class QResizeEvent;
class QWidget;

// 带行号栏的文本编辑器。
// 通过继承 QPlainTextEdit 并额外在左侧绘制一个行号区域实现行号显示，
// 在 .ui 文件中以“提升（promote）”的方式替换普通的 QPlainTextEdit 控件。
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    // 行号栏的宽度，随总行数的位数变化
    int lineNumberAreaWidth() const;
    // 由行号栏控件回调，负责具体的绘制工作
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    bool isLineNumberVisible() const;
    void setLineNumberVisible(bool visible);

    // 字体或背景色改变后，刷新行号栏宽度和当前行高亮
    void refreshLineNumberArea();
    void refreshCurrentLineHighlight();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    QWidget *m_lineNumberArea;      // 行号栏
    bool m_lineNumberVisible;       // 行号栏是否显示
};

#endif // CODEEDITOR_H
