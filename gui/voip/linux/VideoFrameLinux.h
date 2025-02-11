#ifndef __VIDEO_FRAME_LINUX_H__
#define __VIDEO_FRAME_LINUX_H__

#include <QWidget>
#include "../VideoFrame.h"
#include "glad.h"
#include "glfw3.h"
#include <mutex>

namespace Ui
{
    class BaseVideoPanel;
}

namespace platform_linux
{
    void showOnCurrentDesktop(platform_specific::ShowCallback _callback);

class GraphicsPanelLinux : public platform_specific::GraphicsPanel
{
    Q_OBJECT
public:
    GraphicsPanelLinux(QWidget* _parent, std::vector<QPointer<Ui::BaseVideoPanel>>& _panels, bool primaryVideo);
    virtual ~GraphicsPanelLinux();

    WId frameId() const override;
    void resizeEvent(QResizeEvent * event) override;
    void initNative(platform_specific::ViewResize _mode, QSize _size = {}) override;
    void freeNative() override;
    void setOpacity(double _opacity) override;

private:
    static std::unordered_map<GLFWwindow*, QPointer<QWidget>> parents_;

    static void keyCallback(GLFWwindow* _window, int _key, int _scancode, int _actions, int _mods);

    bool isOpacityAvailable() const;

private:
    GLFWwindow* videoWindow_ = nullptr;
    QWidget* parent_ = nullptr;
};

}

#endif//__VIDEO_FRAME_WIN32_H__
