#include "myview.h"

#include <nanogui/texture.h>

using namespace nanogui;

MyView::MyView(nanogui::Widget *parent) : ImageView(parent) {
    std::cout<< "Controls:\n"
             << "  ctrl+mouse move: move view\n"
             << "  scroll: zoom in/out\n"
             << "  left click: move point\n"
             << "  right click: change point normal"
             << std::endl;
}

bool
MyView::isInsideImage(const Vector2f &lp) const {
    const auto bottom_right = Vector2f(m_image->size());
    if ( lp.x() < 0 || lp.x() >= bottom_right.x() )
        return false;
    if ( lp.y() < 0 || lp.y() >= bottom_right.y() )
        return false;
    return true;
}

//#define USE_KDTREE
int
MyView::findPointId(const Vector2f &lp, float epsilon) const{
    if(! m_points.empty()) {
        int i = 0;
#ifndef USE_KDTREE
        for(const auto&p : m_points) {
            Vector2f query(p.x(), p.y());
            if (norm((lp - query)) <= epsilon)
                return i;
            ++i;
        }
#else
        DataPoint::VectorType query(lp.x(), lp.y());
        auto res = m_tree.nearest_neighbor(query);
        if (res.begin() != res.end() &&
           (query-m_tree.point_data()[res.get()].pos()).norm()<epsilon)
            return res.get();
#endif
    }
    return -1;
}

bool
MyView::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers)
{
    auto lp = pos_to_pixel(p - m_pos);
    if (modifiers==0 && down && isInsideImage(lp)) {
//        std::cout << "mouse_button_event: p=[" << pos_to_pixel(p - m_pos) << "], button=[" << button
//        << "], isdown: " << down << std::endl;
        auto pointId = findPointId(lp);
        if (pointId < 0) { // create new point
            std::cout << "MyView::add new point" << std::endl;
            m_points.emplace_back( lp.x(), lp.y(), 0, 1 );
            updateCollection();
            return true;
        } else {
            m_movedPoint = pointId;
            return true;
        }
    }
    m_movedPoint = -1;
    return ImageView::mouse_button_event(p, button, down, modifiers);
}

bool
MyView::mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers)
{
    if (modifiers == 2) //control
        return ImageView::mouse_drag_event(p, rel, button, modifiers);

    auto lp = pos_to_pixel(p - m_pos);
    if (isInsideImage(lp))
    {
        if (m_movedPoint>=0) {
            switch (button) {
                case 1: //left click
                    // if is on a point
                    std::cout << "Move point by [" << rel << "]" << std::endl;
                    m_points[m_movedPoint].x() = lp.x();
                    m_points[m_movedPoint].y() = lp.y();
                    updateCollection();
                    break;
                case 2: //right click
                    // if is on a point
                    std::cout << "Change normal by [" << rel << "]" << std::endl;
                    updateCollection();
                    break;
                default:
                    break;
            }
        }
    }
    return true;
}

void
MyView::updateCollection() {
    // recompute KdTree
    if(m_points.empty())
        m_tree.clear();
    else
        m_tree.build(m_points );

    m_updateFunction();
}
