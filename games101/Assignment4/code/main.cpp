#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

std::vector<cv::Point2f> control_points;

void mouse_handler(int event, int x, int y, int flags, void *userdata) 
{
    if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4) 
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", "
        << y << ")" << '\n';
        control_points.emplace_back(x, y);
    }     
}

void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window) 
{
    auto &p_0 = points[0];
    auto &p_1 = points[1];
    auto &p_2 = points[2];
    auto &p_3 = points[3];

    for (double t = 0.0; t <= 1.0; t += 0.001) 
    {
        auto point = std::pow(1 - t, 3) * p_0 + 3 * t * std::pow(1 - t, 2) * p_1 +
                 3 * std::pow(t, 2) * (1 - t) * p_2 + std::pow(t, 3) * p_3;
        if(std::abs(t - 0.50) < 0.001){
            std::cout << "The coordinates are, x:" << point.x << ", y: " << point.y << '\n';
        }

        window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
        window.at<cv::Vec3b>(point.y, point.x)[0] = 255;
    }
}

long long binomialCoefficient(int n, int i) {
    if (i > n) return 0;
    if (i == 0 || i == n) return 1;  // Base cases: C(n, 0) = 1 and C(n, n) = 1

    // We can optimize by using the smaller of i and n-i, as C(n, i) == C(n, n-i)
    if (i > n - i) {
        i = n - i;
    }

    long long result = 1;
    for (int k = 0; k < i; ++k) {
        result *= (n - k);
        result /= (k + 1);
    }

    return result;
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t) 
{
    // TODO: Implement de Casteljau's algorithm
    
    std::vector<cv::Point2f> points = control_points;

    // De Casteljau's algorithm: repeatedly interpolate between points
    while (points.size() > 1) {
        std::vector<cv::Point2f> new_points;
        
        // Linearly interpolate between consecutive points
        for (size_t i = 0; i < points.size() - 1; ++i) {
            float x = (1 - t) * points[i].x + t * points[i + 1].x;
            float y = (1 - t) * points[i].y + t * points[i + 1].y;
            new_points.push_back(cv::Point2f(x, y));
        }

        points = new_points;  // Update points with the new intermediate points
    }

    // The last remaining point is the point on the Bezier curve at parameter t
    return points[0];

}

// void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window) 
// {
//     // TODO: Iterate through all t = 0 to t = 1 with small steps, and call de Casteljau's 
//     // recursive Bezier algorithm.
//     float t = 0;
//     const float step = 0.001f; // The smaller the step, the smoother the curve

//     // Iterate over t from 0 to 1 with small steps
//     while (t <= 1) {
//         // Get the point on the Bezier curve at parameter t
//         cv::Point2f point = recursive_bezier(control_points, t);
        
//         // Draw the point on the window (image)
//         // cv::circle(window, point, 1, {0,255, 0}, -1); // Red color, filled circle
//         window.at<cv::Vec3b>(point.y, point.x)[1] = 255;
        
        
//         t += step;
//     }

// }
#include <cmath>
// #include <opencv2/opencv.hpp>

// Gaussian-like function to calculate the weight based on distance
float gaussian_weight(float distance, float sigma = 1.0f) {
    return exp(-(distance * distance) / (2 * sigma * sigma));
}

void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window) {
    float t = 0;
    const float step = 0.001f; // The smaller the step, the smoother the curve
    const float radius = 2.0f; // The radius around the point to apply the antialiasing
    const float sigma = 1.5f;  // Sigma for Gaussian decay (adjust for smoothness)

    // Iterate over t from 0 to 1 with small steps
    while (t <= 1) {
        // Get the point on the Bezier curve at parameter t
        cv::Point2f point = recursive_bezier(control_points, t);

        // Round to nearest integer coordinates for pixel index
        int x = static_cast<int>(round(point.x));
        int y = static_cast<int>(round(point.y));

        // Only process pixels within the image boundaries
        if (x >= 0 && x < window.cols && y >= 0 && y < window.rows) {
            // For each pixel around the current curve point, adjust the color
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    // Ensure we're still inside the image
                    if (nx >= 0 && nx < window.cols && ny >= 0 && ny < window.rows) {
                        // Compute the Euclidean distance from the current point to the pixel center
                        float distance = sqrt(dx * dx + dy * dy);

                        // If the distance is within the radius, apply anti-aliasing
                        if (distance <= radius) {
                            // Calculate weight using a Gaussian-like function
                            float weight = gaussian_weight(distance, sigma);

                            // Get the current pixel color (BGR format)
                            cv::Vec3b &pixel = window.at<cv::Vec3b>(ny, nx);

                            // Add weighted color to the pixel (we use green as the color for the curve)
                            pixel[1] = cv::saturate_cast<uchar>(pixel[1] + weight * 255); // Green channel
                        }
                    }
                }
            }
        }

        // Increment t for the next curve point
        t += step;
    }
}

int main() 
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
    cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
    cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);

    int key = -1;
    while (key != 27) 
    {
        for (auto &point : control_points) 
        {
            cv::circle(window, point, 3, {255, 255, 255}, 3);
        }

        if (control_points.size() == 4) 
        {
            naive_bezier(control_points, window);
            bezier(control_points, window);

            cv::imshow("Bezier Curve", window);
            cv::imwrite("my_bezier_curve.png", window);
            key = cv::waitKey(0);

            return 0;
        }

        cv::imshow("Bezier Curve", window);
        key = cv::waitKey(20);
    }

return 0;
}
