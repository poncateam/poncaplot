#pragma once

#include <nanogui/imageview.h>
#include <nanogui/vector.h>

#include <iostream>
#include <optional>
#include <vector>


class MyView : public nanogui::ImageView {
public:
    using PointCollection = std::vector<nanogui::Vector2f>;

    /// Initialize the widget
    explicit MyView(Widget *parent);

    // Check if a point is inside the image
    bool isInsideImage(const nanogui::Vector2f &lp) const;

    // Check if a point is at this coordinate. If yes, return the point id, -1 otherwise
    int findPointId(const nanogui::Vector2f &lp, float epsilon = 1.f) const;

    // Widget implementation
    /// Handle a mouse button event
    bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

    /// Handle a mouse drag event
    bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

    /// Read access to point collection
    inline const PointCollection& getPointCollection() const { return m_points; }

    /// Set Update function, called after each point update
    inline void setUpdateFunction(std::function<void()> &&f) { m_updateFunction = f; }

private:
    PointCollection m_points;
    std::function<void()> m_updateFunction {[](){}};
    int m_movedPoint{-1};
};
