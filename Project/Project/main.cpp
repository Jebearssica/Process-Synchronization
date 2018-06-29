#include<iostream>
#include<Windows.h>
#include<thread>
#include<mutex>
#include<random>
#include<condition_variable>
#define MAX_NUMBER_IN_SALE 200                    //�������ȴ�����
#define MAX_WINDOW_HUM 2                          //�˹�������Ŀ
#define MAX_WINDOW_AUTO 6                         //�Զ�������Ŀ
#define MAX_NUMBER_SERVE 1000                     //��������������
#define TIME_MAN 250                              //�˹����ڴ���ʱ��
#define TIME_AUTO 100                             //�Զ����ڴ���ʱ��
#define TIME_WND_SLEEP 500                        //���ڵȴ�ʱ��
#define TIME_CUSTOMER_CREATE 100                  //�˿Ͳ�������ʱ��
enum Human_Status { Return, Sign, Buy, Collect }; //�˿͵�����״̬

//��������ʼ��
std::mutex In, Out;                               //�����
std::mutex Wnd[MAX_WINDOW_AUTO + MAX_WINDOW_HUM]; //�˸�����

//����һ��������
std::condition_variable CV;
//�ж����ٵĶ���
int Judge(const int a, const int b, const int *Queue) {
  int Result = a;
  for (size_t i = a + 1; i < b; i++) {
    if (Queue[Result] > Queue[i])
      Result = i;
  }
  return Result;
}
//�˿ͽ���
int Customer_In(int &Number) {                    //�˿��߳̽��뺯��
  std::uniform_int_distribution<> u_Number(0, 1000);
  std::uniform_int_distribution<> u_Status(Return, Collect);
  std::random_device e;                           //��ϵͳ����Ϊ������ӵ�����
  std::cout << "*******************" << std::endl;
  std::cout << "��" << Number << "λ�˿ͽ���" << std::endl;
  std::cout << "����ʱ��(ģ����)��" << u_Number(e) << std::endl;
  std::cout << "״̬��";
  switch (u_Status(e)) {
  case Return:
    std::cout << "��Ʊ" << std::endl;
    return 0;
  case Sign:
    std::cout << "ǩƱ" << std::endl;
    return 0;
  case Buy:
    std::cout << "��Ʊ" << std::endl;
    return 1;
  case Collect:
    std::cout << "ȡƱ" << std::endl;
    return 1;
  default:
    std::cout << "���״̬���ɴ���" << std::endl;
    return -1;
  }
}

void Customer(int &Number_People, int Number_Served, int *Number_Queue) {
  std::lock_guard<std::mutex> lockGuard(In);            //�Զ��� �����
  std::cout << "�߳����� ID��" << std::this_thread::get_id() << std::endl;
  if (Number_People < MAX_NUMBER_IN_SALE) {
    Number_People++;
    int Status = Customer_In(Number_Served);        //�˿ͽ���
    //In.unlock;
                                                    //�ж�ѡ�����
    int Result;
    if (Status == -1) {
      return;
    }
    else if (Status == 0) {
      Result = Judge(0, 1, Number_Queue);
      std::cout << "ѡ�����(�˹�)��" << Result << std::endl;
    }
    else {
      Result = Judge(0, 7, Number_Queue);
      std::cout << "ѡ�����(����)��" << Result << std::endl;
    }
    Wnd[Result].lock();
    Number_Queue[Result]++;         //�������
    Wnd[Result].unlock();
  }
  else {
    //In.unlock;
    std::cout << "����ȴ�" << std::endl;
  }
}

int Window(const int Number_Wnd, int &Number) {
  std::uniform_int_distribution<> u_Time(0, 250);
  std::random_device e;
  int Offset = 0;
  while (true) {
    if (Number != 0) {
      Offset = u_Time(e);                             //��ͬ�˴���ʱ�䲻ͬ
      if (Number_Wnd == 0 || Number_Wnd == 1) {       //�˹�����
        Sleep(TIME_MAN + Offset);                     //ģ�⴦��ʱ��
      }
      else {                                          //�Զ�����
        Sleep(TIME_AUTO + Offset);                    //ģ�⴦��ʱ��
      }
      std::lock_guard<std::mutex> lockGuard(Wnd[Number_Wnd]); //���� �ı��������
      Number--;
    }
    else {
      //���ڵȴ�
      Sleep(TIME_WND_SLEEP);
    }
  }
  return 0;
}



int main() {
  //��ʼ��

  int Number_People = 0;  //��ռ����
  int Number_Served = 0;  //�Ѿ����������
  int Number_Queue[MAX_WINDOW_AUTO + MAX_WINDOW_HUM] = { 0 }; //����������

  //�˿Ͳ���ʱ���ʼ�� ƫ������0~0.5s��
  int Time_Offset = 0;
  std::uniform_int_distribution<> u_Time(0, 50);
  std::random_device e;

  //�̳߳�ʼ��
  std::thread Customers[MAX_NUMBER_SERVE]; //�˿��߳�
  std::thread Wnd_Hum[MAX_WINDOW_HUM];    //�˹������̣߳������ٶ�����
  std::thread Wnd_Auto[MAX_WINDOW_AUTO];  //�Զ������̣߳������ٶȿ죩

  //��������

  //�˹������߳�
  for (size_t i = 0; i < MAX_WINDOW_HUM; i++) {
    Wnd_Hum[i] = std::thread(Window, i, std::ref(Number_Queue[i]));
  }
  //�Զ������߳�
  for (size_t i = 0; i < MAX_WINDOW_AUTO; i++) {
    Wnd_Auto[i] = std::thread(Window, i, std::ref(Number_Queue[MAX_WINDOW_HUM + i]));
  }
  //�˿��߳�
  while (Number_Served < MAX_NUMBER_SERVE) {
    Customers[Number_Served] = std::thread(Customer, std::ref(Number_People), Number_Served, Number_Queue);
    Number_Served++;
  }
  //�˿��̵߳ȴ�����
  while (Number_Served < MAX_NUMBER_SERVE) {
    if (Number_People <= MAX_NUMBER_IN_SALE) {
      Customers[Number_Served].join();
      Sleep(TIME_CUSTOMER_CREATE + u_Time(e));   //ÿ(100+Offset)ms����һ���˿�
      Number_Served++;
    }
  }
  std::cout << "�Ѿ��ﵽ���������� ���η����ֹ" << std::endl;
  system("pause");
  return 0;
}