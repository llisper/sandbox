#include "AppDelegate.h"
#include "GameLayer.h"
#include "SimpleAudioEngine.h"

USING_NS_CC;
using namespace CocosDenshion;

AppDelegate::AppDelegate() {

}

AppDelegate::~AppDelegate() 
{
}

bool AppDelegate::applicationDidFinishLaunching() {
    // initialize director
    auto director = Director::getInstance();
    auto glview = director->getOpenGLView();
    if(!glview) {
        glview = GLView::create("My Game");
        director->setOpenGLView(glview);
    }

    // add hd/sd resource directory
    Size scr_size = glview->getFrameSize();
    glview->setDesignResolutionSize(768, 1024, ResolutionPolicy::EXACT_FIT);
    if (scr_size.width > 768) {
        FileUtils::getInstance()->addSearchPath("hd");
        director->setContentScaleFactor(2);
    } else {
        FileUtils::getInstance()->addSearchPath("sd");
        director->setContentScaleFactor(1);
    }

    // add sound files
    SimpleAudioEngine::sharedEngine()->preloadEffect(
        FileUtils::getInstance()->fullPathForFilename("hit.wav").c_str()
        );
    SimpleAudioEngine::sharedEngine()->preloadEffect(
        FileUtils::getInstance()->fullPathForFilename("score.wav").c_str()
        );

    const std::vector<std::string> &search_paths = FileUtils::getInstance()->getSearchPaths();
    for (const auto &path : search_paths) {
        CCLOG("%s", path.c_str());
    }

    // turn on display FPS
    director->setDisplayStats(true);

    // set FPS. the default value is 1.0/60 if you don't call this
    director->setAnimationInterval(1.0 / 60);

    // create a scene. it's an autorelease object
    auto scene = GameLayer::createScene();

    // run
    director->runWithScene(scene);

    return true;
}

// This function will be called when the app is inactive. When comes a phone call,it's be invoked too
void AppDelegate::applicationDidEnterBackground() {
    Director::getInstance()->stopAnimation();

    // if you use SimpleAudioEngine, it must be pause
    // SimpleAudioEngine::getInstance()->pauseBackgroundMusic();
}

// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground() {
    Director::getInstance()->startAnimation();

    // if you use SimpleAudioEngine, it must resume here
    // SimpleAudioEngine::getInstance()->resumeBackgroundMusic();
}
