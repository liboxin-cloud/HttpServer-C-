#include "ThreadPool.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_MAX_SIZE 1024
using namespace std;

// 处理 http 请求
void HandleRequest(SOCKET clientSocket)
{
    auto buffer = std::make_unique<char[]>(BUFFER_MAX_SIZE);
    int byteReceived = recv(clientSocket, buffer.get(), BUFFER_MAX_SIZE - 1, 0);
    if (byteReceived == SOCKET_ERROR)
    {
        cout << GetLastError() << endl;
        closesocket(clientSocket);
        return;
    }
    if (byteReceived > 0)
    {
        buffer[byteReceived] = '\0';
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Hello, World!</h1></body></html>";
        if (send(clientSocket, response.c_str(), response.length(), 0) == SOCKET_ERROR)
        {
            cout << GetLastError() << endl;
        }
    }
    closesocket(clientSocket);
}

int main()
{
	//打开网络权限
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		cout << "WSAData Startup Failed!" << GetLastError() << endl;
		return -1;
	}
	//创建监听套接字
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET)
    {
        cout << GetLastError() << endl;
        return -1;
    }
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;

    if (::bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cout << "Bind Failed" << GetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return -1;

    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        cout << "Listen Failed " << GetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return -1;

    }
    cout << "Server listening on port 8080..." << endl;

    ThreadPool threadPool(thread::hardware_concurrency());
    while (true)
    {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET)
        {
            cout << "Accpet Failed:" << WSAGetLastError() << endl;
            continue;
        }
        //加入线程池
        threadPool.enqueue(
            [clientSocket]
            {
                HandleRequest(clientSocket);

            });
    }
    closesocket(listenSocket);
    WSACleanup();
    while (true);
	return 0;
}