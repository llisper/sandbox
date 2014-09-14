#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"

class GameSprite;
class GameLayer : public cocos2d::Layer
{
public:
    ~GameLayer();

    // there's no 'id' in cpp, so we recommend returning the class instance pointer
    static cocos2d::Scene* createScene();

    // Here's a difference. Method 'init' in cocos2d-x returns bool, instead of returning 'id' in cocos2d-iphone
    virtual bool init();  
    
    // a selector callback
    void menuCloseCallback(cocos2d::Ref* pSender);
    
    // implement the "static create()" method manually
    CREATE_FUNC(GameLayer);

    virtual void onTouchesBegan(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event *unused_event);
    virtual void onTouchesMoved(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event *unused_event);
    virtual void onTouchesEnded(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event *unused_event);

    void update(float dt);
    void playerScore(int player);
    int  playerID(GameSprite *player) const { return _player1 == player ? 1 : 2; }

private:
    GameSprite *_player1;
    GameSprite *_player2;
    GameSprite *_ball;

    cocos2d::Array *_players;
    cocos2d::LabelTTF *_player1ScoreLabel;
    cocos2d::LabelTTF *_player2ScoreLabel;

    cocos2d::Size _screenSize;
    int _player1Score;
    int _player2Score;
};

#endif // __HELLOWORLD_SCENE_H__
