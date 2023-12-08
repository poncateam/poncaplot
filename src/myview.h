#pragma once

#include <nanogui/imageview.h>
#include <nanogui/vector.h>

#include <Ponca/SpatialPartitioning>

#include <iostream>
#include <optional>
#include <vector>

class DataPoint
{
public:
    enum {Dim = 2};
    using Scalar = float;
    using VectorType = Eigen::Vector<Scalar,Dim>;
    inline const auto& pos() const {return m_pos;}
    inline const auto& normal() const {return m_normal;}
    /// \fixme Use maps to avoid duplication
    explicit inline DataPoint(const nanogui::Vector4f &pn)
    : m_pos({pn.x(), pn.y()}), m_normal({pn.z(), pn.w()}) {}
private:
    VectorType m_pos, m_normal;
};


class MyView : public nanogui::ImageView {
public:
    using PointCollection = Ponca::KdTree<DataPoint>;

    /// Initialize the widget
    explicit MyView(Widget *parent);

    // Check if a point is inside the image
    bool isInsideImage(const nanogui::Vector2f &lp) const;

    // Set zoom factor to make the image fit the window
    bool fitImage();

    // Check if a point is at this coordinate. If yes, return the point id, -1 otherwise
    int findPointId(const nanogui::Vector2f &lp, float epsilon = 3.f) const;

    // Widget implementation
    /// Handle a mouse button event
    bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

    /// Handle a mouse drag event
    bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

    /// Disable scrolling
    inline bool scroll_event(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override {return true;}

    /// Read access to point collection
    inline const PointCollection& getPointCollection() const { return m_tree; }

    /// Set Update function, called after each point update
    inline void setUpdateFunction(std::function<void()> &&f) { m_updateFunction = f; }

private:
    inline void updateCollection();

    using PointContainer  = std::vector<nanogui::Vector4f>; // stores x,y,nx,ny
    PointContainer m_points;
    PointCollection m_tree;
    std::function<void()> m_updateFunction {[](){}};
    int m_movedPoint{-1};
};
