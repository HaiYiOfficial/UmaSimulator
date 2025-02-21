#include <iostream>
#include <iomanip> 
#include <sstream>
#include <fstream>
#include <cassert>
#include <thread>  // for std::this_thread::sleep_for
#include <chrono>  // for std::chrono::seconds
#include "../Game/Game.h"
#include "../Search/Search.h"
using namespace std;

const bool NoColor = false;//有些电脑无法正常显示颜色



void main_test6()
{
  const double radicalFactor = 5;//激进度
  const int threadNum = 16; //线程数
  const int searchN = 12288; //每个选项的蒙特卡洛模拟的局数

  //激进度为k，模拟n局时，标准差约为sqrt(1+k^2/(2k+1))*1200/(sqrt(n))
  //标准差大于30时会严重影响判断准确度

  Search search;
  vector<Evaluator> evaluators;
  for (int i = 0; i < threadNum; i++)
    evaluators.push_back(Evaluator(NULL, 128));
  int lastTurn = -1;
  int scoreFirstTurn = 0;   // 第一回合分数
  int scoreLastTurn = 0;   // 上一回合分数
  while (true)
  {
    ifstream fs("./packets/currentGS.json");
    if (!fs.good())
    {
      cout << "等待游戏开始" << endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));//延迟几秒，避免刷屏
      continue;
    }
    ostringstream tmp;
    tmp << fs.rdbuf();
    fs.close();

    string jsonStr = tmp.str();
    Game game;
    bool suc = game.loadGameFromJson(jsonStr);
    if (!suc)
    {
      cout << "出现错误" << endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));//延迟几秒，避免刷屏
      continue;
    }
    if (game.turn == lastTurn)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(300));//检查是否有更新
      continue;
    }
    lastTurn = game.turn;
    if (game.venusIsWisdomActive)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      continue;
    }

    if (game.turn == 0)//第一回合，或者重启ai的第一回合
    {
      scoreFirstTurn = 0;
      scoreLastTurn = 0;
    }
    game.print();
    cout << endl;
    cout << "计算中..." << endl;
    cout << endl;

    auto printPolicy = [](float p)
    {
      cout << fixed << setprecision(1);
      if (!NoColor)
      {
        if (p >= 0.3)cout << "\033[33m";
        //else if (p >= 0.1)cout << "\033[32m";
        else cout << "\033[36m";
      }
      cout << p * 100 << "% ";
      if (!NoColor)cout << "\033[0m";
    };

    search.runSearch(game, evaluators.data(), searchN, TOTAL_TURN, 0, threadNum, radicalFactor);
    cout << "计算完毕" << endl;
    cout << ">>" << endl;
    {
      auto policy = search.extractPolicyFromSearchResults(1);
      if (game.venusAvailableWisdom != 0)
      {
        cout << "使用女神率：";
        printPolicy(policy.useVenusPolicy);
        cout << endl;
      }
      if (!game.isRacing)
      {
        if (game.venusAvailableWisdom != 0)
        {
          cout << "在" << (policy.useVenusPolicy > 0.5 ? "" : " 不 ") << "使用女神的前提下：";
        }

        cout << "速耐力根智：";
        for (int i = 0; i < 5; i++)
          printPolicy(policy.trainingPolicy[i]);
        cout << endl;

        cout << "休息，外出，比赛：";
        for (int i = 0; i < 3; i++)
          printPolicy(policy.trainingPolicy[5 + i]);
        cout << endl;

        cout << "女神三选一事件：红，蓝，黄：";
        for (int i = 0; i < 3; i++)
          printPolicy(policy.threeChoicesEventPolicy[i]);
        cout << endl;

        cout << "五个女神外出以及普通外出：";
        for (int i = 0; i < 6; i++)
          printPolicy(policy.outgoingPolicy[i] * policy.trainingPolicy[6]);
        cout << endl;
      }
    }

    float maxScore = -1e6;
    for (int i = 0; i < 2; i++)
    {
      for (int j = 0; j < 8; j++)
      {
        float s = search.allChoicesValue[i][j].avgScoreMinusTarget;
        if (s > maxScore)maxScore = s;
      }
    }
    if(game.turn==0)
    {
      scoreFirstTurn = maxScore;
    }
    else
    {
      cout<<"以下两个指标没有考虑技能，买技能后下降正常" << endl;
      cout << "此局运气：" << maxScore - scoreFirstTurn  << endl;
      cout << "此回合运气：" << maxScore - scoreLastTurn  << endl; 
      double raceLoss = maxScore - std::max(search.allChoicesValue[0][7].avgScoreMinusTarget, search.allChoicesValue[1][7].avgScoreMinusTarget);
      if (raceLoss < 5e5)//raceLoss大约1e6如果不能比赛
        cout << "比赛亏损（用于选择比赛回合，以完成粉丝数目标）：" << raceLoss << endl;
      cout << "<<" << endl;
    }
    scoreLastTurn = maxScore;
    /*
    cout << "各选项的期望分数：" << endl;
    for (int i = 0; i < 2; i++)
    {
      for (int j = 0; j < 8 + 4 + 6; j++)
      {
        cout
          //<< fixed << setprecision(1) << search.allChoicesValue[i][j].winrate * 100 << "%:" 
          << fixed << setprecision(0) << search.allChoicesValue[i][j].avgScoreMinusTarget << " ";
        if (j == 4 || j == 7 || j == 11)cout << endl;
      }
      cout << endl;
      cout << endl;
    }
    */

  }

}