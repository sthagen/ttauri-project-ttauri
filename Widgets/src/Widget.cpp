// Copyright 2019 Pokitec
// All rights reserved.

#include "TTauri/Widgets/Widget.hpp"
#include "TTauri/GUI/utils.hpp"

namespace TTauri::GUI::Widgets {

Widget::Widget(Window &window, Widget *parent, vec defaultExtent) noexcept :
    window(window),
    parent(parent),
    elevation(parent ? parent->elevation + 1.0f : 0.0f),
    enabled(true, [this](auto...){ forceRedraw = true; })
{
    minimumExtent = defaultExtent;
    minimumWidthConstraint = window.addConstraint(width >= minimumExtent.width());
    minimumHeightConstraint = window.addConstraint(height >= minimumExtent.height());

    preferedExtent = defaultExtent;
    preferedWidthConstraint = window.addConstraint(width >= preferedExtent.width(), rhea::strength::strong());
    preferedHeightConstraint = window.addConstraint(height >= preferedExtent.height(), rhea::strength::strong());
}

Widget::~Widget()
{
    window.removeConstraint(minimumWidthConstraint);
    window.removeConstraint(minimumHeightConstraint);
    window.removeConstraint(preferedWidthConstraint);
    window.removeConstraint(preferedHeightConstraint);
}

Device *Widget::device() const noexcept
{
    auto device = window.device;
    ttauri_assert(device);
    return device;
}

void Widget::setMinimumExtent(vec newMinimumExtent) noexcept
{
    auto lock = std::scoped_lock(mutex);

    if (newMinimumExtent != minimumExtent) {
        minimumExtent = newMinimumExtent;

        minimumWidthConstraint = window.replaceConstraint(
            minimumWidthConstraint,
            width >= minimumExtent.width()
        );

        minimumHeightConstraint = window.replaceConstraint(
            minimumHeightConstraint,
            height >= minimumExtent.height()
        );
    }
}

void Widget::setMinimumExtent(float width, float height) noexcept
{
    setMinimumExtent(vec{width, height});
}

void Widget::setPreferedExtent(vec newPreferedExtent) noexcept
{
    auto lock = std::scoped_lock(mutex);

    if (newPreferedExtent != preferedExtent) {
        preferedExtent = newPreferedExtent;

        preferedWidthConstraint = window.replaceConstraint(
            preferedWidthConstraint,
            width >= preferedExtent.width(),
            rhea::strength::weak()
        );

        preferedHeightConstraint = window.replaceConstraint(
            preferedHeightConstraint,
            height >= preferedExtent.height(),
            rhea::strength::weak()
        );
    }
}

void Widget::setFixedExtent(vec newFixedExtent) noexcept
{
    auto lock = std::scoped_lock(mutex);

    ttauri_assert(newFixedExtent.width() == 0.0f || newFixedExtent.width() >= minimumExtent.width());
    ttauri_assert(newFixedExtent.height() == 0.0f || newFixedExtent.height() >= minimumExtent.height());

    if (newFixedExtent != fixedExtent) {
        if (fixedExtent.width() != 0.0f) {
            window.removeConstraint(fixedWidthConstraint);
        }
        if (fixedExtent.height() != 0.0f) {
            window.removeConstraint(fixedHeightConstraint);
        }
        fixedExtent = newFixedExtent;
        if (fixedExtent.width() != 0.0f) {
            fixedWidthConstraint = window.addConstraint(width == fixedExtent.width());
        }
        if (fixedExtent.height() != 0.0f) {
            fixedHeightConstraint = window.addConstraint(height == fixedExtent.height());
        }
    }
}

void Widget::setFixedHeight(float height) noexcept
{
    setFixedExtent(vec{0.0f, height});
}

void Widget::setFixedWidth(float width) noexcept
{
    setFixedExtent(vec{width, 0.0f});
}



void Widget::placeBelow(Widget const &rhs, float margin) const noexcept {
    window.addConstraint(this->top + margin == rhs.bottom);
}

void Widget::placeAbove(Widget const &rhs, float margin) const noexcept {
    window.addConstraint(this->bottom == rhs.top + margin);
}

void Widget::placeLeftOf(Widget const &rhs, float margin) const noexcept {
    window.addConstraint(this->right + margin == rhs.left);
}

void Widget::placeRightOf(Widget const &rhs, float margin) const noexcept {
    window.addConstraint(this->left == rhs.right + margin);
}

void Widget::placeAtTop(float margin) const noexcept {
    window.addConstraint(this->top + margin == parent->top);
}

void Widget::placeAtBottom(float margin) const noexcept {
    window.addConstraint(this->bottom - margin == parent->bottom);
}

void Widget::placeLeft(float margin) const noexcept {
    window.addConstraint(this->left - margin == parent->left);
}

void Widget::placeRight(float margin) const noexcept {
    window.addConstraint(this->right + margin == parent->right);
}

bool Widget::widthOrHeightValueHasChanged() const noexcept
{
    auto lock = std::scoped_lock(window.widgetSolverMutex);

    let widthValue = width.value();
    let heightValue = height.value();

    auto changed = false;
    changed |= widthValue != widthChangePreviousValue;
    changed |= heightValue != heightChangePreviousValue;

    widthChangePreviousValue = widthValue;
    heightChangePreviousValue = heightValue;

    return changed;
}

int Widget::needs(hires_utc_clock::time_point displayTimePoint) const noexcept
{
    auto layout = forceLayout.exchange(false, std::memory_order::memory_order_relaxed);
    layout |= widthOrHeightValueHasChanged();
    auto redraw = layout;
    redraw |= forceRedraw.exchange(false, std::memory_order::memory_order_relaxed);

    auto need =
        (static_cast<int>(layout) << 1) |
        static_cast<int>(redraw);

    return need;
}

aarect Widget::makeWindowRectangle() const noexcept
{
    auto lock = std::scoped_lock(window.widgetSolverMutex);

    return round(aarect{
        numeric_cast<float>(left.value()),
        numeric_cast<float>(bottom.value()),
        numeric_cast<float>(width.value()),
        numeric_cast<float>(height.value())
    });
}

void Widget::layout(hires_utc_clock::time_point displayTimePoint) noexcept
{
    auto lock = std::scoped_lock(mutex);

    let _windowRectangle = makeWindowRectangle();
    setExtent(_windowRectangle.extent());
    setOffsetFromWindow(_windowRectangle.offset());

    setOffsetFromParent(
        parent ?
            _windowRectangle.offset() - parent->offsetFromWindow():
            _windowRectangle.offset()
    );
        
    fromWindowTransform = mat::T(-_windowRectangle.x(), -_windowRectangle.y(), -z());
    toWindowTransform = mat::T(_windowRectangle.x(), _windowRectangle.y(), z());

    forceRedraw = true;
}

int Widget::layoutChildren(hires_utc_clock::time_point displayTimePoint, bool force) noexcept
{
    auto lock = std::scoped_lock(mutex);

    auto total_need = 0;

    for (auto &&child: children) {
        let child_need = child->needs(displayTimePoint);
        total_need |= child_need;

        if (force || child_need >= 2) {
            child->layout(displayTimePoint);
        }

        total_need |= child->layoutChildren(displayTimePoint, force);
    }

    return total_need;
}

void Widget::draw(DrawContext const &drawContext, hires_utc_clock::time_point displayTimePoint) noexcept
{
    auto lock = std::scoped_lock(mutex);

    auto childContext = drawContext;
    for (auto &child : children) {
        childContext.clippingRectangle = child->clippingRectangle();
        childContext.transform = child->toWindowTransform;

        // The default fill and border colors.
        let childNestingLevel = child->nestingLevel();
        childContext.color = theme->borderColor(childNestingLevel);
        childContext.fillColor = theme->fillColor(childNestingLevel);

        if (child->enabled) {
            if (child->focus) {
                childContext.color = theme->accentColor;
            } else if (child->hover) {
                childContext.color = theme->borderColor(childNestingLevel + 1);
            }

            if (child->hover) {
                childContext.fillColor = theme->fillColor(childNestingLevel + 1);
            }

        } else {
            // Disabled, only the outline is shown.
            childContext.color = theme->borderColor(childNestingLevel - 1);
            childContext.fillColor = theme->fillColor(childNestingLevel - 1);
        }

        child->draw(childContext, displayTimePoint);
    }
}

void Widget::handleCommand(string_ltag command) noexcept {
    auto lock = std::scoped_lock(mutex);

    if (command == "gui.widget.next"_ltag) {
        window.updateToNextKeyboardTarget(this);
    } else if (command == "gui.widget.prev"_ltag) {
        window.updateToPrevKeyboardTarget(this);
    }
}

HitBox Widget::hitBoxTest(vec position) const noexcept
{
    auto lock = std::scoped_lock(mutex);

    auto r = rectangle().contains(position) ?
        HitBox{this, elevation} :
        HitBox{};

    for (let &child : children) {
        r = std::max(r, child->hitBoxTest(position - child->offsetFromParent()));
    }
    return r;
}

}
