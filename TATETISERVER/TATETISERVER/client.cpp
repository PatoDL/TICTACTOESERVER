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

void DrawGame(byte g[][3])
{
	cout << g[0][0] << g[0][1] << g[0][2] << endl;
	cout << g[1][0] << g[1][1] << g[1][2] << endl;
	cout << g[2][0] << g[2][1] << g[2][2] << endl;
}

message received;
message sent;

void AllowInput()
{
	cout << "Escribe el tipo de dato ( g- eleccion en el juego ) a enviar: ";
	cin >> sent.cmd;
	fflush(stdin);
	cin.ignore();

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
			
			if(registered)
			{
				cout << "Escribe el tipo de dato (1- mensaje privado / s- mostrar estado actual de partida / g- eleccion en el juego / 0- recibir data del servidor) a enviar: ";
				cin >> sent.cmd;
				fflush(stdin);
				cin.ignore();

				if (sent.cmd != '0')
				{
					if (sent.cmd == 'g')
						cout << "escriba el numero de la fila seguido del de la columna, sin caracteres de por medio (ej: si desea seleccionar la fila 2, columna 0, debe ingresar '20'" << endl;

					cout << "Escribe el mensaje a mandar: ";

					cin.getline((char*)sent.msg, 255);

					sendto(out, (char*)&sent, sizeof(message), 0, (sockaddr*)&server, sizeof(server));
				}
				else
				{
					
					int bytesIn = recvfrom(out, (char*)&received, sizeof(message), 0, (sockaddr*)&server, &serverSize);

					if (bytesIn == SOCKET_ERROR)
					{
						cerr << "error al recibir data." << endl;
						return -1;
					}
				}
			}
			else
			{
				memset(&received, 0, sizeof(received));
				int bytesIn = recvfrom(out, (char*)&received, sizeof(message), 0, (sockaddr*)&server, &serverSize);

				if (bytesIn == SOCKET_ERROR)
				{
					cerr << "error al recibir data." << endl;
					return -1;
				}
			}

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
				case 2:
					{
						cout << received.msg << endl;
					}
				case 'g':
					{
						if (received.cmd == 'g')
							cout << "received game update:" << endl;
						
						byte g[3][3];
						for (int i = 0; i < 3; i++)
						{
							for (int j = 0; j < 3; j++)
							{
								//int aux = received.msg[i + j] - '0';
								g[i][j] = received.msg[i + j];
							}
						}
						DrawGame(g);
						AllowInput();
						sendto(out, (char*)&sent, sizeof(message), 0, (sockaddr*)&server, sizeof(server));
					}
					break;
				case 'e':
					{
						cout << "an error has occurred: " << received.msg << endl;
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
