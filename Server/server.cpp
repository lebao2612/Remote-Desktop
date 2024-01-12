#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <winsock2.h>
#include <iostream>
#include <WS2tcpip.h>
#include <cstring>
#include <Windows.h>
#include <thread>

SOCKET serverSocket1, clientSocket1, serverSocket2, clientSocket2, serverSocket3, clientSocket3;

using namespace cv;
//using namespace std;
using std::cin;
using std::cout;
using std::endl;
using std::vector;
using std::cerr;


#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <conio.h>

struct Data {
    char key;
    int isCtrl;
    int isShift;
    int isCapsLock;
    int left, right, up, down;
};

struct mouseData {
    int mouseX;
    int mouseY;
    bool leftButtonDown;
    bool rightButtonDown;
    bool scroll;
};

void controlMouse(const mouseData& mData)
{
    SetCursorPos(mData.mouseX, mData.mouseY);
    static bool leftButtonState = false;


    if (mData.leftButtonDown)
    {
        if (!leftButtonState)
        {
            // Press and hold the left mouse button
            mouse_event(MOUSEEVENTF_LEFTDOWN, mData.mouseX, mData.mouseY, 0, 0);
            leftButtonState = true;
        }
    }
    else if (leftButtonState)
    {
        // Release the left mouse button
        mouse_event(MOUSEEVENTF_LEFTUP, mData.mouseX, mData.mouseY, 0, 0);
        leftButtonState = false;
    }

    if (mData.rightButtonDown)
    {
        //SetCursorPos(mData.mouseX, mData.mouseY);
        mouse_event(MOUSEEVENTF_RIGHTDOWN, mData.mouseX, mData.mouseY, 0, 0);
        mouse_event(MOUSEEVENTF_RIGHTUP, mData.mouseX, mData.mouseY, 0, 0);
    }
    if (mData.scroll)
    {
        //SetCursorPos(mData.mouseX, mData.mouseY);
        mouse_event(MOUSEEVENTF_WHEEL, mData.mouseX, mData.mouseY, 0, 0);
    }
}

BITMAPINFOHEADER createBitmapHeader(int width, int height)
{
    BITMAPINFOHEADER  bi;

    // create a bitmap
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;  //this is the line that makes it draw upside down or not
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    return bi;
}

Mat captureScreenMat(HWND hwnd)
{
    Mat src;

    // get handles to a device context (DC)
    HDC hwindowDC = GetDC(hwnd);
    HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

    // define scale, height and width
    int screenx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screeny = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // create mat object
    src.create(height, width, CV_8UC4);

    // create a bitmap
    HBITMAP hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
    BITMAPINFOHEADER bi = createBitmapHeader(width, height);

    // use the previously created device context with the bitmap
    SelectObject(hwindowCompatibleDC, hbwindow);

    // copy from the window device context to the bitmap device context
    StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, screenx, screeny, width, height, SRCCOPY);  //change SRCCOPY to NOTSRCCOPY for wacky colors !
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);            //copy from hwindowCompatibleDC to hbwindow

    // avoid memory leak
    DeleteObject(hbwindow);
    DeleteDC(hwindowCompatibleDC);
    ReleaseDC(hwnd, hwindowDC);

    return src;
}


void CaptureScreen() {

    int delay = 75;


    while (true) {
        // Khởi tạo cửa sổ để chụp màn hình
        HWND hwnd = GetDesktopWindow();
        Mat src = captureScreenMat(hwnd);
        vector<uchar> buf;
        vector<int> para_jpg(2);
        para_jpg[0] = cv::IMWRITE_JPEG_QUALITY;
        para_jpg[1] = 60;
        cv::imencode(".jpg", src, buf, para_jpg);


        // Gửi kích thước ảnh đến server 
        int imgSize = buf.size();
        int  bytes = send(clientSocket2, reinterpret_cast<char*>(buf.data()), imgSize, 0);
        if (bytes <= 0) {
            cout << "Failed to send image data." << endl;
            break;
        }

        Sleep(delay);
    }
}


void moveMouse() {
    mouseData mData;

    while (true) {
        //Sleep()
        int bytesRead = recv(clientSocket3, reinterpret_cast<char*>(&mData), sizeof(mData), 0);

        //Gửi dữ liệu ảnh đến server
        if (bytesRead == SOCKET_ERROR)
            cerr << "Loi truyen nhan du lieu chuot\n";
        // bytesRead lưu trữ số lượng byte đã nhận. Nếu giá trị này là SOCKET_ERROR, có thể có lỗi trong quá trình nhận dữ liệu                  
        else {
            controlMouse(mData);
        }
    }
}


void typeKeyBoard() {
    Data data;
    while (1)
    {
        int bytes = recv(clientSocket1, reinterpret_cast<char*>(&data), sizeof(data), 0);
        if (bytes <= 0) {
            cout << "Failed to receive data." << endl;
        }
        if (data.key == '\0')
            continue;
        else {

            if (data.isCtrl) {
                keybd_event(VK_CONTROL, 0, 0, 0);  // Nhấn phím Ctrl
            }

            if (data.isShift) {
                keybd_event(VK_LSHIFT, 0, 0, 0);
            }

            if (data.isCapsLock) {
                keybd_event(VK_CAPITAL, 0, 0, 0);  // Nhấn nút CapsLock
                keybd_event(VK_CAPITAL, 0, KEYEVENTF_KEYUP, 0);  // Thả nút CapsLock
            }

            keybd_event(VkKeyScan(data.key), 0, 0, 0);  // Nhấn phím tương ứng với data.ch
            keybd_event(VkKeyScan(data.key), 0, KEYEVENTF_KEYUP, 0);  // Thả phím tương ứng với data.ch

            if (data.isCtrl) {
                keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);  // Thả phím Ctrl
            }

            if (data.isShift) {
                keybd_event(VK_LSHIFT, 0, KEYEVENTF_KEYUP, 0);
            }
        }
    }
}


void timeOut() {
    char b = 0; int retcode = 0;
    do {
        Sleep(500);
        b = 'a';
        retcode = send(clientSocket1, &b, sizeof(char), 0);
    } while (retcode != SOCKET_ERROR);
}


void Connect(SOCKET& Server, SOCKET& Client, int port) {
    // Khởi tạo Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cout << "WSAStartup failed: " << result << endl;
        return;
    }

    // Tạo socket
    Server = socket(AF_INET, SOCK_STREAM, 0);
    if (Server == INVALID_SOCKET) {
        cout << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return;
    }

    // Thiết lập địa chỉ và cổng cho server
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port); // Sử dụng cổng 8888, bạn có thể thay đổi nếu cần


    // Bind socket tới địa chỉ và cổng
    if (bind(Server, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cerr << "Gan (Bind) that bai\n";
        closesocket(Server);
        //Đóng socket, giải phóng tài nguyên và tránh rò rỉ socket 
        WSACleanup();
        return;
    }

    // Chờ kết nối
    if (listen(Server, 1) == SOCKET_ERROR) {
        cout << "Socket listen failed: " << WSAGetLastError() << endl;
        closesocket(Server);
        WSACleanup();
        return;
    }

    cout << "Waiting for incoming connections..." << endl;

    // Chấp nhận kết nối
    /*SOCKET clientSocket;*/
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    Client = accept(Server, (SOCKADDR*)&clientAddr, &clientAddrSize);
    if (Client == INVALID_SOCKET) {
        cout << "Accept failed: " << WSAGetLastError() << endl;
        closesocket(Server);
        WSACleanup();
        return;
    }
    closesocket(Server);
    cout << "Client connected!" << endl;

}

int main() {

    Connect(serverSocket1, clientSocket1, 8080);

    Connect(serverSocket2, clientSocket2, 5050);

    Connect(serverSocket3, clientSocket3, 1010);

    std::thread t1(typeKeyBoard);
    std::thread t2(CaptureScreen);
    std::thread t3(moveMouse);
    // std::thread t4(timeOut);

    t1.join();
    t2.join();
    t3.join();
    // t4.join();


     // Đóng socket và giải phóng zxzx BNN %%^ 
    closesocket(clientSocket1);
    closesocket(clientSocket2);
    closesocket(clientSocket3);
    WSACleanup();
    return 0;
}