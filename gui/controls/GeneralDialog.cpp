#include "stdafx.h"

#include "GeneralDialog.h"
#include "TextEmojiWidget.h"
#include "TextEditEx.h"
#include "SemitransparentWindowAnimated.h"
#include "../fonts.h"
#include "../gui_settings.h"
#include "../main_window/MainWindow.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "../controls/DialogButton.h"
#include "../controls/TextUnit.h"
#include "main_window/history_control/ThreadPlate.h"

namespace
{
    constexpr std::chrono::milliseconds animationDuration = std::chrono::milliseconds(100);
    constexpr auto defaultDialogWidth() noexcept
    {
        return 360;
    }

    int getMargin() noexcept
    {
        return Utils::scale_value(16);
    }

    int getTopMargin() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(12);

        return Utils::scale_value(8);
    }

    int getExtendedVerMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    int getIconSize() noexcept
    {
        return Utils::scale_value(20);
    }

    int getOptionHeight() noexcept
    {
        return Utils::scale_value(44);
    }

    auto dialogMaximumHeight()
    {
        return Utils::scale_value(660) + 2 * Ui::get_gui_settings()->get_shadow_width();
    }

    auto dialogVerticalMargin() noexcept
    {
        return Utils::scale_value(10);
    }
}

namespace Ui
{
    bool GeneralDialog::inExec_ = false;

    GeneralDialog::GeneralDialog(QWidget* _mainWidget, QWidget* _parent, const Options& _options)
        : QDialog(nullptr)
        , mainWidget_(_mainWidget)
        , nextButton_(nullptr)
        , semiWindow_(new SemitransparentWindowAnimated(_parent, animationDuration.count()))
        , headerLabelHost_(nullptr)
        , areaWidget_(nullptr)
        , shadow_(true)
        , leftButtonDisableOnClicked_(false)
        , rightButtonDisableOnClicked_(false)
        , options_(_options)
    {
        setParent(semiWindow_);

        semiWindow_->setCloseWindowInfo({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
        semiWindow_->hide();
        if (_options.withSemiwindow_)
            qApp->processEvents();

        mainHost_ = new QWidget(this);
        Utils::setDefaultBackground(mainHost_);
        Testing::setAccessibleName(mainHost_, qsl("AS GeneralPopup"));

        if (options_.fixedSize_)
        {
            if (options_.preferredWidth_ <= 0)
                mainHost_->setFixedWidth(Utils::scale_value(defaultDialogWidth()));
            else
                mainHost_->setFixedWidth(options_.preferredWidth_);
        }
        else
        {
            mainHost_->setMaximumSize(Utils::GetMainRect().size() - 2 * Utils::scale_value(QSize(8, 8)));
        }

        auto globalLayout = Utils::emptyVLayout(mainHost_);
        globalLayout->setSizeConstraint(QLayout::SetMaximumSize);

        headerLabelHost_ = new QWidget(mainHost_);
        headerLabelHost_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        headerLabelHost_->setVisible(false);
        Testing::setAccessibleName(headerLabelHost_, qsl("AS GeneralPopup headerLabel"));
        globalLayout->addWidget(headerLabelHost_);

        errorHost_ = new QWidget(mainHost_);
        errorHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        Utils::ApplyStyle(errorHost_, qsl("height: 1dip;"));
        errorHost_->setVisible(false);
        Testing::setAccessibleName(errorHost_, qsl("AS GeneralPopup errorLabel"));
        globalLayout->addWidget(errorHost_);

        textHost_ = new QWidget(mainHost_);
        textHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        Utils::ApplyStyle(textHost_, qsl("height: 1dip;"));
        textHost_->setVisible(false);
        Testing::setAccessibleName(textHost_, qsl("AS GeneralPopup textLabel"));
        globalLayout->addWidget(textHost_);

        if (mainWidget_)
        {
            mainWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
            globalLayout->addWidget(mainWidget_);
        }

        areaWidget_ = new QWidget(mainHost_);
        areaWidget_->setVisible(false);
        Testing::setAccessibleName(areaWidget_, qsl("AS GeneralPopup areaWidget"));
        globalLayout->addWidget(areaWidget_);

        bottomWidget_ = new QWidget(mainHost_);
        bottomWidget_->setVisible(false);
        bottomWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        Utils::setDefaultBackground(bottomWidget_);
        Testing::setAccessibleName(bottomWidget_, qsl("AS GeneralPopup bottomWidget"));
        globalLayout->addWidget(bottomWidget_);

        setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint | Qt::SubWindow);
        setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);

        if (options_.rejectable_)
        {
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnySemitransparentWindow, this, &GeneralDialog::rejectDialog);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupWindow, this, &GeneralDialog::rejectDialog);
        }

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mainWindowResized, this, &GeneralDialog::updateSize);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::acceptGeneralDialog, this, &GeneralDialog::acceptDialog);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationLocked, this, &GeneralDialog::close);

        mainHost_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Policy::Expanding);

        auto constrainedLayout = Utils::emptyVLayout(this);
        auto constrainedContainer = new QWidget(this);
        constrainedContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
        constrainedContainer->setMaximumHeight(dialogMaximumHeight());
        const auto verticalMargin = dialogVerticalMargin();
        constrainedLayout->setContentsMargins(0, verticalMargin, 0, verticalMargin);
        constrainedLayout->addWidget(constrainedContainer);

        shadowHost_ = new QWidget;
        auto shadowLayout = Utils::emptyVLayout(shadowHost_);
        shadowLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
        shadowLayout->addWidget(mainHost_);

        auto mainLayout = Utils::emptyVLayout(constrainedContainer);
        mainLayout->addStretch();
        mainLayout->addWidget(shadowHost_, 1);
        mainLayout->addStretch();

        auto parentLayout = Utils::emptyVLayout(parentWidget());
        parentLayout->setAlignment(Qt::AlignHCenter);
        parentLayout->addWidget(this);

        setAttribute(Qt::WA_QuitOnClose, false);
        installEventFilter(this);

        if (auto prevFocused = qobject_cast<QWidget*>(qApp->focusObject()))
        {
            connect(this, &GeneralDialog::finished, prevFocused, [prevFocused]()
            {
                prevFocused->setFocus();
            });
        }
        setFocus();
    }

    void GeneralDialog::rejectDialog(const Utils::CloseWindowInfo& _info)
    {
        if (options_.isIgnored(_info))
            return;

        rejectDialogInternal();
    }

    void GeneralDialog::rejectDialogInternal()
    {
        if (!options_.rejectable_)
            return;

        if (options_.withSemiwindow_)
            semiWindow_->hideAnimated();

        QDialog::reject();
    }

    void GeneralDialog::acceptDialog()
    {
        if (options_.withSemiwindow_)
            semiWindow_->hideAnimated();

        QDialog::accept();
    }

    void GeneralDialog::done(int r)
    {
        window()->setFocus();
        QDialog::done(r);
    }

    void GeneralDialog::addLabel(const QString& _text, Qt::Alignment _alignment, int _maxLinesNumber)
    {
        if (headerLabelHost_->layout())
        {
            auto label = headerLabelHost_->findChild<TextUnitLabel*>();
            if (label)
            {
                label->setText(_text);
                return;
            }
        }

        headerLabelHost_->setVisible(true);
        auto hostLayout = Utils::emptyHLayout(headerLabelHost_);

        auto text = Ui::TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        text->init(Fonts::appFontScaled(22), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, _maxLinesNumber, TextRendering::LineBreakType::PREFER_SPACES);

        const auto maxWidth = mainHost_->maximumWidth() - 2 * getMargin();
        auto label = new TextUnitLabel(this, std::move(text), Ui::TextRendering::VerPosition::TOP, maxWidth, true);
        label->setMaximumWidth(maxWidth);

        const auto leftMargin = (_alignment & Qt::AlignHCenter) ? (maxWidth - label->width()) / 2 + getMargin() : getMargin();
        const auto topMargin = (_alignment & Qt::AlignVCenter) ? getExtendedVerMargin() : getTopMargin();
        hostLayout->setContentsMargins(leftMargin, topMargin, 0, 0);
        hostLayout->setAlignment(hostLayout->alignment() | Qt::AlignTop);
        Testing::setAccessibleName(label, qsl("AS GeneralPopup label"));
        hostLayout->addWidget(label);

        if (options_.threadBadge_)
        {
            hostLayout->addSpacing(getMargin());
            hostLayout->addWidget(ThreadPlate::plateForPopup(this), 0, Qt::AlignRight);
            hostLayout->addSpacing(getMargin());
        }
    }

    void GeneralDialog::addText(const QString& _messageText, int _upperMarginPx)
    {
        addText(_messageText, _upperMarginPx, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    }

    void GeneralDialog::addText(const QString& _messageText, int _upperMarginPx, const QFont& _font, const QColor& _color)
    {
        auto text = Ui::TextRendering::MakeTextUnit(_messageText, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        text->init(_font, _color);

        const auto maxWidth = mainHost_->maximumWidth() - 2 * getMargin();
        auto label = new TextUnitLabel(textHost_, std::move(text), Ui::TextRendering::VerPosition::TOP, maxWidth);
        label->setFixedWidth(maxWidth);
        label->resize(label->sizeHint());
        Testing::setAccessibleName(label, qsl("AS GeneralPopup textLabel"));

        auto textLayout = Utils::emptyHLayout(textHost_);
        textLayout->setContentsMargins(getMargin(), _upperMarginPx, getMargin(), 0);
        textLayout->addWidget(label);
        textHost_->setVisible(true);
    }


    DialogButton* GeneralDialog::addAcceptButton(const QString& _buttonText, const bool _isEnabled)
    {
        {
            nextButton_ =  new DialogButton(bottomWidget_, _buttonText, _isEnabled ? DialogButtonRole::CONFIRM : DialogButtonRole::DISABLED);
            nextButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            nextButton_->adjustSize();
            setButtonActive(_isEnabled);

            connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);

            Testing::setAccessibleName(nextButton_, qsl("AS GeneralPopup nextButton"));

            auto bottomLayout = getBottomLayout();
            bottomLayout->addWidget(nextButton_);
        }

        bottomWidget_->setVisible(true);

        return nextButton_;
    }

    DialogButton* GeneralDialog::addCancelButton(const QString& _buttonText, const bool _setActive)
    {
        {
            nextButton_ = new DialogButton(bottomWidget_, _buttonText, DialogButtonRole::CANCEL);
            nextButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            nextButton_->adjustSize();
            setButtonActive(_setActive);

            connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::reject, Qt::QueuedConnection);

            Testing::setAccessibleName(nextButton_, qsl("AS GeneralPopup cancelButton"));

            auto bottomLayout = getBottomLayout();
            bottomLayout->addWidget(nextButton_);
        }

        bottomWidget_->setVisible(true);

        return nextButton_;
    }

    QPair<DialogButton*, DialogButton*> GeneralDialog::addButtonsPair(const QString& _buttonTextLeft, const QString& _buttonTextRight, bool _isActive, bool _rejectable, bool _acceptable, QWidget* _area)
    {
        QPair<DialogButton*, DialogButton*> result;
        {
            auto cancelButton = new DialogButton(bottomWidget_, _buttonTextLeft, DialogButtonRole::CANCEL);
            cancelButton->setObjectName(qsl("left_button"));
            cancelButton->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            connect(cancelButton, &DialogButton::clicked, this, &GeneralDialog::leftButtonClick, Qt::QueuedConnection);
            if (_rejectable)
                connect(cancelButton, &DialogButton::clicked, this, &GeneralDialog::reject, Qt::QueuedConnection);
            Testing::setAccessibleName(cancelButton, qsl("AS GeneralPopup cancelButton"));

            nextButton_ = new DialogButton(bottomWidget_, _buttonTextRight, DialogButtonRole::CONFIRM);
            setButtonActive(_isActive);
            nextButton_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::rightButtonClick, Qt::QueuedConnection);
            if (_acceptable)
                connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);
            Testing::setAccessibleName(nextButton_, qsl("AS GeneralPopup nextButton"));

            auto bottomLayout = getBottomLayout();
            bottomLayout->addWidget(cancelButton);
            bottomLayout->addSpacing(getMargin());
            bottomLayout->addWidget(nextButton_);

            result = { nextButton_, cancelButton };
        }

        if (_area)
        {
            auto v = Utils::emptyVLayout(areaWidget_);
            Testing::setAccessibleName(_area, qsl("AS GeneralPopup area"));
            v->addWidget(_area);
            areaWidget_->setVisible(true);
        }

        bottomWidget_->setVisible(true);

        return result;
    }

    void GeneralDialog::setButtonsAreaMargins(const QMargins& _margins)
    {
        getBottomLayout()->setContentsMargins(_margins);
    }

    DialogButton* GeneralDialog::takeAcceptButton()
    {
        if (nextButton_)
            QObject::disconnect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::accept);

        return nextButton_;
    }

    void GeneralDialog::setAcceptButtonText(const QString& _text)
    {
        if (!nextButton_)
            return;

        nextButton_->setText(_text);
    }

    GeneralDialog::~GeneralDialog()
    {
        semiWindow_->close();
    }

    bool GeneralDialog::showInCenter()
    {
        if (shadow_)
        {
            shadow_ = false;
            Utils::addShadowToWindow(shadowHost_, true);
        }

        if (options_.withSemiwindow_)
        {
            if (!semiWindow_->isVisible())
                semiWindow_->showAnimated();
        }
        else
        {
            semiWindow_->setStep(0);
            semiWindow_->show();
        }

        show();

        inExec_ = true;
        const auto guard = QPointer(this);
        const auto result = (exec() == QDialog::Accepted);
        if (!guard)
            return false;
        inExec_ = false;
        close();

        if constexpr (platform::is_apple())
            semiWindow_->parentWidget()->activateWindow();
        if (semiWindow_->isVisible())
            semiWindow_->hideAnimated();
        return result;
    }

    QWidget* GeneralDialog::getMainHost()
    {
        return mainHost_;
    }

    void GeneralDialog::setButtonActive(bool _active)
    {
        if (!nextButton_)
            return;

        nextButton_->setEnabled(_active);
    }

    QHBoxLayout* GeneralDialog::getBottomLayout()
    {
        im_assert(bottomWidget_);
        if (bottomWidget_->layout())
        {
            im_assert(qobject_cast<QHBoxLayout*>(bottomWidget_->layout()));
            return qobject_cast<QHBoxLayout*>(bottomWidget_->layout());
        }

        auto bottomLayout = Utils::emptyHLayout(bottomWidget_);
        bottomLayout->setContentsMargins(getMargin(), getMargin(), getMargin(), getMargin());
        bottomLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
        return bottomLayout;
    }

    void GeneralDialog::mousePressEvent(QMouseEvent* _e)
    {
        QDialog::mousePressEvent(_e);

        if (options_.rejectable_ && !mainHost_->rect().contains(mainHost_->mapFrom(this, _e->pos())))
            close();
        else
            _e->accept();
    }

    void GeneralDialog::keyPressEvent(QKeyEvent* _e)
    {
        if (ignoredKeys_.find(static_cast<Qt::Key>(_e->key())) != ignoredKeys_.end())
            return;

        if (_e->key() == Qt::Key_Escape)
        {
            close();
            return;
        }
        else if (_e->key() == Qt::Key_Return || _e->key() == Qt::Key_Enter)
        {
            if (!options_.ignoreKeyPressEvents_ && (!nextButton_ || nextButton_->isEnabled()))
                accept();
            else
                _e->ignore();
            return;
        }

        QDialog::keyPressEvent(_e);
    }

    bool GeneralDialog::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_obj == this)
        {
            if (_event->type() == QEvent::ShortcutOverride || _event->type() == QEvent::KeyPress)
            {
                auto keyEvent = static_cast<QKeyEvent*>(_event);
                if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
                {
                    keyEvent->accept();
                    return true;
                }
            }
        }

        return QDialog::eventFilter(_obj, _event);
    }

    void GeneralDialog::showEvent(QShowEvent *event)
    {
        QDialog::showEvent(event);
        Q_EMIT shown(this);
    }

    void GeneralDialog::hideEvent(QHideEvent* _event)
    {
        Q_EMIT hidden(this);
        QDialog::hideEvent(_event);
    }

    void GeneralDialog::moveEvent(QMoveEvent* _event)
    {
        QDialog::moveEvent(_event);
        Q_EMIT moved(this);
    }

    void GeneralDialog::resizeEvent(QResizeEvent* _event)
    {
        QDialog::resizeEvent(_event);
        updateSize();

        Q_EMIT resized(this);
    }

    void GeneralDialog::addError(const QString& _messageText)
    {
        errorHost_->setVisible(true);
        errorHost_->setContentsMargins(0, 0, 0, 0);

        auto textLayout = Utils::emptyVLayout(errorHost_);

        auto upperSpacer = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer);

        const QString backgroundStyle = qsl("background-color: #fbdbd9; ");
        const QString labelStyle = u"QWidget { " % backgroundStyle % u"border: none; padding-left: 16dip; padding-right: 16dip; padding-top: 0dip; padding-bottom: 0dip; }";

        auto upperSpacerRedUp = new QLabel();
        upperSpacerRedUp->setFixedHeight(Utils::scale_value(16));
        Utils::ApplyStyle(upperSpacerRedUp, backgroundStyle);
        Testing::setAccessibleName(upperSpacerRedUp, qsl("AS GeneralPopup upperSpacerRedUp"));
        textLayout->addWidget(upperSpacerRedUp);

        auto errorLabel = new Ui::TextEditEx(errorHost_, Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION), true, true);
        errorLabel->setContentsMargins(0, 0, 0, 0);
        errorLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        errorLabel->setPlaceholderText(QString());
        errorLabel->setAutoFillBackground(false);
        errorLabel->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        errorLabel->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(errorLabel, labelStyle);

        errorLabel->setText(QT_TRANSLATE_NOOP("popup_window", "Unfortunately, an error occurred:"));
        Testing::setAccessibleName(errorLabel, qsl("AS GeneralPopup errorLabel"));
        textLayout->addWidget(errorLabel);

        auto errorText = new Ui::TextEditEx(errorHost_, Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), true, true);
        errorText->setContentsMargins(0, 0, 0, 0);
        errorText->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        errorText->setPlaceholderText(QString());
        errorText->setAutoFillBackground(false);
        errorText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        errorText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(errorText, labelStyle);

        errorText->setText(_messageText);
        Testing::setAccessibleName(errorText, qsl("AS GeneralPopup errorText"));
        textLayout->addWidget(errorText);

        auto upperSpacerRedBottom = new QLabel();
        Utils::ApplyStyle(upperSpacerRedBottom, backgroundStyle);
        upperSpacerRedBottom->setFixedHeight(Utils::scale_value(16));
        Testing::setAccessibleName(upperSpacerRedBottom, qsl("AS GeneralPopup upperSpacerRedBottom"));
        textLayout->addWidget(upperSpacerRedBottom);

        auto upperSpacer2 = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer2);
    }

    void GeneralDialog::leftButtonClick()
    {
        if (auto leftButton = bottomWidget_->findChild<DialogButton*>(qsl("left_button")))
            leftButton->setEnabled(!leftButtonDisableOnClicked_);
        Q_EMIT leftButtonClicked();
    }

    void GeneralDialog::rightButtonClick()
    {
        nextButton_->setEnabled(!rightButtonDisableOnClicked_);
        Q_EMIT rightButtonClicked();
    }

    void GeneralDialog::updateSize()
    {
        if (semiWindow_)
            semiWindow_->updateSize();
    }

    void GeneralDialog::setIgnoreClicksInSemiWindow(bool _ignore)
    {
        if (semiWindow_)
            semiWindow_->setForceIgnoreClicks(_ignore);
    }

    void GeneralDialog::setIgnoredKeys(const GeneralDialog::KeysContainer &_keys)
    {
        ignoredKeys_ = _keys;
    }

    bool GeneralDialog::isActive()
    {
        return inExec_;
    }

    int GeneralDialog::getHeaderHeight() const
    {
        return headerLabelHost_ ? headerLabelHost_->sizeHint().height() : 0;
    }

    void GeneralDialog::setTransparentBackground(bool _enable)
    {
        if (_enable)
            Utils::updateBgColor(mainHost_, Qt::transparent);
        else
            Utils::setDefaultBackground(mainHost_);
    }

    QSize GeneralDialog::sizeHint() const
    {
        return layout()->sizeHint();
    }

    OptionWidget::OptionWidget(QWidget* _parent, const QString& _icon, const QString& _caption)
        : SimpleListItem(_parent)
        , icon_(Utils::renderSvg(_icon, QSize(getIconSize(), getIconSize()), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY)))
        , caption_(TextRendering::MakeTextUnit(_caption, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS))
    {
        setFixedHeight(getOptionHeight());

        caption_->init(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        caption_->setOffsets(getMargin() * 2 + getIconSize(), height() / 2);
    }

    OptionWidget::~OptionWidget() = default;

    void OptionWidget::paintEvent(QPaintEvent*)
    {
        QPainter p(this);

        if (isHovered())
            p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

        p.drawPixmap(getMargin(), getOptionHeight() / 2 - getIconSize() / 2, icon_);
        caption_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    void OptionWidget::resizeEvent(QResizeEvent* _e)
    {
        if (width() != _e->oldSize().width())
        {
            caption_->elide(width() - getMargin() * 3 - getIconSize());
            update();
        }
    }

    MultipleOptionsWidget::MultipleOptionsWidget(QWidget* _parent, const optionsVector& _options)
        : QWidget(_parent)
        , listWidget_(new SimpleListWidget(Qt::Vertical, this))
    {
        im_assert(!_options.empty());

        int i = 1;
        for (const auto& [icon, caption] : _options)
        {
            auto opt = new OptionWidget(this, icon, caption);
            Testing::setAccessibleName(opt, qsl("AS optionsWidget option") % QString::number(i++));

            listWidget_->addItem(opt);
        }

        connect(listWidget_, &SimpleListWidget::clicked, this, [this](int idx)
        {
            if (!listWidget_->isValidIndex(idx))
                return;

            selectedIndex_ = idx;
            Q_EMIT Utils::InterConnector::instance().acceptGeneralDialog();
        });

        auto layout = Utils::emptyVLayout(this);
        layout->addSpacing(getMargin());
        layout->addWidget(listWidget_);

        setFixedHeight(getOptionHeight() * _options.size() + getMargin());
        Testing::setAccessibleName(this, qsl("AS optionsWidget"));
    }

    bool GeneralDialog::Options::isIgnored(const Utils::CloseWindowInfo& _info) const
    {
        return std::any_of(ignoredInfos_.begin(), ignoredInfos_.end(), [&_info](const auto& i) { return i == _info; });
    }
}
