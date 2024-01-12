
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib,"Ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <WS2tcpip.h>
#include <cstring>
#include <conio.h>
#include <Windows.h>
#include <tchar.h>
#include <thread>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <string>

HHOOK keyboardHook;

SOCKET serverSocket1, serverSocket2, serverSocket3, clientSocket1, clientSocket2, clientSocket3;

struct Data {
    char ch;
    int isCtrl;
    int isShift;
    int isCapsLock;
};
using namespace cv;
//using namespace std;
using std::cin;
using std::cout;
using std::endl;
using std::vector;
using std::cerr;
using std::string;
std::mutex dataMutex;
std::vector<char> revbufKey(6000000);
std::vector<char> revbufImage(6000000);
Mat receivedImage;

struct mouseData {
    int mouseX;
    int mouseY;
    bool leftButtonDown;
    bool rightButtonDown;
    bool scroll;
};

void Connection(SOCKET& server, SOCKET& client, int port,char ip[])
{
    // Khởi tạo Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "Khoi tao WinSock that bai\n";
        return;
    }
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        cout << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return;
    }

    // Thiết lập địa chỉ server để kết nối tới client
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); // Thay đổi số cổng theo cấu hình server
    serverAddr.sin_addr.s_addr = inet_addr(ip); //

    if (connect(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Connection failed: " << WSAGetLastError() << endl;
        closesocket(server);
        WSACleanup();
        return;
    }
    else {
        cout << "Connection successful!" << endl;
    }
}



LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kbdStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        bool pressCap = false;
        Data data;
        
    

        if (kbdStruct->vkCode == VK_LEFT || kbdStruct->vkCode == VK_RIGHT ||
            kbdStruct->vkCode == VK_UP || kbdStruct->vkCode == VK_DOWN) {
            data.ch = '\0'; // Không có ký tự nào được gõ
            data.isCtrl = (GetAsyncKeyState(VK_CONTROL)) != 0;
            data.isShift = (GetAsyncKeyState(VK_SHIFT)) != 0;
            data.isCapsLock = (GetAsyncKeyState(VK_CAPITAL) & 0x0001) != 0;
            // Gửi sự kiện nhấn phím mũi tên đến máy chủ
            send(serverSocket1, (char*)(&data), sizeof(data), 0);
        }


        WCHAR unicodeChar;
        BYTE keyboardState[256];
        GetKeyboardState(keyboardState);

        if (ToUnicodeEx(kbdStruct->vkCode, kbdStruct->scanCode, keyboardState, &unicodeChar, 1, 0, NULL) == 1) {
            //data.ch = char(unicodeChar);
            data.isCtrl = (GetAsyncKeyState(VK_CONTROL)) != 0;
            data.isShift = (GetAsyncKeyState(VK_SHIFT)) != 0;
            data.isCapsLock = (GetAsyncKeyState(VK_CAPITAL) & 0x0001) != 0;
            if (data.isCapsLock == 1 && ((GetAsyncKeyState(VK_CAPITAL) & 0x8000) != 0))
                data.isCapsLock = 0;
            else if (data.isCapsLock == 0 && ((GetAsyncKeyState(VK_CAPITAL) & 0x8000) != 0)) {
                data.isCapsLock = 1;
            }
        }

        if ((GetAsyncKeyState(VK_CAPITAL) & 0x8000) == 0)
            data.ch = char(unicodeChar);
        else
            data.ch = '\0';

        send(serverSocket1, (char*)(&data), sizeof(data), 0);
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}
void Key()
{
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        Sleep(100);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(keyboardHook);
}

void ReceiveImage()
{
    // tạo một cửa sổ có thể hiển thị hình ảnh
    namedWindow("Received Image", WINDOW_NORMAL);

    // thiết lập cửa sổ che toàn bộ màn hình
    setWindowProperty("Received Image", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);

    while (1)
    {
        waitKey(1);
        int bytes = recv(serverSocket2, revbufImage.data(), 512000, 0);  // Sử dụng revbufImage cho dữ liệu hình ảnh
        if (bytes <= 0) {
            cout << "failed to receive image data." << endl;
            closesocket(serverSocket2);
            WSACleanup();
            return;
        }
        else {
            Mat receivedImage = imdecode(Mat(1, bytes, CV_8UC1, (void*)revbufImage.data()), IMREAD_COLOR);
            if (receivedImage.empty()) {
                cout << "Failed to decode received image." << endl;
                //continue;
            }
            else {
                // lấy kích thước màn hình
                int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                int screenHeight = GetSystemMetrics(SM_CYSCREEN);

                // thay đổi kích thước hình ảnh để phù hợp với màn hình
                resize(receivedImage, receivedImage, Size(screenWidth, screenHeight));

                // hiển thị hình ảnh trên cửa sổ
                imshow("Received Image", receivedImage);
            }
        }
    }
}

void Mouse()
{
    POINT mouse;
    while (1)
    {
        Sleep(100);
        GetCursorPos(&mouse);
        mouseData mData;
        mData.leftButtonDown = ((GetAsyncKeyState(VK_LBUTTON) & 0x8001) != 0);
        mData.rightButtonDown = ((GetAsyncKeyState(VK_RBUTTON) & 0x8001) != 0);
        mData.scroll = ((GetAsyncKeyState(VK_MBUTTON) & 0x8001) != 0);
        mData.mouseX = mouse.x;
        mData.mouseY = mouse.y;
        send(serverSocket3, reinterpret_cast<char*>(&mData), sizeof(mData), 0);
    }
}
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

char* getIP() {
    static char ip[16]; // Mảng char để lưu địa chỉ IP (tối đa 15 ký tự + ký tự kết thúc chuỗi '\0')

    while (true)
    {
        Mat frame(300, 500, CV_8UC3, Scalar(201, 195, 110));
        rectangle(frame, Point(50, 120), Point(450, 160), Scalar(255, 255, 255), FILLED);
        putText(frame, "Nhap IP: ", Point(50, 100), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
        putText(frame, ip, Point(50, 150), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0), 2); // Màu đen
        imshow("Remote Desktop", frame);

        int key = waitKey(0);

        if (key == 13) // Enter
        {
            break;
        }
        else if (key == 8) // Backspace
        {
            int len = strlen(ip);
            if (len > 0)
            {
                ip[len - 1] = '\0'; // Xóa ký tự cuối cùng của chuỗi
            }
        }
        else if ((key >= 48 && key <= 57) || key == 46)
        {
            int len = strlen(ip);
            if (len < 15) // Chỉ cho phép tối đa 15 ký tự IP
            {
                ip[len] = static_cast<char>(key);
                ip[len + 1] = '\0'; // Kết thúc chuỗi
            }
        }
    }
    destroyWindow("Remote Desktop");
   

    
    return ip;
}

int main() {
    char* IP = getIP();

    Connection(serverSocket1, clientSocket1, 8080,IP);
    Connection(serverSocket2, clientSocket2, 5050,IP);
    Connection(serverSocket3, clientSocket3, 1010,IP);


    std::thread t1(Key);
    std::thread t2(ReceiveImage);
    std::thread t3(Mouse);
    t1.join();
    t2.join();
    t3.join();

    // Đóng socket và giải phóng bộ nhớ
    closesocket(serverSocket1);
    closesocket(serverSocket2);
    closesocket(serverSocket3);
    WSACleanup();
    return 0;
}