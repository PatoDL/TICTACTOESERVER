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
	int wantsToRestart = -1;
};

class GameState
{
public:
	char state[3][3] = { { '0','0','0' }, { '0','0','0' }, { '0','0','0' } };
};

class Game
{
public:
	int turn = 0;
	Player* p[2];
	GameState gs;
};

char CheckAll(GameState n)
{
	for (int i = 0; i < 3; i++)
	{
		if (n.state[i][0] == n.state[i][1] && n.state[i][0] == n.state[i][2] && n.state[i][0] != '0')
		{
			return n.state[i][0];
		}
	}	

	for (int i = 0; i < 3; i++)
	{
		if (n.state[0][i] == n.state[1][i] && n.state[0][i] == n.state[2][i] && n.state[0][i] != '0')
		{
			return n.state[0][i];
		}
	}
	//checking the win for both diagonal

	if (n.state[0][0] == n.state[1][1] && n.state[0][0] == n.state[2][2] && n.state[0][0] != '0')
	{
		return n.state[0][0];
	}
	else if (n.state[0][2] == n.state[1][1] && n.state[0][2] == n.state[2][0] && n.state[0][2] != '0')
	{
		return n.state[0][2];
	}

	return '0';
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

int Turn(Game* g, Player p, string position)
{
	int num = stoi(position);

	int row = num / 10;
	int col = num % 10;

	byte turn;
	
	if (p.num == 0)
		turn = 'O';
	else
		turn = 'X';
	
	if(g->gs.state[row][col] == '0')
		g->gs.state[row][col] = turn;

	char status = CheckAll(g->gs);

	if (status == 'X')
		return 1;
	else if (status == 'O')
		return 0;

	return -1;
}

char* ArrayToString(char g[][3])
{
	char* toReturn = new char[9];/* = g[0][0] + g[0][1] + g[0][2] + g[1][0] + g[1][1] + g[1][2] + g[2][0] + g[2][1] + g[2][2];*/
	
	char* aux = toReturn;

	for(int i=0;i<3;i++)
	{
		for(int j=0;j<3;j++)
		{
			*toReturn = g[i][j];
			toReturn++;
		}
	}

	*toReturn = '\0';

	toReturn = aux;

	return toReturn;
}

void RestartGame(Game* g)
{
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			g->gs.state[i][j] = '0';
		}
	}
}

void DrawGame(int g[][3])
{
	cout << g[0][0] << g[0][1] << g[0][2] << endl;
	cout << g[1][0] << g[1][1] << g[1][2] << endl;
	cout << g[2][0] << g[2][1] << g[2][2] << endl;
}

void StringToCharPtr(string s, char c[])
{
	char* aux = new char[255];
	strcpy_s(aux, 255, s.c_str());
	strcpy_s(c,255, aux);
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
				g->p[player]->wantsToRestart = -1;
				cout << "player " << player << " added to game " << game << endl;

				if(player == 1)
				{
					g->p[0]->enemy = g->p[player];
					g->p[player]->enemy = g->p[0];
				}
				
				sent.cmd = 'r';
				string s = "Choose an alias with which you will be recognized in game: ";
				strcpy_s(sent.msg, sizeof(sent.msg), s.c_str());
				sendto(listening, (char*)&sent, sizeof(message), 0, (sockaddr*)&(client), sizeof(client));
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

					message m;
					m.cmd = 1;
					string s;
					char* aux = new char[255];

					if (p->enemy != nullptr)
					{
						s = "You are Playing Against ";

						char c[255] = "You are Playing Against ";
						
						string s2 = ", it's your turn!";

						Player* auxPlayer = games[p->gameItBelongsTo]->p[0];
						
						strcat_s(c, 255, auxPlayer->enemy->alias.c_str());

						strcat_s(c, 255, s2.c_str());
						
						StringToCharPtr(c, m.msg);
			
						sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&auxPlayer->client, sizeof(auxPlayer->client));

						m.cmd = 'g';

						s = ArrayToString(games[auxPlayer->gameItBelongsTo]->gs.state);

						

						StringToCharPtr(s, m.msg);
						cout << m.msg << endl;
						sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&auxPlayer->client, sizeof(auxPlayer->client));

						m.cmd = 1;

						char c2[255] = "You are Playing Against ";
						
						s2 = ", it's your opponent's turn!, please wait...";

						strcat_s(c2, 255, auxPlayer->alias.c_str());

						strcat_s(c2, 255, s2.c_str());
						
						StringToCharPtr(c2, m.msg);

						auxPlayer = auxPlayer->enemy;
						sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&auxPlayer->client, sizeof(auxPlayer->client));

						m.cmd = 't';

						s = ArrayToString(games[auxPlayer->gameItBelongsTo]->gs.state);

						StringToCharPtr(s, m.msg);

						sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&auxPlayer->client, sizeof(auxPlayer->client));
					}
					else
					{
						m.cmd = 'o';
						s = "You don't have an opponent yet, please wait!";
						StringToCharPtr(s, m.msg);
						sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&p->client, sizeof(p->client));
					}
				}
			}
			break;
		case 'd':
			{
			if (received.msg[0] == 'y')
				{
					bool found = false;
					Player* p = SearchPlayer(client.sin_port, found);
					message m;
					RestartGame(games[p->gameItBelongsTo]);
					m.cmd = 1;
					string s;
					bool restarting = false;
					p->wantsToRestart = 1;
					if (p->enemy != nullptr)
					{
						if (p->enemy->wantsToRestart == -1)
							s = "waiting for your enemy";
						else if (p->enemy->wantsToRestart == 1)
						{
							s = "your enemy also wants to restart";
							restarting = true;
						}
					}
					else
						s = "your enemy doesn't want to play again, searching for another game";

					StringToCharPtr(s, m.msg);

					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&p->enemy->client, sizeof(p->enemy->client));

					if (restarting)
					{
						m.cmd = 1;
						char c[255] = "you are playing vs: ";
						strcat_s(c, p->alias.c_str());
						s = " again";
						strcat_s(c, s.c_str());
						StringToCharPtr(c, m.msg);
						sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&p->enemy->client, sizeof(p->enemy->client));
						m.cmd = 'g';
						s = ArrayToString(games[p->gameItBelongsTo]->gs.state);
						StringToCharPtr(s, m.msg);
						sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&p->enemy->client, sizeof(p->enemy->client));

						m.cmd = 1;
						char c2[255] = "you are playing vs: ";
						strcat_s(c2, p->enemy->alias.c_str());
						s = " again";
						strcat_s(c2, s.c_str());
						StringToCharPtr(c2, m.msg);
						sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&p->client, sizeof(p->client));
						m.cmd = 't';
						s = ArrayToString(games[p->gameItBelongsTo]->gs.state);
						StringToCharPtr(s, m.msg);
						sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&p->client, sizeof(p->client));

						p->wantsToRestart = -1;
						p->enemy->wantsToRestart = -1;
					}
				}
				else if(received.msg[0] == 'n')
				{
					bool found = false;
					Player* p = SearchPlayer(client.sin_port, found);
					message m;
					m.cmd = '1';
					string s = "your opponent has chosen to not play again, wait for another player";
					StringToCharPtr(s, m.msg);
					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&p->enemy->client, sizeof(p->enemy->client));
					RestartGame(games[p->gameItBelongsTo]);
					p->wantsToRestart = 0;
					games[p->gameItBelongsTo]->p[p->num] = nullptr;
					p->enemy->enemy = nullptr;
					delete p;
				}
			}
			break;
		case 'g':
			{
				bool found = false;
				Player* p = SearchPlayer(client.sin_port, found);
				message m;
				
				char* aux = new char[255];
				string s;

				Player* toSend = p->enemy;

				m.cmd = 'g';
				
				int winner = Turn(games[p->gameItBelongsTo], *p, received.msg);
				
				s = ArrayToString(games[p->gameItBelongsTo]->gs.state);

				

				if (winner != -1)
				{
					m.cmd = 't';

					StringToCharPtr(s, m.msg);
					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&(p->client), sizeof(p->client));
					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&(toSend->client), sizeof(toSend->client));

					m.cmd = 's';
					s = "You Won, do you want to play again?";
					StringToCharPtr(s, m.msg);

					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&(p->client), sizeof(p->client));

					s = "You Lose, do you want to play again?";
					StringToCharPtr(s, m.msg);
					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&(toSend->client), sizeof(toSend->client));
				}
				else
				{
					StringToCharPtr(s, m.msg);
					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&(toSend->client), sizeof(toSend->client));

					m.cmd = 't';
					sendto(listening, (char*)&m, sizeof(message), 0, (sockaddr*)&(p->client), sizeof(p->client));
				}
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