#pragma once

#include <nanogui/imageview.h>
#include <nanogui/vector.h>

#include "dataManager.h"

#include <iostream>
#include <optional>
#include <vector>

class MyView : public nanogui::ImageView {
public:
    /// Initialize the widget
    explicit MyView(Widget *parent, DataManager* mgr);

    // Check if a point is inside the image
    bool isInsideImage(const nanogui::Vector2f &lp) const;

    // Set zoom factor to make the image fit the window
    bool fitImage();

    // Check if a point is at this coordinate. If yes, return the point id, -1 otherwise
    // \see m_selectionThreshold
    int findPointId(const nanogui::Vector2f &lp) const;

    // Widget implementation
    /// Handle a mouse button event
    bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

    /// Handle a mouse drag event
    bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

    /// Disable scrolling
    inline bool scroll_event(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override {return true;}

    /// Set selection threshold
    inline void setSelectionThreshold(float dist) { m_selectionThreshold = dist; }

private:
    int m_movedPoint{-1};
    float m_selectionThreshold{2}; // distance in pixel used to select points
    DataManager* m_dataMgr{nullptr};
};
