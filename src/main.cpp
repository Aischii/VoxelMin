#include "core/Application.hpp"
#include "core/Log.hpp"

#include <cstdio>

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0); // unbuffered logs, handy for dev

    vox::Application app;
    if (!app.init()) {
        vox::log::error("Initialization failed, exiting");
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}
