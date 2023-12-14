#include "myview.h"

#include <nanogui/texture.h>


using namespace nanogui;

MyView::MyView(nanogui::Widget *parent, DataManager* mgr) : ImageView(parent), m_dataMgr(mgr) {
    std::cout<< "Controls:\n"
             << "  scroll: zoom in/out\n"
             << "  left click: move point\n"
             << "  right click + move: rotate point normal"
             << "  ctrl+click: invert normal\n"
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

bool
MyView::fitImage() {
    if (!m_enabled || !m_image)
        return false;

    const auto iSize = Vector2f (m_image->size());
    const auto wSize = Vector2f (size() );

    m_scale = 5.f*std::min(wSize.x() / iSize.x(), wSize.y() / iSize.y());
    return true;
}

//#define USE_KDTREE
int
MyView::findPointId(const Vector2f &lp) const{
    const auto& points = m_dataMgr->getPointContainer();
    if(! points.empty()) {
        int i = 0;
#ifndef USE_KDTREE
        for(const auto&p : points) {
            Vector2f query(p.x(), p.y());
            if (norm((lp - query)) <= m_selectionThreshold)
                return i;
            ++i;
        }
#else
        DataPoint::VectorType query(lp.x(), lp.y());
        const auto& tree = m_dataMgr->getKdTree();
        auto res = tree.nearest_neighbor(query);
        if (res.begin() != res.end() &&
           (query-tree.point_data()[res.get()].pos()).norm()<m_selectionThreshold)
            return res.get();
#endif
    }
    return -1;
}

bool
MyView::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers)
{
    auto lp = pos_to_pixel(p - m_pos);
    // left click, on press
    if (down && isInsideImage(lp)) {
        auto pointId = findPointId(lp);
        if (modifiers == 0) { // no modified
            if (pointId < 0) {
                if (button == 0) { // create new point iif left click (button id seems to be different wrt drag event
                    std::cout << "MyView::add new point" << std::endl;
                    m_dataMgr->getPointContainer().emplace_back(lp.x(), lp.y(), DEFAULT_POINT_ANGLE);
                    m_dataMgr->updateKdTree();
                }
            } else {
                m_movedPoint = pointId;
            }
        } else if (modifiers == 2) { // Ctrl
            if (pointId >= 0) {
                std::cout << "Flip normal of point " << pointId << std::endl;
                auto& angle = m_dataMgr->getPointContainer()[pointId].z();
                angle = float(std::fmod(angle + M_PI, 2.*M_PI));
                m_dataMgr->updateKdTree();
            }
        }
        return true;
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
            auto& points = m_dataMgr->getPointContainer();
            switch (button) {
                case 1: //left click
                    // if is on a point
//                    std::cout << "Move point by [" << rel << "]" << std::endl;
                    points[m_movedPoint].x() = lp.x();
                    points[m_movedPoint].y() = lp.y();
                    m_dataMgr->updateKdTree();
                    break;
                case 2: //right click
                {
                    int dist = std::min(std::abs(rel.x()), 50);
                    if (rel.x() < 0) dist *=-1;
                    // if is on a point
                    auto relAngle = std::asin(float(dist) / 50.1f); // move by 40px to get 90 degree angle
                    points[m_movedPoint].z() += relAngle;
//                    std::cout << "Change normal by " << rel << ". Gives angle " << m_points[m_movedPoint].z() << std::endl;
                    m_dataMgr->updateKdTree();
                }
                    break;
                default:
                    break;
            }
        }
    }
    return true;
}
