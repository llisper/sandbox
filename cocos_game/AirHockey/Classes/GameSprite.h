#ifndef __GAMESPRITE_H__
#define __GAMESPRITE_H__

#include "cocos2d.h"

class GameSprite : public cocos2d::Sprite {
public:
    CC_SYNTHESIZE(cocos2d::Point, _nextPosition, NextPosition);
    CC_SYNTHESIZE(cocos2d::Point, _vector, Vector);
    CC_SYNTHESIZE(cocos2d::Touch*, _touch, Touch);

    GameSprite(void) : _vector(0, 0) {}
    ~GameSprite(void) {}

    static GameSprite* gameSpriteWithFile(const char *file_name);

    virtual void setPosition(const cocos2d::Point &pos);
    float radius(); 
};

#endif // __GAMESPRITE_H__
