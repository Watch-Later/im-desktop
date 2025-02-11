#include "stdafx.h"
#import "VideoFrameMacos.h"
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#include "render/VOIPRenderViewClasses.h"
#include "../../utils/utils.h"
#include "iTunes.h"
#include "styles/ThemeParameters.h"
#include "voip/CommonUI.h"

@interface WindowRect : NSObject
{
}

@property NSRect rect;

@end

@implementation WindowRect

-(instancetype)init
{
    self = [super init];
    if (!!self)
    {
        NSRect rc;
        rc.origin.x = 0.0f;
        rc.origin.y = 0.0f;
        rc.size.width  = 0.0f;
        rc.size.height = 0.0f;
        self.rect = rc;
    }
    return self;
}

@end

@interface WindowListApplierData : NSObject
{
}

@property (strong, nonatomic) NSMutableArray* outputArray;
@property int order;

@end

@implementation WindowListApplierData

-(instancetype)initWindowListData:(NSMutableArray *)array
 {
    self = [super init];
    if (!!self)
    {
        self.outputArray = array;
        self.order = 0;
    }
    return self;
}

-(void)dealloc
{
    if (!!self && !!self.outputArray)
    {
        for (id item in self.outputArray)
        {
            [item release];
        }
    }
    [super dealloc];
}

@end

@interface FullScreenDelegate : NSObject <NSWindowDelegate>
{
    platform_macos::FullScreenNotificaton* _notification;
    // Save prev delegate
    NSObject <NSWindowDelegate>* _oldDelegate;
}
@end

@implementation FullScreenDelegate

-(id)init: (platform_macos::FullScreenNotificaton*) notification
     withOldDelegate:(NSObject <NSWindowDelegate>*) oldDelegate
{
    if (![super init])
        return nil;
    _notification = notification;
    _oldDelegate  = oldDelegate;
    [self registerSpaceNotification];
    return self;
}

-(void)dealloc
{
    [self unregisterSpaceNotification];
    [super dealloc];
}

-(void)windowWillEnterFullScreen:(NSNotification *)notification
{
    if (_notification)
    {
        _notification->fullscreenAnimationStart();
        _notification->changeFullscreenState(true);
    }
}

-(void)windowDidEnterFullScreen:(NSNotification *)notification
{
    if (_notification)
        _notification->fullscreenAnimationFinish();
}

-(void)windowWillExitFullScreen:(NSNotification *)notification
{
    if (_notification)
    {
        _notification->fullscreenAnimationStart();
        _notification->changeFullscreenState(false);
    }
}

-(void)windowDidExitFullScreen:(NSNotification *)notification
{
    if (_notification)
        _notification->fullscreenAnimationFinish();
}

-(void)activeSpaceDidChange:(NSNotification *)notification
{
    if (_notification)
        _notification->activeSpaceDidChange();
}

-(void)registerSpaceNotification
{
    NSNotificationCenter *notificationCenter = [[NSWorkspace sharedWorkspace] notificationCenter];
    if (notificationCenter)
    {
        [notificationCenter addObserver:self
           selector:@selector(activeSpaceDidChange:)
               name:NSWorkspaceActiveSpaceDidChangeNotification
             object:[NSWorkspace sharedWorkspace]];
    }
}

-(void)unregisterSpaceNotification
{
    NSNotificationCenter *notificationCenter = [[NSWorkspace sharedWorkspace] notificationCenter];
    if (notificationCenter)
        [notificationCenter removeObserver:self];
}

-(BOOL)windowShouldClose:(id)sender
{
    if (_oldDelegate)
        return [_oldDelegate windowShouldClose: sender];
    return YES;
}

-(NSObject<NSWindowDelegate>*)getDelegate
{
    return _oldDelegate;
}

@end

NSString *kAppNameKey      = @"applicationName"; // Application Name & PID
NSString *kWindowOriginKey = @"windowOrigin";    // Window Origin as a string
NSString *kWindowSizeKey   = @"windowSize";      // Window Size as a string
NSString *kWindowRectKey   = @"windowRect";      // The overall front-to-back ordering of the windows as returned by the window server
NSString *kWindowIDKey     = @"windowId";

inline uint32_t ChangeBits(uint32_t currentBits, uint32_t flagsToChange, BOOL setFlags)
{
    if (setFlags)
        return currentBits | flagsToChange;
    else
        return currentBits & ~flagsToChange;
}

void WindowListApplierFunction(const void *inputDictionary, void *context)
{
    im_assert(!!inputDictionary && !!context);
    if (!inputDictionary || !context)
        return;

    NSDictionary* entry         = (__bridge NSDictionary*)inputDictionary;
    WindowListApplierData* data = (__bridge WindowListApplierData*)context;

    int sharingState = [entry[(id)kCGWindowSharingState] intValue];
    if (sharingState != kCGWindowSharingNone)
    {
        NSMutableDictionary *outputEntry = [NSMutableDictionary dictionary];
        im_assert(!!outputEntry);
        if (!outputEntry)
            return;

        NSString* applicationName = entry[(id)kCGWindowOwnerName];
        if(applicationName != NULL)
        {
            if ([applicationName compare:@"dock" options:NSCaseInsensitiveSearch] == NSOrderedSame)
                return;
            if ([applicationName compare:@"screenshot" options:NSCaseInsensitiveSearch] == NSOrderedSame)
                return;
#ifdef _DEBUG
            NSString *nameAndPID = [NSString stringWithFormat:@"%@ (%@)", applicationName, entry[(id)kCGWindowOwnerPID]];
            outputEntry[kAppNameKey] = nameAndPID;
#endif
        } else
            return;

        CGRect bounds;
        CGRectMakeWithDictionaryRepresentation((CFDictionaryRef)entry[(id)kCGWindowBounds], &bounds);
#ifdef _DEBUG
        NSString *originString = [NSString stringWithFormat:@"%.0f/%.0f", bounds.origin.x, bounds.origin.y];
        outputEntry[kWindowOriginKey] = originString;
        NSString *sizeString = [NSString stringWithFormat:@"%.0f*%.0f", bounds.size.width, bounds.size.height];
        outputEntry[kWindowSizeKey] = sizeString;
#endif
        outputEntry[kWindowIDKey] = entry[(id)kCGWindowNumber];

        WindowRect* wr = [[WindowRect alloc] init];
        wr.rect = bounds;
        outputEntry[kWindowRectKey] = wr;

        [data.outputArray addObject:outputEntry];
    }
}

void platform_macos::fadeIn(QWidget* wnd)
{
    im_assert(!!wnd);
    if (!wnd)
        return;

    NSView* view = (NSView*)wnd->winId();
    im_assert(view);

    NSWindow* window = [view window];
    im_assert(window);

    if ([[window animator] alphaValue] < 0.01f)
        [[window animator] setAlphaValue:1.0f];
}

void platform_macos::fadeOut(QWidget* wnd)
{
    im_assert(!!wnd);
    if (!wnd)
        return;

    NSView* view = (NSView*)wnd->winId();
    im_assert(view);

    NSWindow* window = [view window];
    im_assert(window);

    if ([[window animator] alphaValue] > 0.9f)
        [[window animator] setAlphaValue:0.0f];
}

void platform_macos::setPanelAttachedAsChild(bool attach, QWidget* parent, QWidget* child)
{
    NSView* parentView = (NSView*)parent->winId();
    im_assert(parentView);
    if (!parentView)
        return;

    NSWindow* parentWindow = [parentView window];
    im_assert(parentWindow);

    NSView* childView = (NSView*)child->winId();
    im_assert(childView);
    if (!childView)
        return;

    NSWindow* childWindow = [childView window];
    im_assert(childWindow);
    if (!childWindow)
        return;

    if (attach)
        [parentWindow addChildWindow:childWindow ordered:NSWindowAbove];
    else
        [parentWindow removeChildWindow:childWindow];
}

void platform_macos::setWindowPosition(QWidget& widget, const QRect& widgetRect)
{
    NSView* view = (NSView*)widget.winId();
    im_assert(view);

    NSWindow* window = [view window];
    im_assert(window);

    NSRect rect;
    rect.size.width  = widgetRect.width();
    rect.size.height = widgetRect.height();
    rect.origin.x    = widgetRect.left();
    rect.origin.y    = widgetRect.top();

    [window setFrame:rect display:YES];
}

QRect platform_macos::getWidgetRect(const QWidget& widget)
{
    NSRect rect;
    {
        // get parent window rect
        NSView* view = (NSView*)widget.winId();
        im_assert(view);
        if (!view)
            return QRect();

        NSWindow* window = [view window];
        im_assert(window);
        if (!window)
            return QRect();

        rect = [window frame];
    }
    return QRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}

void setAspectRatioForWindow(QWidget& wnd, float w, float h)
{
    NSView* view = (NSView*)wnd.winId();
    im_assert(view);
    if (view)
    {
        NSWindow* window = [view window];
        im_assert(window);
        if (window)
        {
            if (w > 0.0f)
                [window setContentAspectRatio: NSMakeSize(w, h)];
            else
                [window setResizeIncrements: NSMakeSize(1.0, 1.0)];
        }
    }
}

void platform_macos::setAspectRatioForWindow(QWidget& wnd, float aspectRatio)
{
    setAspectRatioForWindow(wnd, 10.0f * aspectRatio, 10.0f);
}

void platform_macos::unsetAspectRatioForWindow(QWidget& wnd)
{
    setAspectRatioForWindow(wnd, 0.0f, 0.0f);
}

bool platform_macos::windowIsOnActiveSpace(QWidget* _widget)
{
    if (_widget)
        if (auto wnd = [reinterpret_cast<NSView *>(_widget->winId()) window])
            return [wnd isOnActiveSpace];

    return false;
}

bool platform_macos::windowIsOverlapped(QWidget* _frame, const std::vector<QWidget*>& _exclude)
{
    im_assert(!!_frame);
    if (!_frame)
        return false;

    NSView* view = (NSView*)_frame->winId();
    im_assert(!!view);
    if (!view)
        return false;

    NSWindow* window = [view window];
    im_assert(!!window);
    if (!window)
        return false;

    QRect screenGeometry = QDesktopWidget().availableGeometry(_frame);

    const CGWindowID windowID = (CGWindowID)[window windowNumber];
    CGWindowListOption listOptions = kCGWindowListOptionOnScreenAboveWindow;
    listOptions = ChangeBits(listOptions, kCGWindowListExcludeDesktopElements, YES);

    CFArrayRef windowList = CGWindowListCopyWindowInfo(listOptions, windowID);
    NSMutableArray* prunedWindowList = [NSMutableArray array];
    WindowListApplierData* windowListData = [[WindowListApplierData alloc] initWindowListData:prunedWindowList];

    CFArrayApplyFunction(windowList, CFRangeMake(0, CFArrayGetCount(windowList)), &WindowListApplierFunction, (__bridge void *)(windowListData));

    // Flip y coord.
    const QRect frameRect([window frame].origin.x, screenGeometry.height() - int([window frame].origin.y + [window frame].size.height), [window frame].size.width, [window frame].size.height);
    const int originalSquare = frameRect.width() * frameRect.height();

    // Fill array with exclude window id.
    QList<CGWindowID> excludeWindowID;
    for (auto widget : _exclude)
    {
        if (widget)
        {
            NSView* view = (NSView*)widget->winId();
            if (view)
            {
                NSWindow* window = [view window];
                if (window)
                    excludeWindowID.push_back((CGWindowID)[window windowNumber]);
            }
        }
    }

    QRegion selfRegion(frameRect);
    for (NSMutableDictionary* params in windowListData.outputArray)
    {
        // Skip if it is excluded window.
        CGWindowID winId = [params[kWindowIDKey] unsignedIntValue];
        if (excludeWindowID.indexOf(winId) != -1)
        {
            continue;
        }
        WindowRect* wr = params[kWindowRectKey];
        QRect rc(wr.rect.origin.x, wr.rect.origin.y, wr.rect.size.width, wr.rect.size.height);
        QRegion wrRegion(rc);
        selfRegion -= wrRegion;
    }

    int remainsSquare = 0;
    const auto remainsRects = selfRegion.rects();
    for (unsigned ix = 0; ix < remainsRects.count(); ++ix)
    {
        const QRect& rc = remainsRects.at(ix);
        remainsSquare += rc.width() * rc.height();
    }

    [windowListData release];
    CFRelease(windowList);

    return remainsSquare < originalSquare*0.6f;
}

bool platform_macos::pauseiTunes()
{
    bool res = false;
    iTunesApplication* iTunes = [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
    if ([iTunes isRunning])
    {
        // pause iTunes if it is currently playing.
        if (iTunesEPlSPlaying == [iTunes playerState])
        {
            [iTunes playpause];
            res = true;
        }
    }
    return res;
}

void platform_macos::playiTunes()
{
    iTunesApplication* iTunes = [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
    if ([iTunes isRunning])
    {
        // play iTunes if it was paused.
        if (iTunesEPlSPaused == [iTunes playerState])
        {
            [iTunes playpause];
        }
    }
}

void platform_macos::moveAboveParentWindow(QWidget& parent, QWidget& child)
{
    NSView* parentView = (NSView*)parent.winId();
    im_assert(parentView);

    NSWindow* window = [parentView window];
    im_assert(window);

    NSView* childView = (NSView*)child.winId();
    im_assert(childView);
    if (!childView)
        return;

    NSWindow* childWnd = [childView window];
    im_assert(childWnd);
    if (!childWnd)
        return;

    [childWnd orderWindow: NSWindowAbove relativeTo: [window windowNumber]];
}

int platform_macos::doubleClickInterval()
{
    return [NSEvent doubleClickInterval] * 1000;
}

void platform_macos::showInWorkspace(QWidget* _w, platform_specific::ShowCallback _showCallback)
{
    auto wnd = [reinterpret_cast<NSView *>(_w->winId()) window];
    [wnd setCollectionBehavior:NSWindowCollectionBehaviorMoveToActiveSpace];
    if (_showCallback)
        _showCallback();
}

NSWindow* getNSWindow(QWidget& parentWindow)
{
    NSWindow* res = nil;
    NSView* view = (NSView*)parentWindow.winId();
    im_assert(view);
    if (view)
        res = [view window];
    return res;
}

platform_macos::FullScreenNotificaton::FullScreenNotificaton (QWidget& parentWindow) : _delegate(nil), _parentWindow(parentWindow)
{
    NSWindow* window = getNSWindow(_parentWindow);
    im_assert(!!window);
    if (window)
    {
        FullScreenDelegate* delegate = [[FullScreenDelegate alloc] init: this withOldDelegate: [window delegate]];
        [window setDelegate: delegate];
        _delegate = delegate;
        //[delegate retain];
    }
}

platform_macos::FullScreenNotificaton::~FullScreenNotificaton ()
{
    NSWindow* window = getNSWindow(_parentWindow);
    if (window)
    {
        if (_delegate)
        {
            // return old delegate back.
            [window setDelegate: [(FullScreenDelegate*)_delegate getDelegate]];
        } else
        {
            [window setDelegate: nil];
        }
    }
    if (_delegate != nil)
        [(FullScreenDelegate*)_delegate release];
}



namespace platform_macos {

class GraphicsPanelMacosImpl : public platform_specific::GraphicsPanel {
    std::vector<QPointer<Ui::BaseVideoPanel>> _panels;
    NSView* _renderView = nullptr;
    NSView* _parentView = nullptr;

    void moveEvent(QMoveEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void mouseMoveEvent(QMouseEvent* _e) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

    void enableMouseEvents(bool enabled) override;
    void initNative(platform_specific::ViewResize _mode, QSize _size = {}) override;
    void freeNative() override;

    void _setPanelsAttached(bool attach);
    void _mouseMoveEvent(QMouseEvent* _e);

public:
    GraphicsPanelMacosImpl(QWidget* parent, std::vector<QPointer<Ui::BaseVideoPanel>>& panels, bool primaryVideo, bool titleBar);
    virtual ~GraphicsPanelMacosImpl();

    WId frameId() const override;

    void addPanels(std::vector<QPointer<Ui::BaseVideoPanel> >& panels) override;
    void fullscreenModeChanged(bool fullscreen) override;
    void clearPanels() override;

    void fullscreenAnimationStart() override;
    void fullscreenAnimationFinish() override;
    void windowWillDeminiaturize() override;
    void windowDidDeminiaturize()  override;
    void setOpacity(double _opacity) override;

    QPoint mouseMovePoint;
    bool primaryVideo_;
    bool enableMouseEvents_ = false;
};

GraphicsPanelMacosImpl::GraphicsPanelMacosImpl(QWidget* parent, std::vector<QPointer<Ui::BaseVideoPanel>>& panels, bool primaryVideo, bool titleBar)
    : platform_specific::GraphicsPanel(parent)
    , _panels(panels)
    , primaryVideo_ (primaryVideo)
{
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
    setAttribute(Qt::WA_UpdatesDisabled);

    {
        _parentView = (NSView*)(parent ? parent->winId() : winId());
        im_assert(_parentView);

        NSWindow* window = [_parentView window];
        im_assert(window);

        if (!titleBar)
        {
            [[window standardWindowButton:NSWindowCloseButton] setHidden:YES];
            [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
            [[window standardWindowButton:NSWindowZoomButton] setHidden:YES];

            [window setValue:@(YES) forKey:@"titlebarAppearsTransparent"];
            [window setValue:@(1) forKey:@"titleVisibility"];
            window.styleMask |= (1 << 15);//NSFullSizeContentViewWindowMask;
        }

        window.movableByWindowBackground  = NO;
        const auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALBLACK_PERMANENT);

        window.backgroundColor = [NSColor
                colorWithCalibratedRed: color.redF()
                green: color.greenF()
                blue: color.blueF()
                alpha: color.alphaF()];

        window.contentView.autoresizesSubviews = YES;
        _parentView.autoresizesSubviews = YES;
    }
}

GraphicsPanelMacosImpl::~GraphicsPanelMacosImpl() {
    if (_renderView)
        [_renderView release];
}

WId GraphicsPanelMacosImpl::frameId() const {
    return (WId)_renderView;
}

void GraphicsPanelMacosImpl::fullscreenModeChanged(bool fullscreen) {
    NSView* parentView = (NSView*)winId();
    im_assert(parentView);
    if (!parentView)
        return;

    NSWindow* window = [parentView window];
    im_assert(window);
    if (!window)
        return;
}

void GraphicsPanelMacosImpl::moveEvent(QMoveEvent* e)
{
    platform_specific::GraphicsPanel::moveEvent(e);
}

void GraphicsPanelMacosImpl::resizeEvent(QResizeEvent* e)
{
    platform_specific::GraphicsPanel::resizeEvent(e);

    const QRect windowRc = rect();
    NSRect frame;
    frame.origin.x    = windowRc.left();
    frame.origin.y    = windowRc.top();
    frame.size.width  = windowRc.width();
    frame.size.height = windowRc.height();
}

void GraphicsPanelMacosImpl::_setPanelsAttached(bool attach)
{
    if (!parent())
        return;
    for (unsigned ix = 0; ix < _panels.size(); ix++) {
        im_assert(_panels[ix]);
        if (!_panels[ix]) { continue; }
        setPanelAttachedAsChild(attach, parentWidget(), _panels[ix].data());
    }
}

void GraphicsPanelMacosImpl::showEvent(QShowEvent* e)
{
    platform_specific::GraphicsPanel::showEvent(e);
    _setPanelsAttached(true);

    if (primaryVideo_)
    {
        // To catch mouse move event. under Mac.
        QCoreApplication::instance()->installEventFilter(this);
    }
}

void GraphicsPanelMacosImpl::hideEvent(QHideEvent* e)
{
    platform_specific::GraphicsPanel::hideEvent(e);
    _setPanelsAttached(false);

    if (primaryVideo_)
        QCoreApplication::instance()->removeEventFilter(this);
}

void GraphicsPanelMacosImpl::mousePressEvent(QMouseEvent * event)
{
    if (!_renderView)
        return;
    if (enableMouseEvents_ && primaryVideo_ && event->button() == Qt::LeftButton)
    {
        NSEventType evtType = (event->button() == Qt::LeftButton ? NSLeftMouseDown : NSRightMouseDown);
        NSPoint where;
        where.x = event->pos().x();
        // Cocoa coords is inversted.
        where.y = height() - event->pos().y();

        NSEvent *mouseEvent = [NSEvent mouseEventWithType:evtType
                                                 location:where
                                            modifierFlags:0
                                                timestamp:0
                                             windowNumber:0
                                                  context:0
                                              eventNumber:0
                                               clickCount:1
                                                 pressure:0];
        NSView* htView = [_renderView hitTest:where];
        [htView mouseDown:mouseEvent];
    }
    // To pass event to parent widget.
    event->ignore();
}

void GraphicsPanelMacosImpl::mouseReleaseEvent(QMouseEvent * event)
{
    if (!_renderView)
        return;
    if (enableMouseEvents_ && primaryVideo_ && event->button() == Qt::LeftButton)
    {
        NSEventType evtType = (event->button() == Qt::LeftButton ? NSLeftMouseUp : NSRightMouseUp);
        NSPoint where;
        where.x = event->pos().x();
        where.y = height() - event->pos().y();

        NSEvent *mouseEvent = [NSEvent mouseEventWithType:evtType
                                                 location:where
                                            modifierFlags:0
                                                timestamp:0
                                             windowNumber:0
                                                  context:0
                                              eventNumber:0
                                               clickCount:1
                                                 pressure:0];
        NSView* htView = [_renderView hitTest:where];
        [htView mouseUp:mouseEvent];
    }
    event->ignore();
}

void GraphicsPanelMacosImpl::mouseMoveEvent(QMouseEvent* _e)
{
    _mouseMoveEvent (_e);
    _e->ignore();
}

void GraphicsPanelMacosImpl::_mouseMoveEvent(QMouseEvent* _e)
{
    if (!_renderView)
        return;
    QPoint localPos = mapFromGlobal(QCursor::pos());
    if (enableMouseEvents_ && primaryVideo_ && mouseMovePoint != localPos)
    {
        mouseMovePoint = localPos;
        NSEventType evtType = NSMouseMoved;
        NSPoint where;
        where.x = _e->pos().x();
        where.y = height() - _e->pos().y();

        if (where.x >= 0 && where.x < width() && where.y >= 0 && where.y < height())
        {
            int nClickCount = (_e->buttons() == Qt::LeftButton) ? 1 : 0;
            NSEvent *mouseEvent = [NSEvent mouseEventWithType:evtType
                                                     location:where
                                                modifierFlags:0
                                                    timestamp:0
                                                 windowNumber:0
                                                      context:0
                                                  eventNumber:0
                                                   clickCount: nClickCount
                                                     pressure:0];
            NSView* htView = [_renderView hitTest:where];
            if (nClickCount == 0)
                [htView mouseMoved:mouseEvent];
            else
                [htView mouseDragged:mouseEvent];
        }
    }
}

void GraphicsPanelMacosImpl::addPanels(std::vector<QPointer<Ui::BaseVideoPanel>>& panels)
{
    _panels = panels;
    _setPanelsAttached(true);
}

void GraphicsPanelMacosImpl::fullscreenAnimationStart()
{
    // Commented, because on 10.13 with this line we have problem with top most video panels.
    //_setPanelsAttached(false);
    moveAboveParentWindow(*(QWidget*)parent(), *this);

    if (!_renderView)
        return;
    if ([_renderView respondsToSelector:@selector(startFullScreenAnimation)]) {
        [_renderView startFullScreenAnimation];
    }
}

void GraphicsPanelMacosImpl::fullscreenAnimationFinish()
{
    // Commented, because on 10.13 with this line we have problem with top most video panels.
    //_setPanelsAttached(true);

    if (!_renderView)
        return;
    if ([_renderView respondsToSelector:@selector(finishFullScreenAnimation)]) {
        [_renderView finishFullScreenAnimation];
    }
}

void GraphicsPanelMacosImpl::windowWillDeminiaturize()
{
    // Stop render while deminiaturize animation is running. Fix artifacts.
    if (!_renderView)
        return;
    if ([_renderView respondsToSelector:@selector(finishFullScreenAnimation)]) {
        [_renderView startFullScreenAnimation];
    }
}

void GraphicsPanelMacosImpl::windowDidDeminiaturize()
{
    // Start render on deminiatureze finished.
    if (!_renderView)
        return;
    if ([_renderView respondsToSelector:@selector(finishFullScreenAnimation)]) {
        [_renderView finishFullScreenAnimation];
    }
}

void GraphicsPanelMacosImpl::clearPanels()
{
    _setPanelsAttached(false);
    _panels.clear();
}

bool GraphicsPanelMacosImpl::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->buttons() == Qt::NoButton)
        {
            _mouseMoveEvent(mouseEvent);
        }
    }
    return false;
}

void GraphicsPanelMacosImpl::enableMouseEvents(bool enable)
{
    enableMouseEvents_ = enable;
}

void GraphicsPanelMacosImpl::initNative(platform_specific::ViewResize _mode, QSize)
{
    if (_renderView)
        return;
    NSRect frame = [_parentView frame];
    // We resize only videostream in mini-window with proper aspectRatio
    // and add 1px for correct interraction while _renderView is 'invisible'
    if (_mode == platform_specific::ViewResize::Adjust)
        frame.size.height = height() + Utils::scale_value(1);
    
    _renderView = [VOIPMTLRenderView isSupported] ?  [[VOIPMTLRenderView alloc] initWithFrame:frame] : [[VOIPRenderViewOGL alloc] initWithFrame:frame];
    [_parentView addSubview:_renderView];
}

void GraphicsPanelMacosImpl::freeNative()
{
    im_assert(_renderView);
    if (!_renderView)
        return;
    [_renderView removeFromSuperview];
    [_renderView release];
    _renderView = nullptr;
}

void GraphicsPanelMacosImpl::setOpacity(double _opacity)
{
    NSView* view = (NSView*)winId();
    im_assert(view);
    if (!view)
        return;

    NSWindow* window = [view window];
    im_assert(window);
    if (!window)
        return;
    [window setAlphaValue:_opacity];
}

}

platform_specific::GraphicsPanel* platform_macos::GraphicsPanelMacos::create(QWidget* parent, std::vector<QPointer<Ui::BaseVideoPanel>>& panels, bool primaryVideo, bool titleBar) {

    IOPMAssertionID im_assertionID = 0;
    if (CGDisplayIsAsleep(CGMainDisplayID()))
    {
        CFStringRef reasonForActivity = CFSTR("ICQ Call is active");
        IOPMAssertionDeclareUserActivity(reasonForActivity, kIOPMUserActiveLocal, &im_assertionID);
        // Give 1s to monitor to turn on.
        QThread::msleep(1000);
    }
    platform_specific::GraphicsPanel* res = new platform_macos::GraphicsPanelMacosImpl(parent, panels, primaryVideo, titleBar);
    if (im_assertionID)
    {
        IOPMAssertionRelease((IOPMAssertionID)im_assertionID);
    }
    return res;
}
