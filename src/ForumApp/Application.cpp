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

    httpListener_ = std::make_unique<HttpListener>(getIOServiceProvider().getIOService());
}

int Application::run()
{
    auto& events = getApplicationEvents();

    events.onApplicationStart();
    
    httpListener_->startListening();

    getIOServiceProvider().start();
    getIOServiceProvider().waitForStop();
    
    httpListener_->stopListening();

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
