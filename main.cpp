#include <iostream>
#include <format>
#include <array>
#include <vector>
#include <utility>
#include <algorithm>
#include <random>
#include <thread>
#include <stop_token>
#include <chrono>
#include <cstddef>

int main()
{
    constexpr int rows = 80,
                  columns = 120;

    constexpr double minX = 0,
                     maxX = 10,
                     xIncrement = (maxX - minX) / columns,
                     minY = 0,
                     maxY = 1;

    std::random_device rd;
    std::uniform_real_distribution<double> smallYDist(minY, (minY + maxY) / 2),
                                           coeffDist(0.90, 1.2);
    std::array<std::pair<double, double>, columns> points;

    auto generatePoints = [&]()
    {
        points[0].first = minX;
        points[0].second = smallYDist(rd);

        for (int i = 1; i < columns; ++i)
        {
            points[i].first = points[i - 1].first + xIncrement;
            points[i].second = std::clamp(points[i - 1].second * coeffDist(rd), minY, maxY);
        }
    };

    auto changeColor = [](int red, int green, int blue)
    {
        std::cout << std::format("\033[38;2;{};{};{}m", red, green, blue);
    };

    auto moveCursor = [](int row, int column)
    {
        std::cout << std::format("\033[{};{}H", row + 1, column + 1);
    };

    std::cout << "\033[?25l"; // turn off cursor

    std::jthread thr{[&](std::stop_token st)
    {
        while (!st.stop_requested())
        {
            generatePoints();

            std::vector<std::pair<double, int>> averages;
            averages.reserve(columns);
            for (int i = 0; i < columns; ++i)
                averages.emplace_back(points[i].second, 1);

            std::size_t index = 1;
            while (index < averages.size())
            {
                const auto [currentAverage, currentWeight] = averages[index];
                const auto [previousAverage, previousWeight] = averages[index - 1];

                if (currentAverage < previousAverage)
                {
                    averages[index - 1].first = (currentAverage * currentWeight + previousAverage * previousWeight)
                                                / (currentWeight + previousWeight);
                    averages[index - 1].second = currentWeight + previousWeight;
                    averages.erase(averages.begin() + index);
                    index = 1;
                }
                else
                    ++index;
            }

            std::cout << "\033[2J"; // clear screen

            changeColor(255, 0, 0);
            for (int column = 0; column < columns; ++column)
            {
                const double yValue = points[column].second;
                const int row = rows - 1 - static_cast<int>((yValue - minY) / (maxY - minY) * (rows - 1));
                moveCursor(row, column);
                std::cout << 'o';
            }

            changeColor(0, 255, 255);
            int column = 0;
            for (const auto [average, weight] : averages)
            {
                const int row = rows - 1 - static_cast<int>((average - minY) / (maxY - minY) * (rows - 1));
                for (int i = 0; i < weight; ++i)
                {
                    moveCursor(row, column);
                    std::cout << 'o';
                    ++column;
                }
            }
            std::cout << std::flush;

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }};
    std::cin.get();

    moveCursor(rows, 0);
    std::cout << "\033[?25h"; // restore cursor
}