#pragma once

#include <algorithm>
#include <vector>
#include <functional>
#include <fstream>
#include <iostream>
#include "vulkan/vulkan.hpp"

namespace toy2d {

using CreateSurfaceFunc = std::function<vk::SurfaceKHR(vk::Instance)>;

struct Color {
    float r, g, b;
};


template <typename T, typename U>
void RemoveNosupportedElems(std::vector<T>& elems, const std::vector<U>& supportedElems,
                            std::function<bool(const T&, const U&)> eq) {
    int i = 0;
    while (i < elems.size()) {
        if (std::find_if(supportedElems.begin(), supportedElems.end(),
                         [&](const U& elem) {
                           return eq(elems[i], elem);
                         })
            == supportedElems.end()) {
            elems.erase(elems.begin() + i);
        } else {
            i ++;
        }
    }
}

std::string ReadWholeFile(const std::string& filename);

}
