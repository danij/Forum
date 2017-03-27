#include "Application.h"
#include "ContextProviders.h"
#include "DefaultIOServiceProvider.h"
#include "StringHelpers.h"

#include <unicode/uclean.h>

using namespace Forum;
using namespace Forum::Context;
using namespace Forum::Network;

Application::Application(int argc, const char* argv[])
{
    setApplicationEventCollection(std::make_unique<ApplicationEventCollection>());
    setIOServiceProvider(std::make_unique<DefaultIOServiceProvider>());
}

int Application::run()
{
    auto& events = getApplicationEvents();

    events.onApplicationStart();

    getIOServiceProvider().waitForStop();
    
    events.beforeApplicationStop();
    
    cleanup();

    return 0;
}

void Application::cleanup()
{
    Helpers::cleanupStringHelpers();

    //clean up resources cached by ICU so that they don't show up as memory leaks
    u_cleanup();
}