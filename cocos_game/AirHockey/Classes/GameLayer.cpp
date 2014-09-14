#include "GameLayer.h"
#include "GameSprite.h"
#include "SimpleAudioEngine.h"

USING_NS_CC;
using namespace CocosDenshion; 

#define GOLD_WIDTH 400

Scene* GameLayer::createScene()
{
  // 'scene' is an autorelease object
  auto scene = Scene::create();

  // 'layer' is an autorelease object
  auto layer = GameLayer::create();

  // add layer as a child to scene
  scene->addChild(layer);

  // return the scene
  return scene;
}

// on "init" you need to initialize your instance
bool GameLayer::init()
{
  //////////////////////////////
  // 1. super init first
  if ( !Layer::init() )
  {
    return false;
  }

  Director *director = Director::getInstance();
  _screenSize = director->getWinSize();
  _player1Score = _player2Score = 0;

  // place background
  // place players
  // place ball
  // place score label

  Sprite *court = Sprite::create("court.png");
  court->setPosition(_screenSize.width / 2, _screenSize.height / 2);
  this->addChild(court);

  _player1 = GameSprite::gameSpriteWithFile("mallet.png");
  _player1->setPosition(Point(_screenSize.width / 2, _screenSize.height - _player1->radius() * 2));
  this->addChild(_player1);

  _player2 = GameSprite::gameSpriteWithFile("mallet.png");
  _player2->setPosition(Point(_screenSize.width / 2, _player2->radius() * 2));
  this->addChild(_player2);

  _players = Array::create(_player1, _player2, nullptr);
  _players->retain();

  _ball = GameSprite::gameSpriteWithFile("puck.png");
  _ball->setPosition(Point(_screenSize.width / 2, _screenSize.height / 2 - _ball->radius() * 2));
  this->addChild(_ball);

  _player1ScoreLabel = LabelTTF::create("0", "Arial", 60);
  _player1ScoreLabel->setPosition(_screenSize.width - 60, _screenSize.height / 2 - 80);
  _player1ScoreLabel->setRotation(90);
  this->addChild(_player1ScoreLabel);

  _player2ScoreLabel = LabelTTF::create("0", "Arial", 60);
  _player2ScoreLabel->setPosition(_screenSize.width - 60, _screenSize.height / 2 + 80);
  _player2ScoreLabel->setRotation(90);
  this->addChild(_player2ScoreLabel);

  this->setTouchEnabled(true);
  this->schedule(schedule_selector(GameLayer::update));

  return true;
}


void GameLayer::menuCloseCallback(Ref* pSender)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WP8) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
  MessageBox("You pressed the close button. Windows Store Apps do not implement a close button.","Alert");
  return;
#endif

  Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
  exit(0);
#endif
}

GameLayer::~GameLayer()
{
  CC_SAFE_RELEASE(_players);
}

void GameLayer::update( float dt )
{
  Point ball_next_position = _ball->getNextPosition() + _ball->getVector() * 0.98f;
  Vec2 ball_vector = _ball->getVector();
  float sq_radii = std::pow(_ball->radius() + _player1->radius(), 2);
  for (ssize_t i = 0; i < _players->count(); ++i) {
    GameSprite *player = (GameSprite*)_players->getObjectAtIndex(i);
    float distance1 = player->getPosition().distanceSquared(ball_next_position);
    float distance2 = player->getNextPosition().distanceSquared(_ball->getPosition());

    if (distance1 <= sq_radii || distance2 <= sq_radii) {
      float mag_ball = _ball->getVector().lengthSquared();
      float mag_player = player->getVector().lengthSquared();
      float force = std::sqrt(mag_player + mag_ball);
      Vec2 vector_player_to_ball = ball_next_position - player->getPosition();
      float angle = std::atan2(vector_player_to_ball.y, vector_player_to_ball.x);
      ball_vector = Vec2(std::cos(angle) * force, std::sin(angle) * force);
      ball_next_position = Point(
        player->getNextPosition().x + (player->radius() + _ball->radius() + force) * std::cos(angle),
        player->getNextPosition().y + (player->radius() + _ball->radius() + force) * std::sin(angle));

      SimpleAudioEngine::getInstance()->playEffect("hit.wav");
    }
  }

  if (ball_next_position.x < _ball->radius()) {
    ball_next_position.x = _ball->radius();
    ball_vector.x *= -0.8f;
    SimpleAudioEngine::getInstance()->playEffect("hit.wav");
  }

  if (ball_next_position.x > _screenSize.width - _ball->radius()) {
    ball_next_position.x = _screenSize.width - _ball->radius();
    ball_vector.x *= -0.8f;
    SimpleAudioEngine::getInstance()->playEffect("hit.wav");
  }

  if (ball_next_position.y > _screenSize.height - _ball->radius()) {
    if (ball_next_position.x < _screenSize.width / 2 - GOLD_WIDTH / 2 || ball_next_position.x > _screenSize.width / 2 + GOLD_WIDTH / 2) {
      ball_next_position.y = _screenSize.height - _ball->radius();
      ball_vector.y *= -0.8f;
      SimpleAudioEngine::getInstance()->playEffect("hit.wav");
    }
  }

  if (ball_next_position.y < _ball->radius()) {
    if (ball_next_position.x < _screenSize.width / 2 - GOLD_WIDTH / 2 || ball_next_position.x > _screenSize.width / 2 + GOLD_WIDTH / 2) {
      ball_next_position.y = _ball->radius();
      ball_vector.y *= -0.8f;
      SimpleAudioEngine::getInstance()->playEffect("hit.wav");
    }
  }

  _ball->setVector(ball_vector);
  _ball->setNextPosition(ball_next_position);

  if (ball_next_position.y < - _ball->radius() * 2) {
    playerScore(2);
  }

  if (ball_next_position.y > _screenSize.height + _ball->radius() * 2) {
    playerScore(1);
  }

  _player1->setPosition(_player1->getNextPosition());
  _player2->setPosition(_player2->getNextPosition());
  _ball->setPosition(_ball->getNextPosition());
}

void GameLayer::playerScore(int player) {
  SimpleAudioEngine::getInstance()->playEffect("score.wav");
  _ball->setVector(Vec2());
    
  if (player == 1) {
    ++_player1Score; 
    _player1ScoreLabel->setString(StringUtils::toString(_player1Score));
    _ball->setPosition(Point(_screenSize.width / 2, _screenSize.height / 2 + _ball->radius() * 2));
  } else {
    ++_player2Score; 
    _player2ScoreLabel->setString(StringUtils::toString(_player2Score));
    _ball->setPosition(Point(_screenSize.width / 2, _screenSize.height / 2 - _ball->radius() * 2));
  }

  _player1->setPosition(Point(_screenSize.width / 2, _player1->radius() * 2));
  _player1->setTouch(nullptr);
  _player1->setPosition(Point(_screenSize.width / 2, _screenSize.height - _player1->radius() * 2));
  _player1->setTouch(nullptr);
}

void GameLayer::onTouchesBegan( const std::vector<cocos2d::Touch*>& touches, cocos2d::Event *unused_event )
{
  for (Touch *touch : touches) {
    Point tap = touch->getLocation();
    for (ssize_t i = 0; i < _players->count(); ++i) {
      GameSprite *player = (GameSprite*)_players->getObjectAtIndex(i);
      if (player->boundingBox().containsPoint(tap)) {
        player->setTouch(touch);
        CCLOG("player[%d] onTouchesBegan: %p[%f,%f]", playerID(player), touch, tap.x, tap.y);
      }
    } 
  }
}

void GameLayer::onTouchesMoved( const std::vector<cocos2d::Touch*>& touches, cocos2d::Event *unused_event )
{
  for (Touch *touch : touches) {
    Point tap = touch->getLocation();
    CCLOG("onTouchesMoved: %p[%f,%f]", touch, tap.x, tap.y);
    for (ssize_t i = 0; i < _players->count(); ++i) {
      GameSprite *player = (GameSprite*)_players->getObjectAtIndex(i);
      if (player->getTouch() && player->getTouch() == touch) {
        Point next_position = tap;

        // correct position x
        if (next_position.x < player->radius())
          next_position.x = player->radius();
        else if (next_position.x > _screenSize.width - player->radius())
            next_position.x = _screenSize.width - player->radius();

        // correct position y
        if (next_position.y < player->radius())
          next_position.y = player->radius();
        else if (next_position.y > _screenSize.height - player->radius())
            next_position.y = _screenSize.height - player->radius();

        // correct position y according to which side the player belongs to
        if (player->getPositionY() < _screenSize.height / 2) {
            if (next_position.y > _screenSize.height / 2 - player->radius())
                next_position.y = _screenSize.height / 2 - player->radius();
        } else {
            if (next_position.y < _screenSize.height / 2 + player->radius())
                next_position.y = _screenSize.height / 2 + player->radius();
        }

        player->setNextPosition(next_position);
        player->setVector(tap - player->getPosition());
      }
    }
  }
}

void GameLayer::onTouchesEnded( const std::vector<cocos2d::Touch*>& touches, cocos2d::Event *unused_event )
{
  for (Touch *touch : touches) {
    for (ssize_t i = 0; i < _players->count(); ++i) {
      GameSprite *player = (GameSprite*)_players->getObjectAtIndex(i);
      if (player->getTouch() && player->getTouch() == touch) {
        player->setTouch(nullptr);
        player->setVector(Point());
      }
    }
  }
}

#undef GOLD_WIDTH
