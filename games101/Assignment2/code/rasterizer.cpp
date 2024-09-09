// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>



rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}




static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

static bool insideTriangle(int x, int y, const Vector3f* _v)
{   
    // TODO : Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
    auto [c1, c2, c3] = computeBarycentric2D(x, y,_v); // Call the compute function

    float sum = c1 + c2 + c3;
    const float epsilon = 1e-6;

    if(sum - 1 <= epsilon) return true;

    return false;
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto& buf = pos_buf[pos_buffer.pos_id];
    auto& ind = ind_buf[ind_buffer.ind_id];
    auto& col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        //Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        rasterize_triangle(t);
    }
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    auto v = t.toVector4();
    auto a = v[0];
    auto b = v[1];
    auto c = v[2];

    // 计算当前三角形的边界框
    float min_x = std::min({a.x(), b.x(), c.x()});
    float max_x = std::max({a.x(), b.x(), c.x()});
    float min_y = std::min({a.y(), b.y(), c.y()});
    float max_y = std::max({a.y(), b.y(), c.y()});

    // 2x2 超级采样点的偏移
    std::vector<Eigen::Vector2f> sample_offsets = {
        {0.25, 0.25}, {0.75, 0.25}, {0.25, 0.75}, {0.75, 0.75}
    };

    // 遍历边界框内的像素
    for (int x = static_cast<int>(min_x); x <= static_cast<int>(max_x); ++x) {
        for (int y = static_cast<int>(min_y); y <= static_cast<int>(max_y); ++y) {
            float color_r = 0, color_g = 0, color_b = 0;
            float min_depth = std::numeric_limits<float>::infinity(); // 初始化为最大深度
            int count = 0; // 用于统计在三角形内的样本数目
            float totalzValue = 0;

            // 对每个像素进行 2x2 采样
            for (const auto& offset : sample_offsets) {
                float sample_x = x + offset.x();
                float sample_y = y + offset.y();
                
                // 计算采样点的重心坐标
                auto [alpha, beta, gamma] = computeBarycentric2D(sample_x, sample_y, t.v);
                
                // 如果采样点在三角形内
                if (alpha >= 0 && beta >= 0 && gamma >= 0) {
                    float w_reciprocal = 1.0f / (alpha / a.w() + beta / b.w() + gamma / c.w());
                    float z_interpolated = (alpha * a.z() / a.w() + beta * b.z() / b.w() + gamma * c.z() / c.w()) * w_reciprocal;

                    // int index = get_index(sample_x, sample_y); // 计算当前样本点的索引
                    // if (depth_buf[index] > z_interpolated) {
                    depth_buf[index] = z_interpolated;
                    totalzValue += z_interpolated;
                    Eigen::Vector3f color = t.getColor();
                    color_r += color[0];
                    color_g += color[1];
                    color_b += color[2];
                    count++;
                    
                    // 更新最小深度
                    // min_depth = std::min(min_depth, z_interpolated);
                    // }
                }
            }

            // 如果至少一个样本在三角形内，则根据深度测试更新像素颜色
            if (count > 0 && (totalzValue / count) < depth_buf[get_index(x, y)]) {
                depth_buf[get_index(x, y)] = totalzValue / count;
                set_pixel(Vector3f(x, y, 1), Eigen::Vector3f(color_r / count, color_g / count, color_b / count));
            }
        }
    }
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height-1-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

// clang-format on