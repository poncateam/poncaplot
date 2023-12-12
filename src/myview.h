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
    using MatrixType = Eigen::Matrix<Scalar,Dim,Dim>;
    [[nodiscard]] inline const auto& pos() const {return m_pos;}
    [[nodiscard]] inline const auto& normal() const {return m_normal;}
    /// \fixme Use maps to avoid duplication
    explicit inline DataPoint(const nanogui::Vector3f &pn)
    : m_pos({pn.x(), pn.y()}), m_normal({std::cos(pn.z()), std::sin(pn.z())}) {}
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
    // \see m_selectionThreshold
    int findPointId(const nanogui::Vector2f &lp) const;

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

    /// Set selection threshold
    inline void setSelectionThreshold(float dist) { m_selectionThreshold = dist; }

private:
    inline void updateCollection();

    using PointContainer  = std::vector<nanogui::Vector3f>; // stores x,y,normal angle in radians
    PointContainer m_points;
    PointCollection m_tree;
    std::function<void()> m_updateFunction {[](){}};
    int m_movedPoint{-1};
    float m_selectionThreshold{2}; // distance in pixel used to select points
};
