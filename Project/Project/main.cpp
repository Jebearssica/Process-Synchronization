#include<iostream>
#include<Windows.h>
#include<thread>
#include<mutex>
#include<random>
#include<condition_variable>
#define MAX_NUMBER_IN_SALE 200                    //大厅最大等待人数
#define MAX_WINDOW_HUM 2                          //人工窗口数目
#define MAX_WINDOW_AUTO 6                         //自动窗口数目
#define MAX_NUMBER_SERVE 1000                     //本次最大服务人数
#define TIME_MAN 250                              //人工窗口处理时间
#define TIME_AUTO 100                             //自动窗口处理时间
#define TIME_WND_SLEEP 500                        //窗口等待时间
#define TIME_CUSTOMER_CREATE 100                  //顾客产生基础时间
enum Human_Status { Return, Sign, Buy, Collect }; //顾客的四种状态

//互斥量初始化
std::mutex In, Out;                               //出入口
std::mutex Wnd[MAX_WINDOW_AUTO + MAX_WINDOW_HUM]; //八个窗口

//创建一个休眠类
std::condition_variable CV;
//判断最少的队列
int Judge(const int a, const int b, const int *Queue) {
  int Result = a;
  for (size_t i = a + 1; i < b; i++) {
    if (Queue[Result] > Queue[i])
      Result = i;
  }
  return Result;
}
//顾客进程
int Customer_In(int &Number) {                    //顾客线程进入函数
  std::uniform_int_distribution<> u_Number(0, 1000);
  std::uniform_int_distribution<> u_Status(Return, Collect);
  std::random_device e;                           //以系统熵作为随机种子的引擎
  std::cout << "*******************" << std::endl;
  std::cout << "第" << Number << "位顾客进入" << std::endl;
  std::cout << "进入时间(模拟量)：" << u_Number(e) << std::endl;
  std::cout << "状态：";
  switch (u_Status(e)) {
  case Return:
    std::cout << "退票" << std::endl;
    return 0;
  case Sign:
    std::cout << "签票" << std::endl;
    return 0;
  case Buy:
    std::cout << "购票" << std::endl;
    return 1;
  case Collect:
    std::cout << "取票" << std::endl;
    return 1;
  default:
    std::cout << "随机状态生成错误" << std::endl;
    return -1;
  }
}

void Customer(int &Number_People, int Number_Served, int *Number_Queue) {
  std::lock_guard<std::mutex> lockGuard(In);            //自动锁 锁入口
  std::cout << "线程生成 ID：" << std::this_thread::get_id() << std::endl;
  if (Number_People < MAX_NUMBER_IN_SALE) {
    Number_People++;
    int Status = Customer_In(Number_Served);        //顾客进入
    //In.unlock;
                                                    //判断选择队列
    int Result;
    if (Status == -1) {
      return;
    }
    else if (Status == 0) {
      Result = Judge(0, 1, Number_Queue);
      std::cout << "选择队列(人工)：" << Result << std::endl;
    }
    else {
      Result = Judge(0, 7, Number_Queue);
      std::cout << "选择队列(所有)：" << Result << std::endl;
    }
    Wnd[Result].lock();
    Number_Queue[Result]++;         //进入队列
    Wnd[Result].unlock();
  }
  else {
    //In.unlock;
    std::cout << "进入等待" << std::endl;
  }
}

int Window(const int Number_Wnd, int &Number) {
  std::uniform_int_distribution<> u_Time(0, 250);
  std::random_device e;
  int Offset = 0;
  while (true) {
    if (Number != 0) {
      Offset = u_Time(e);                             //不同人处理时间不同
      if (Number_Wnd == 0 || Number_Wnd == 1) {       //人工窗口
        Sleep(TIME_MAN + Offset);                     //模拟处理时间
      }
      else {                                          //自动窗口
        Sleep(TIME_AUTO + Offset);                    //模拟处理时间
      }
      std::lock_guard<std::mutex> lockGuard(Wnd[Number_Wnd]); //上锁 改变队列人数
      Number--;
    }
    else {
      //窗口等待
      Sleep(TIME_WND_SLEEP);
    }
  }
  return 0;
}



int main() {
  //初始化

  int Number_People = 0;  //所占人数
  int Number_Served = 0;  //已经服务的人数
  int Number_Queue[MAX_WINDOW_AUTO + MAX_WINDOW_HUM] = { 0 }; //各队列人数

  //顾客产生时间初始化 偏移量（0~0.5s）
  int Time_Offset = 0;
  std::uniform_int_distribution<> u_Time(0, 50);
  std::random_device e;

  //线程初始化
  std::thread Customers[MAX_NUMBER_SERVE]; //顾客线程
  std::thread Wnd_Hum[MAX_WINDOW_HUM];    //人工窗口线程（处理速度慢）
  std::thread Wnd_Auto[MAX_WINDOW_AUTO];  //自动窗口线程（处理速度快）

  //产生进程

  //人工窗口线程
  for (size_t i = 0; i < MAX_WINDOW_HUM; i++) {
    Wnd_Hum[i] = std::thread(Window, i, std::ref(Number_Queue[i]));
  }
  //自动窗口线程
  for (size_t i = 0; i < MAX_WINDOW_AUTO; i++) {
    Wnd_Auto[i] = std::thread(Window, i, std::ref(Number_Queue[MAX_WINDOW_HUM + i]));
  }
  //顾客线程
  while (Number_Served < MAX_NUMBER_SERVE) {
    Customers[Number_Served] = std::thread(Customer, std::ref(Number_People), Number_Served, Number_Queue);
    Number_Served++;
  }
  //顾客线程等待进入
  while (Number_Served < MAX_NUMBER_SERVE) {
    if (Number_People <= MAX_NUMBER_IN_SALE) {
      Customers[Number_Served].join();
      Sleep(TIME_CUSTOMER_CREATE + u_Time(e));   //每(100+Offset)ms产生一个顾客
      Number_Served++;
    }
  }
  std::cout << "已经达到最大服务人数 本次服务截止" << std::endl;
  system("pause");
  return 0;
}