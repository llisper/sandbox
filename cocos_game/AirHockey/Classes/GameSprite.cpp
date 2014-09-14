#include "GameSprite.h"

USING_NS_CC;

GameSprite* GameSprite::gameSpriteWithFile(const char *file_name)
{
    GameSprite *sprite = new GameSprite;
    if (sprite && sprite->initWithFile(file_name)) {
        sprite->autorelease();
        return sprite;
    }
    CC_SAFE_DELETE(sprite);
    return nullptr;
}

void GameSprite::setPosition(const Point &pos)
{
    Sprite::setPosition(pos);
    if (!_nextPosition.equals(pos))
        _nextPosition = pos;
}

float GameSprite::radius()
{
    return getTexture()->getContentSize().width * 0.5f;
}
