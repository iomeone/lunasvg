#include "graphics.h"
#include "lunasvg.h"

#include <cfloat>
#include <cmath>

namespace lunasvg {

const Color Color::Black(0xFF000000);
const Color Color::White(0xFFFFFFFF);
const Color Color::Transparent(0x00000000);

const Rect Rect::Empty(0, 0, 0, 0);
const Rect Rect::Invalid(0, 0, -1, -1);
const Rect Rect::Infinite(-FLT_MAX / 2.f, -FLT_MAX / 2.f, FLT_MAX, FLT_MAX);

Rect::Rect(const Box& box)
    : x(box.x), y(box.y), w(box.w), h(box.h)
{
}

const Transform Transform::Identity(1, 0, 0, 1, 0, 0);

Transform::Transform()
{
    plutovg_matrix_init_identity(&m_matrix);
}

Transform::Transform(float a, float b, float c, float d, float e, float f)
{
    plutovg_matrix_init(&m_matrix, a, b, c, d, e, f);
}

Transform::Transform(const Matrix& matrix)
    : Transform(matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f)
{
}

Transform Transform::operator*(const Transform& transform) const
{
    plutovg_matrix_t result;
    plutovg_matrix_multiply(&result, &transform.m_matrix, &m_matrix);
    return result;
}

Transform& Transform::operator*=(const Transform& transform)
{
    return (*this = *this * transform);
}

Transform& Transform::multiply(const Transform& transform)
{
    return (*this = *this * transform);
}

Transform& Transform::rotate(float angle)
{
    return multiply(rotated(angle));
}

Transform& Transform::rotate(float angle, float cx, float cy)
{
    return multiply(rotated(angle, cx, cy));
}

Transform& Transform::scale(float sx, float sy)
{
    return multiply(scaled(sx, sy));
}

Transform& Transform::shear(float shx, float shy)
{
    return multiply(sheared(shx, shy));
}

Transform& Transform::translate(float tx, float ty)
{
    return multiply(translated(tx, ty));
}

Transform& Transform::postMultiply(const Transform& transform)
{
    return (*this = transform * *this);
}

Transform& Transform::postRotate(float angle)
{
    return postMultiply(rotated(angle));
}

Transform& Transform::postRotate(float angle, float cx, float cy)
{
    return postMultiply(rotated(angle, cx, cy));
}

Transform& Transform::postScale(float sx, float sy)
{
    return postMultiply(scaled(sx, sy));
}

Transform& Transform::postShear(float shx, float shy)
{
    return postMultiply(sheared(shx, shy));
}

Transform& Transform::postTranslate(float tx, float ty)
{
    return postMultiply(translated(tx, ty));
}

Transform Transform::inverse() const
{
    plutovg_matrix_t inverse;
    plutovg_matrix_invert(&m_matrix, &inverse);
    return inverse;
}

Transform& Transform::invert()
{
    plutovg_matrix_invert(&m_matrix, &m_matrix);
    return *this;
}

void Transform::reset()
{
    plutovg_matrix_init_identity(&m_matrix);
}

Point Transform::mapPoint(float x, float y) const
{
    plutovg_matrix_map(&m_matrix, x, y, &x, &y);
    return Point(x, y);
}

Point Transform::mapPoint(const Point& point) const
{
    return mapPoint(point.x, point.y);
}

Rect Transform::mapRect(const Rect& rect) const
{
    if(!rect.isValid()) {
        return Rect::Invalid;
    }

    plutovg_rect_t result = {rect.x, rect.y, rect.w, rect.h};
    plutovg_matrix_map_rect(&m_matrix, &result, &result);
    return result;
}

float Transform::xScale() const
{
    return std::sqrt(m_matrix.a * m_matrix.a + m_matrix.b * m_matrix.b);
}

float Transform::yScale() const
{
    return std::sqrt(m_matrix.c * m_matrix.c + m_matrix.d * m_matrix.d);
}

bool Transform::parse(const char* data, size_t length)
{
    return plutovg_matrix_parse(&m_matrix, data, length);
}

Transform Transform::rotated(float angle)
{
    plutovg_matrix_t matrix;
    plutovg_matrix_init_rotate(&matrix, PLUTOVG_DEG2RAD(angle));
    return matrix;
}

Transform Transform::rotated(float angle, float cx, float cy)
{
    plutovg_matrix_t matrix;
    plutovg_matrix_init_translate(&matrix, cx, cy);
    plutovg_matrix_rotate(&matrix, PLUTOVG_DEG2RAD(angle));
    plutovg_matrix_translate(&matrix, -cx, -cy);
    return matrix;
}

Transform Transform::scaled(float sx, float sy)
{
    plutovg_matrix_t matrix;
    plutovg_matrix_init_scale(&matrix, sx, sy);
    return matrix;
}

Transform Transform::sheared(float shx, float shy)
{
    plutovg_matrix_t matrix;
    plutovg_matrix_init_shear(&matrix, PLUTOVG_DEG2RAD(shx), PLUTOVG_DEG2RAD(shy));
    return matrix;
}

Transform Transform::translated(float tx, float ty)
{
    plutovg_matrix_t matrix;
    plutovg_matrix_init_translate(&matrix, tx, ty);
    return matrix;
}

static plutovg_path_t* defaultPathData()
{
    thread_local plutovg_path_t* path = nullptr;
    if(path == nullptr)
        path = plutovg_path_create();
    return plutovg_path_reference(path);
}

Path::Path()
    : m_data(defaultPathData())
{
}

Path::Path(const Path& path)
    : m_data(plutovg_path_reference(path.data()))
{
}

Path::Path(Path&& path)
    : m_data(path.release())
{
}

Path::~Path()
{
    plutovg_path_destroy(m_data);
}

Path& Path::operator=(const Path& path)
{
    Path(path).swap(*this);
    return *this;
}

Path& Path::operator=(Path&& path)
{
    Path(std::move(path)).swap(*this);
    return *this;
}

void Path::moveTo(float x, float y)
{
    plutovg_path_move_to(ensure(), x, y);
}

void Path::lineTo(float x, float y)
{
    plutovg_path_line_to(ensure(), x, y);
}

void Path::quadTo(float x1, float y1, float x2, float y2)
{
    plutovg_path_quad_to(ensure(), x1, y1, x2, y2);
}

void Path::cubicTo(float x1, float y1, float x2, float y2, float x3, float y3)
{
    plutovg_path_cubic_to(ensure(), x1, y1, x2, y2, x3, y3);
}

void Path::arcTo(float rx, float ry, float xAxisRotation, bool largeArcFlag, bool sweepFlag, float x, float y)
{
    plutovg_path_arc_to(ensure(), rx, ry, PLUTOVG_DEG2RAD(xAxisRotation), largeArcFlag, sweepFlag, x, y);
}

void Path::close()
{
    plutovg_path_close(ensure());
}

void Path::addEllipse(float cx, float cy, float rx, float ry)
{
    plutovg_path_add_ellipse(ensure(), cx, cy, rx, ry);
}

void Path::addRoundRect(float x, float y, float w, float h, float rx, float ry)
{
    plutovg_path_add_round_rect(ensure(), x, y, w, h, rx, ry);
}

void Path::addRect(float x, float y, float w, float h)
{
    plutovg_path_add_rect(ensure(), x, y, w, h);
}

void Path::addEllipse(const Point& center, const Size& radii)
{
    addEllipse(center.x, center.y, radii.w, radii.h);
}

void Path::addRoundRect(const Rect& rect, const Size& radii)
{
    addRoundRect(rect.x, rect.y, rect.w, rect.h, radii.w, radii.h);
}

void Path::addRect(const Rect& rect)
{
    addRect(rect.x, rect.y, rect.w, rect.h);
}

void Path::reset()
{
    plutovg_path_reset(ensure());
}

Rect Path::boundingRect() const
{
    plutovg_rect_t extents;
    plutovg_path_extents(m_data, &extents);
    return extents;
}

bool Path::isEmpty() const
{
    return plutovg_path_get_elements(m_data, nullptr) == 0;
}

bool Path::isUnique() const
{
    return plutovg_path_get_reference_count(m_data) == 1;
}

bool Path::parse(const char* data, size_t length)
{
    plutovg_path_reset(ensure());
    return plutovg_path_parse(ensure(), data, length);
}

plutovg_path_t* Path::ensure()
{
    if(!isUnique())
        m_data = plutovg_path_clone(m_data);
    return m_data;
}

PathIterator::PathIterator(const Path& path)
    : m_size(plutovg_path_get_elements(path.data(), &m_elements))
    , m_index(0)
{
}

PathCommand PathIterator::currentSegment(std::array<Point, 3>& points) const
{
    auto command = m_elements[m_index].header.command;
    switch(command) {
    case PLUTOVG_PATH_COMMAND_MOVE_TO:
        points[0] = m_elements[m_index + 1].point;
        break;
    case PLUTOVG_PATH_COMMAND_LINE_TO:
        points[0] = m_elements[m_index + 1].point;
        break;
    case PLUTOVG_PATH_COMMAND_CUBIC_TO:
        points[0] = m_elements[m_index + 1].point;
        points[1] = m_elements[m_index + 2].point;
        points[2] = m_elements[m_index + 3].point;
        break;
    case PLUTOVG_PATH_COMMAND_CLOSE:
        points[0] = m_elements[m_index + 1].point;
        break;
    }

    return PathCommand(command);
}

void PathIterator::next()
{
    m_index += m_elements[m_index].header.length;
}

std::shared_ptr<Canvas> Canvas::create(const Bitmap& bitmap)
{
    return std::shared_ptr<Canvas>(new Canvas(bitmap));
}

std::shared_ptr<Canvas> Canvas::create(float x, float y, float width, float height)
{
    constexpr int kMaxSize = 1 << 24;
    if(width <= 0 || height <= 0 || width > kMaxSize || height > kMaxSize)
        return std::shared_ptr<Canvas>(new Canvas(0, 0, 1, 1));
    auto l = static_cast<int>(std::floor(x));
    auto t = static_cast<int>(std::floor(y));
    auto r = static_cast<int>(std::ceil(x + width));
    auto b = static_cast<int>(std::ceil(y + height));
    return std::shared_ptr<Canvas>(new Canvas(l, t, r - l, b - t));
}

std::shared_ptr<Canvas> Canvas::create(const Rect& extents)
{
    return std::shared_ptr<Canvas>(new Canvas(extents.x, extents.y, extents.w, extents.h));
}

void Canvas::setColor(const Color& color)
{
    setColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

void Canvas::setColor(float r, float g, float b, float a)
{
    plutovg_canvas_set_rgba(m_canvas, r, g, b, a);
}

void Canvas::setLinearGradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const Transform& transform)
{
    auto paint = plutovg_paint_create_linear_gradient(x1, y1, x2, y2, plutovg_spread_method_t(spread), stops.data(), stops.size(), &transform.matrix());
    plutovg_canvas_set_paint(m_canvas, paint);
    plutovg_paint_destroy(paint);
}

void Canvas::setRadialGradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const Transform& transform)
{
    auto paint = plutovg_paint_create_radial_gradient(cx, cy, r, fx, fy, 0.f, plutovg_spread_method_t(spread), stops.data(), stops.size(), &transform.matrix());
    plutovg_canvas_set_paint(m_canvas, paint);
    plutovg_paint_destroy(paint);
}

void Canvas::setTexture(const Canvas& source, TextureType type, float opacity, const Transform& transform)
{
    auto paint = plutovg_paint_create_texture(source.surface(), plutovg_texture_type_t(type), opacity, &transform.matrix());
    plutovg_canvas_set_paint(m_canvas, paint);
    plutovg_paint_destroy(paint);
}

void Canvas::fillPath(const Path& path, FillRule fillRule, const Transform& transform)
{
    plutovg_canvas_reset_matrix(m_canvas);
    plutovg_canvas_translate(m_canvas, -m_x, -m_y);
    plutovg_canvas_transform(m_canvas, &transform.matrix());
    plutovg_canvas_set_fill_rule(m_canvas, plutovg_fill_rule_t(fillRule));
    plutovg_canvas_set_operator(m_canvas, PLUTOVG_OPERATOR_SRC_OVER);
    plutovg_canvas_fill_path(m_canvas, path.data());
}

void Canvas::strokePath(const Path& path, const StrokeData& strokeData, const Transform& transform)
{
    plutovg_canvas_reset_matrix(m_canvas);
    plutovg_canvas_translate(m_canvas, -m_x, -m_y);
    plutovg_canvas_transform(m_canvas, &transform.matrix());
    plutovg_canvas_set_line_width(m_canvas, strokeData.lineWidth());
    plutovg_canvas_set_miter_limit(m_canvas, strokeData.miterLimit());
    plutovg_canvas_set_line_cap(m_canvas, plutovg_line_cap_t(strokeData.lineCap()));
    plutovg_canvas_set_line_join(m_canvas, plutovg_line_join_t(strokeData.lineJoin()));
    plutovg_canvas_set_dash_offset(m_canvas, strokeData.dashOffset());
    plutovg_canvas_set_dash_array(m_canvas, strokeData.dashArray().data(), strokeData.dashArray().size());
    plutovg_canvas_set_operator(m_canvas, PLUTOVG_OPERATOR_SRC_OVER);
    plutovg_canvas_stroke_path(m_canvas, path.data());
}

void Canvas::clipPath(const Path& path, FillRule clipRule, const Transform& transform)
{
    plutovg_canvas_reset_matrix(m_canvas);
    plutovg_canvas_translate(m_canvas, -m_x, -m_y);
    plutovg_canvas_transform(m_canvas, &transform.matrix());
    plutovg_canvas_set_fill_rule(m_canvas, plutovg_fill_rule_t(clipRule));
    plutovg_canvas_clip_path(m_canvas, path.data());
}

void Canvas::clipRect(const Rect& rect, FillRule clipRule, const Transform& transform)
{
    plutovg_canvas_reset_matrix(m_canvas);
    plutovg_canvas_translate(m_canvas, -m_x, -m_y);
    plutovg_canvas_transform(m_canvas, &transform.matrix());
    plutovg_canvas_set_fill_rule(m_canvas, plutovg_fill_rule_t(clipRule));
    plutovg_canvas_clip_rect(m_canvas, rect.x, rect.y, rect.w, rect.h);
}

void Canvas::blendCanvas(const Canvas& canvas, BlendMode blendMode, float opacity)
{
    auto transform = Transform::translated(canvas.x(), canvas.y());
    auto paint = plutovg_paint_create_texture(canvas.surface(), PLUTOVG_TEXTURE_TYPE_PLAIN, opacity, nullptr);
    plutovg_canvas_reset_matrix(m_canvas);
    plutovg_canvas_translate(m_canvas, -m_x, -m_y);
    plutovg_canvas_transform(m_canvas, &transform.matrix());
    plutovg_canvas_set_operator(m_canvas, plutovg_operator_t(blendMode));
    plutovg_canvas_set_paint(m_canvas, paint);
    plutovg_canvas_paint(m_canvas);
    plutovg_paint_destroy(paint);
}

void Canvas::drawImage(const Bitmap& image, const Rect& dstRect, const Rect& srcRect, const Transform& transform)
{
    if(dstRect.isEmpty() || srcRect.isEmpty()) {
        return;
    }

    auto xScale = dstRect.w / srcRect.w;
    auto yScale = dstRect.h / srcRect.h;
    plutovg_matrix_t matrix = {xScale, 0, 0, yScale, -srcRect.x * xScale, -srcRect.y * yScale};

    auto paint = plutovg_paint_create_texture(image.surface(), PLUTOVG_TEXTURE_TYPE_PLAIN, 1.f, &matrix);
    plutovg_canvas_reset_matrix(m_canvas);
    plutovg_canvas_translate(m_canvas, -m_x, -m_y);
    plutovg_canvas_transform(m_canvas, &transform.matrix());
    plutovg_canvas_translate(m_canvas, dstRect.x, dstRect.y);
    plutovg_canvas_set_fill_rule(m_canvas, PLUTOVG_FILL_RULE_NON_ZERO);
    plutovg_canvas_clip_rect(m_canvas, 0, 0, dstRect.w, dstRect.h);
    plutovg_canvas_set_operator(m_canvas, PLUTOVG_OPERATOR_SRC_OVER);
    plutovg_canvas_set_paint(m_canvas, paint);
    plutovg_canvas_paint(m_canvas);
    plutovg_paint_destroy(paint);
}

void Canvas::save()
{
    plutovg_canvas_save(m_canvas);
}

void Canvas::restore()
{
    plutovg_canvas_restore(m_canvas);
}

int Canvas::width() const
{
    return plutovg_surface_get_width(m_surface);
}

int Canvas::height() const
{
    return plutovg_surface_get_height(m_surface);
}

void Canvas::convertToLuminanceMask()
{
    auto width = plutovg_surface_get_width(m_surface);
    auto height = plutovg_surface_get_height(m_surface);
    auto stride = plutovg_surface_get_stride(m_surface);
    auto data = plutovg_surface_get_data(m_surface);
    for(int y = 0; y < height; y++) {
        auto pixels = reinterpret_cast<uint32_t*>(data + stride * y);
        for(int x = 0; x < width; x++) {
            auto pixel = pixels[x];
            auto r = (pixel >> 16) & 0xFF;
            auto g = (pixel >> 8) & 0xFF;
            auto b = (pixel >> 0) & 0xFF;
            auto l = (2*r + 3*g + b) / 6;
            pixels[x] = l << 24;
        }
    }
}

Canvas::~Canvas()
{
    plutovg_canvas_destroy(m_canvas);
    plutovg_surface_destroy(m_surface);
}

Canvas::Canvas(const Bitmap& bitmap)
    : m_surface(plutovg_surface_reference(bitmap.surface()))
    , m_canvas(plutovg_canvas_create(m_surface))
    , m_x(0), m_y(0)
{
}

Canvas::Canvas(int x, int y, int width, int height)
    : m_surface(plutovg_surface_create(width, height))
    , m_canvas(plutovg_canvas_create(m_surface))
    , m_x(x), m_y(y)
{
}

} // namespace lunasvg