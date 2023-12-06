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

int
MyView::findPointId(const Vector2f &lp, float epsilon) const{
    return -1;
}

bool
MyView::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers)
{
    auto lp = pos_to_pixel(p - m_pos);
    if (isInsideImage(lp)) {
//        std::cout << "mouse_button_event: p=[" << pos_to_pixel(p - m_pos) << "], button=[" << button
//        << "], isdown: " << down << std::endl;
        auto pointId = findPointId(lp);
        if (pointId>=0) {
            std::cout << "Point clicked: " << pointId << std::endl;
            return true;
        }
    }
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
        auto pointId = findPointId(lp);
        if (pointId>=0) {
            switch (button) {
                case 1: //left click
                    // if is on a point
                    std::cout << "Move point by [" << rel << "]" << std::endl;
                    break;
                case 2: //right click
                    // if is on a point
                    std::cout << "Change normal by [" << rel << "]" << std::endl;
                    break;
                default:
                    break;
            }
        }
    }
    return true;
}