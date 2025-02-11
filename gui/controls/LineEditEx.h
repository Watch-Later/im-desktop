#pragma once


namespace Ui
{
    class LineEditEx : public QLineEdit
    {
        Q_OBJECT
Q_SIGNALS:
        void focusIn();
        void focusOut();
        void clicked(Qt::MouseButton _button);
        void emptyTextBackspace();
        void escapePressed();
        void upArrow();
        void downArrow();
        void enter();
        void tab();
        void backtab();
        void scrolled(int _steps, QPrivateSignal);

    public:
        struct Options
        {
            std::unordered_set<int> noPropagateKeys_;
        };

    public:
        explicit LineEditEx(QWidget* _parent, const Options& _options = Options());

        void setCustomPlaceholder(const QString& _text);
        void setCustomPlaceholderColor(const QColor& _color);
        void changeTextColor(const QColor& _color);

        QMenu* contextMenu();

        void setOptions(Options _options);

    protected:
        void focusInEvent(QFocusEvent*) override;
        void focusOutEvent(QFocusEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void keyPressEvent(QKeyEvent*) override;
        void contextMenuEvent(QContextMenuEvent *) override;
        void paintEvent(QPaintEvent* _event) override;
        bool eventFilter(QObject* _obj, QEvent* _event) override;
        void wheelEvent(QWheelEvent* _event) override;

        enum class RenderMargins
        {
            ContentMargin,
            TextMargin
        };

        void setRenderMargins(RenderMargins _margins);

    private:
        QString customPlaceholder_;
        QColor  customPlaceholderColor_;
        QMenu* contextMenu_ = nullptr;
        Options options_;
        RenderMargins margins_ = RenderMargins::ContentMargin;
    };
}
