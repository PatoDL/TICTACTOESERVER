#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <vector>

#pragma comment (lib, "ws2_32.lib")

using namespace std;

struct message
{
	byte cmd;
	char msg[255];
};

class Player
{
public:
	string alias = "";
	int gameItBelongsTo = -1;
	int port = 0;
	int num = -1;
	Player* enemy;
	sockaddr_in client;
};

class GameState
{
public:
	byte state[3][3] = { 0 };
};

class Game
{
public:
	int turn = 0;
	Player* p[2];
	GameState gs;
};

int CheckAll(GameState n)
{
	for (int i = 0; i < 3; i++)
		if (n.state[i][0] == n.state[i][1] && n.state[i][0] == n.state[i][2] || n.state[0][i] == n.state[1][i] && n.state[0][i] == n.state[2][i])
			return n.state[i][0];

	//checking the win for both diagonal

	if (n.state[0][0] == n.state[1][1] && n.state[0][0] == n.state[2][2])
	{
		return n.state[0][0];
	}
	else if (n.state[0][2] == n.state[1][1] && n.state[0][2] == n.state[2][0])
	{
		return n.state[0][2];
	}

	return 0;
}

vector<Game*> games;

Player* SearchPlayer(int port, bool &found)
{
	for (int i = 0; i < games.size(); i++)
	{
		for (int j = 0; j < 2; j++)
		{
			if (port == games[i]->p[j]->port)
			{
				found = true;
				return games[i]->p[j];
			}
		}
	}

	return nullptr;
}

Game* SearchAvailableGame(int& game, int& player)
{
	for(int i=0;i<games.size();i++)
	{
		for(int j=0;j<2;j++)
		{
			if(games[i]->p[j] == nullptr)
			{
				game = i;
				player = j;
				return games[i];
			}
		}
	}

	Game* g = new Game();
	game = 0;
	player = 0;
	games.push_back(g);

	return g;
}

bool Turn(Game* g, Player p, string position)
{
	int num = stoi(position);

	int row = num / 10;
	int col = num % 10;

	byte turn;
	
	if (p.num = 0)
		turn = 'O';
	else
		turn = 'X';
	
	g->gs.state[row][col] = turn;

	return true;
}

string ArrayToString(byte g[][3])
{
	string toReturn;/* = g[0][0] + g[0][1] + g[0][2] + g[1][0] + g[1][1] + g[1][2] + g[2][0] + g[2][1] + g[2][2];*/
	
	for(int i=0;i<3;i++)
	{
		for(int j=0;j<3;j++)
		{
			toReturn += std::to_string(g[i][j]);
		}
	}

	return toReturn;
}

void DrawGame(int g[][3])
{
	cout << g[0][0] << g[0][1] << g[0][2] << endl;
	cout << g[1][0] << g[1][1] << g[1][2] << endl;
	cout << g[2][0] << g[2][1] << g[2][2] << endl;
}

int main()
{
	//iniciar winsock
	WSADATA data;
	WORD ver = MAKEWORD(2, 2);
	int wsOK = WSAStartup(ver, &data);
	if (wsOK != 0)
	{
		cerr << "No pudo iniciar winsock" << endl;
		return -1;
	}

	//Crear un listening socket (socket de escucha)
	SOCKET listening = socket(AF_INET, SOCK_DGRAM, 0);

	if (listening == INVALID_SOCKET)
	{
		cerr << "invalid socket" << endl;
		return -1;
	}

	//bind el socket (atar el socket a una dupla ip:puerto / direccion del socket)
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(8900); // puerto en el que vamos a escuchar
	hint.sin_addr.S_un.S_addr = ADDR_ANY;
	//inet_pton(AF_INET, "127.0.0.1", &hint.sin_addr); // ip de loopback

	int bindResult = bind(listening, (sockaddr*)&hint, sizeof(hint));
	if (bindResult == SOCKET_ERROR)
	{
		cerr << "no pudo hacer el bind" << endl;
		return -1;
	}

	//recibir data del socket, y procesarla

	char buf[1024];
	message received;
	message sent;
	//estructura con la data del cliente que nos esta enviando mensajes
	sockaddr_in client;
	int clientSize = sizeof(client);

	//ZeroMemory(buf, sizeof(buf));
	memset(buf, 0, sizeof(buf));

	// funcion bloqueante
	bool closeWindow = false;
	while (!closeWindow)
	{
		memset(&received, 0, sizeof(received));
		memset(&sent, 0, sizeof(sent));

		int bytesIn = recvfrom(listening, (char*)&received, sizeof(message), 0, (sockaddr*)&client, &clientSize);

		if (bytesIn == SOCKET_ERROR)
		{
			cerr << "error al recibir data." << endl;
			return -1;
		}

		cout << "Receiving data of type: " << received.cmd << endl;
		
		switch (received.cmd)
		{
		case 'c':
			{
				int game = -1;
				int player = -1;
				Game* g = SearchAvailableGame(game, player);

				g->p[player] = new Player();
				
				g->p[player]->gameItBelongsTo = game;
				g->p[player]->num = player;
				g->p[player]->port = client.sin_port;
				g->p[player]->client = client;

				cout << "player " << player << " added to game " << game << endl;

				if(player == 1)
				{
					g->p[0]->enemy = g->p[player];
					g->p[player]->enemy = g->p[0];

					message m;
					m.cmd = 'o';

					string s;
					char* aux = new char[255];

					s = "it's your opponent's turn, please wait...";

					strcpy_s(aux, 255, s.c_str());
					strcpy_s(m.msg, aux);
					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&(g->p[player]->client), sizeof(g->p[player]->client));

					m.cmd = 1;
					
					s = "an opponent was found! it's your turn!";

					strcpy_s(aux, 255, s.c_str());
					strcpy_s(m.msg, aux);
					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&(g->p[0]->client), sizeof(g->p[0]->client));

					m.cmd = 'g';

					s = ArrayToString(g->gs.state);
					strcpy_s(aux, 255, s.c_str());
					strcpy_s(m.msg, aux);

					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&(g->p[0]->client), sizeof(g->p[0]->client));
				}
				
				sent.cmd = 'r';
				string s = "Choose an alias with which you will be recognized in game: ";
				strcpy_s(sent.msg, sizeof(sent.msg), s.c_str());
				sendto(listening, (char*)&sent, sizeof(message), 0, (sockaddr*)&(client), sizeof(client));
			}
			break;
		case 's':
			{
				bool found = false;
				Player* p = SearchPlayer(client.sin_port, found);
				if(found)
				{
					message m;
					m.cmd = 's';
					char* aux = new char[255];
					//DrawGame(games[p->gameItBelongsTo]->gs.state);
					string s = ArrayToString(games[p->gameItBelongsTo]->gs.state);
					strcpy_s(aux, 255, s.c_str());
					strcpy_s(m.msg, aux);
					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&(p->client), sizeof(p->client));
				}
			}
			break;
		case 'r':
			{
				bool found = false;
				Player* p = SearchPlayer(client.sin_port, found);
				if(found)
				{
					p->alias = received.msg;
					cout << "Player "<< p->num <<" from game "<< p->gameItBelongsTo <<" registered with name: " << p->alias.c_str() << endl;

					
				}
			}
			break;
		case 1:
			/*{
			bool sent = false;
				for (int i = 0; i < games.size(); i++)
				{
					for (int j = 0; j < 2; j++)
					{
						if (client.sin_port == games[i].p[j].port)
						{
							message mes;
							mes.cmd = 1;
							

							string start = "Message from: ";

							char* aux = new char[255];
							strcpy_s(aux, 255, start.c_str());
							strcat_s(aux, 255, games[i].p[j]->alias.c_str());

							strcat_s(aux, 255, " : ");

							strcat_s(aux, 255, received.msg);

							strcpy_s(mes.msg, aux);

							if (games[i].p[j].enemy)
							{
								sendto(listening, (char*)&mes, sizeof(message), 0, (sockaddr*)&(games[i].p[j]->enemy->client), sizeof(games[i].p[j]->enemy->client));
							}
							sent = true;
							break;
						}
					}
					if (sent)
						break;
				}
			}*/
			break;
		case 'g':
			{
				bool found = false;
				Player* p = SearchPlayer(client.sin_port, found);
				message m;
				
				char* aux = new char[255];
				string s;

				Player* toSend;
				
				if (p->enemy != nullptr)
				{
					m.cmd = 'g';
					Turn(games[p->gameItBelongsTo], *p, received.msg);
					
					s = ArrayToString(games[p->gameItBelongsTo]->gs.state);
					
					cout << "g: " << m.msg<<endl;

					toSend = p->enemy;
				}
				else
				{
					m.cmd = 'e';
					s = "cant't make a move because you don't have an opponent yet, please wait";
					toSend = p;
				}
				strcpy_s(aux, 255, s.c_str());
				strcpy_s(m.msg, aux);

				
				
				sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&(toSend->client), sizeof(p->enemy->client));
			}
			
			break;
		}
		
	}

	//Destruir el socket
	closesocket(listening);

	//cleanup winsock
	WSACleanup();
	return 0;
}