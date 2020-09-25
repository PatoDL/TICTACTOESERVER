#include <iostream>
#include <WS2tcpip.h>
#include <string>

#pragma comment (lib, "ws2_32.lib")

using namespace std;

struct message
{
	byte cmd;
	char msg[255];
};

void DrawGame(char g[][3])
{
	cout << g[0][0] << g[0][1] << g[0][2] << endl;
	cout << g[1][0] << g[1][1] << g[1][2] << endl;
	cout << g[2][0] << g[2][1] << g[2][2] << endl;
}

message received;
message sent;

void StringToCharPtr(string s, char c[])
{
	char* aux = new char[255];
	strcpy_s(aux, 255, s.c_str());
	strcpy_s(c, 255, aux);
}

void AllowInput()
{
	cout << "Escribe el tipo de dato ( g- eleccion en el juego ) a enviar: ";
	cin >> sent.cmd;
	fflush(stdin);
	cin.ignore();
	while (sent.cmd != 'g')
	{
		cout << "that option doesn't exist, please choose another one:" << endl;
		cout << "Escribe el tipo de dato ( g- eleccion en el juego ) a enviar: ";
		cin >> sent.cmd;
		cin.ignore();
	}

	if(sent.cmd== 'g')
		cout << "escriba el numero de la fila seguido del de la columna, sin caracteres de por medio (ej: si desea seleccionar la fila 2, columna 0, debe ingresar '20')" << endl;
	
	cin.getline((char*)sent.msg, 255);
}

int main()
{
	// inicializar winsock

	WORD version = MAKEWORD(2, 2);
	WSADATA data;

	if (WSAStartup(version, &data) == 0)
	{
		string ipToUse = "";
		int portToUse = 0;

		cout << "Escribe la ip a usar: ";
		cin >> ipToUse;
		cout << endl;
		cout << "Escribe el puerto a usar: ";
		cin >> portToUse;
		cout << endl;
		cin.ignore();

		// indicar ip y puerto de destino
		sockaddr_in server;
		server.sin_family = AF_INET; // AF_INET == IPV4
		server.sin_port = htons(portToUse); // puerto destino
		inet_pton(AF_INET, ipToUse.c_str(), &server.sin_addr); // direccion ip destino		

		// crear socket, UDP
		SOCKET out = socket(AF_INET, SOCK_DGRAM, 0);

		// enviar data a traves del socket
		string msgtest = "";
		char xd[1024];
		int serverSize = sizeof(server);

		message connectionMessage;
		connectionMessage.cmd = 'c';
		string mes = "";
		strcpy_s(connectionMessage.msg, sizeof(string), mes.c_str());

		sendto(out, (char*)&connectionMessage, sizeof(message), 0, (sockaddr*)&server, sizeof(server));

		bool registered = false;

		do 
		{
			memset(&sent, 0, sizeof(sent));
			memset(&received, 0, sizeof(received));
			
			int bytesIn = recvfrom(out, (char*)&received, sizeof(message), 0, (sockaddr*)&server, &serverSize);

			if (received.cmd == 'r')
			{
				registered = true;
				cout << received.msg << endl;
				sent.cmd = 'r';
				cin.getline((char*)sent.msg, 255);
				sendto(out, (char*)&sent, sizeof(message), 0, (sockaddr*)&server, sizeof(server));
			}

			if (registered)
			{
				switch (received.cmd)
				{
				case 1:
					{
						cout << received.msg << endl;
					}
					break;
				case 'g':
					{
						cout << "received game update:" << endl;
						
						char g[3][3];
						int counter = 0;
						for (int i = 0; i < 3; i++)
						{
							for (int j = 0; j < 3; j++)
							{
								g[i][j] = received.msg[i + j + counter];
							}
							counter+=2;
						}
						DrawGame(g);

						cout << "it's your turn! make your move" << endl;
						
						AllowInput();
						sendto(out, (char*)&sent, sizeof(message), 0, (sockaddr*)&server, sizeof(server));
						cout << "waiting for the other player to make its move..." << endl;
					}
					break;
				case 's':
					{
						cout << received.msg << endl;
						cout << "press 'y' to play again or 'n' to close the client" << endl;
						cin >> sent.cmd;
						if (sent.cmd == 'n')
							msgtest = "close";

						string s = "";
						StringToCharPtr(s, sent.msg);
						sendto(out, (char*)&sent, sizeof(message), 0, (sockaddr*)&server, sizeof(server));
					}
					break;
				case 'o':
					{
						cout << received.msg << endl;
					}
					break;
				case 'e':
					{
						cout << "an error has occurred: " << received.msg << endl;
					}
					break;
				case 't':
					{
						char g[3][3];
						int counter = 0;
						for (int i = 0; i < 3; i++)
						{
							for (int j = 0; j < 3; j++)
							{
								g[i][j] = received.msg[i + j + counter];
							}
							counter += 2;
						}
						DrawGame(g);
					}
					break;
				}
			}
		} 
		while (msgtest != "close");

		// cerrar el socket
		closesocket(out);
	}
	else
	{
		return -1;
	}

	fflush(stdin);

	return 0;

	// cerrar winsock
	WSACleanup();
}
