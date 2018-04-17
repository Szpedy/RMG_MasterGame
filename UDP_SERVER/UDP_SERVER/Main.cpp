#include "Server.h"
using namespace std;

//Zmienne :|
bool gameOver = false;		// Stan gry
vector<bool> synchro(2);	// zmiena sluzaca do synchronizacji graczy
int players = 0;			//Ilosc graczy
int Liczba = -1;			//Wylosowana liczba ktora gracze musza odgadnac
int winner = -1;			// ktos wygral
vector<pair<sockaddr_in, int> > Clients; // Informacje o klientach

// Inforamcja o kliencie
void Clientinfo(sockaddr_in Client) {
	char ipAddress[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &Client.sin_addr, ipAddress, INET_ADDRSTRLEN);
	cout << ipAddress;
}

//dekodowanie wiadomosci
void decodeMSG(packetStruct &PS, char *in) {
	std::string str;
	std::bitset<8> bits;

	//Dodawanie wiadomosci do stringa
	for (int i = 0; i < 4; i++) {
		bits = in[i];
		str += bits.to_string();
	}

	// Dodanie wartosci do bitsetow (zamiana z systemu dwojkowego na dziesietny)
	PS.ID = stoi(str.substr(0, 8), nullptr, 2);
	PS.OP = stoi(str.substr(8, 6), nullptr, 2);
	PS.ODP = stoi(str.substr(14, 4), nullptr, 2);
	PS.ZAR = stoi(str.substr(18, 6), nullptr, 2);
	PS.DANE = stoi(str.substr(24, 8), nullptr, 2);
}

//kodowanie wiadomosci
void encodeMSG(packetStruct &PS, char *out) {
	std::string str;

	//Dodanie wszystkich bitow do stringa
	str += PS.ID.to_string();
	str += PS.OP.to_string();
	str += PS.ODP.to_string();
	str += PS.ZAR.to_string();
	str += PS.DANE.to_string();
	
	//zapakowanie bitow w bajty do wysylki
	out[0] = stoi(str.substr(0, 8), nullptr, 2);
	out[1] = stoi(str.substr(8, 8), nullptr, 2);
	out[2] = stoi(str.substr(16, 8), nullptr, 2);
	out[3] = stoi(str.substr(24, 8), nullptr, 2);

}

//wyswietlanie pakietu
void printMSG(const packetStruct &PS) {
	cout << PS.ID << " " << PS.OP << " " << PS.ODP << " " << PS.ZAR << " " << PS.DANE<<endl;
	cout << PS.ID.to_ullong() << " " << PS.OP.to_ullong() << " " << PS.ODP.to_ullong() << " " << PS.ZAR.to_ullong() << " " << PS.DANE.to_ullong() << endl;
}

// Wysyla datagram start do graczy
void sendStart(SOCKET s, char packet[4], packetStruct PS) {
	// Jezeli dwoch graczy sie polaczylo i ma ID to wysylam start
	cout << "Mozna startowac !" << endl;
	PS.OP = START;
	PS.ODP = 0;
	PS.DANE = 0;

	PS.ID = Clients[0].second;
	SendPacket(s, Clients[0].first, PS, packet, 16);

	PS.ID = Clients[1].second;
	SendPacket(s, Clients[1].first, PS, packet, 16);
}

//Losowanie id
int randomInt() {
	random_device rd;
	std::default_random_engine generator(rd());
	std::uniform_int_distribution<int> distribution(0, 255);
	int id = distribution(generator);
	return id;
}

// Funkcja przysylajaca pakiet
int SendPacket(SOCKET s, sockaddr_in SendAddr, packetStruct &PS, char packet[4] ,int SendAddrSize) {
	encodeMSG(PS,packet);
//	cout << "WYSLALEM WIADOMOSC" << endl;
//	printMSG(PS);
	int iResult = sendto(s, packet, 4, 0, (SOCKADDR *)& SendAddr, SendAddrSize);
	if (iResult == SOCKET_ERROR) {
		throw 0;
	}
	return iResult;
}

// Wysyla czas na poczatku oraz co 10 sekund koniec czasu == koniec rozgrywki 
// Wykorzystywana przez watek do okreslania czasu
void Timer(SOCKET s, char packet[4], packetStruct PS) {
	int seconds = ((((Clients[0].second + Clients[1].second) * 99) % 100) + 30);
	int n = seconds / 10;
	cout << "Czas rozgrywki: " << seconds << "s" << endl;
	cout << Clients[0].second;
	PS.ID = Clients[0].second;
	PS.OP = 3;
	PS.ODP = 0;
	PS.ZAR = 0;
	PS.DANE = seconds;
	SendPacket(s, Clients[0].first, PS, packet, 16);

	std::this_thread::sleep_for(20ms);
	cout << Clients[1].second;
	PS.ID = Clients[1].second;
	PS.OP = 3;
	PS.ODP = 0;
	PS.ZAR = 0;
	PS.DANE = seconds;
	SendPacket(s, Clients[1].first, PS, packet, 16);

	for (int i = 0; i < n; i++) {
		std::this_thread::sleep_for(10s);
		if (gameOver == false) {
			seconds -= 10;
			cout << "Pozostalo: " << seconds << "s" << endl;

			PS.ID = Clients[0].second;
			PS.OP = 3;
			PS.ODP = 0;
			PS.ZAR = 0;
			PS.DANE = seconds;
			SendPacket(s, Clients[0].first, PS, packet, 16);

			std::this_thread::sleep_for(20ms);
			PS.ID = Clients[1].second;
			PS.OP = 3;
			PS.ODP = 0;
			PS.ZAR = 0;
			PS.DANE = seconds;
			SendPacket(s, Clients[1].first, PS, packet, 16);
		}
		else return;
	}
	if (seconds == 0 && gameOver == false) {
		gameOver = true;
		// WYSYLAMY INFORMACJE O PORAZCE
		PS.ID = Clients[0].second;
		PS.OP = RESULTS;
		PS.ODP = LOSS;
		PS.ZAR = 0;
		PS.DANE = 0;
		SendPacket(s, Clients[0].first, PS, packet, 16);

		PS.ID = Clients[1].second;
		PS.OP = RESULTS;
		PS.ODP = LOSS;
		PS.ZAR = 0;
		PS.DANE = 0;
		SendPacket(s, Clients[1].first, PS, packet, 16);

	}
	if (seconds > 0 && gameOver == false) {
		std::this_thread::sleep_for(std::chrono::seconds(seconds));
		gameOver = true;
		PS.ID = Clients[0].second;
		PS.OP = RESULTS;
		PS.ODP = LOSS;
		PS.ZAR = 0;
		PS.DANE = 0;
		SendPacket(s, Clients[0].first, PS, packet, 16);

		PS.ID = Clients[1].second;
		PS.OP = RESULTS;
		PS.ODP = LOSS;
		PS.ZAR = 0;
		PS.DANE = 0;
		SendPacket(s, Clients[1].first, PS, packet, 16);
	}
}

// Odbiur pakietu
int RecievePacket(SOCKET s, sockaddr_in &RecvAddr, packetStruct &PS,char (&packet)[4], int RecvAddrSize) {
	cout << "Odbieram dane........." << endl;
	
	int iResult = recvfrom(s, packet, 4, 0, (SOCKADDR *)& RecvAddr, &RecvAddrSize);
	if (iResult == SOCKET_ERROR) {
		throw 1;
	}
	decodeMSG(PS, packet);
	//printMSG(PS);
	return iResult;
}

// Zwraca numer klienta ktory przeslal datagram oraz przydziela ID nowym gracza
int whoSendDatagram(vector<pair<sockaddr_in, int> > &Clients , sockaddr_in Client, packetStruct &PS) {
	int i = 0;
	for ( i = 0; i < Clients.size(); i++) {
		if (PS.ID.to_ullong() == Clients[i].second) {
			//cout << "Wiadomosc od gracza: " << i << endl;
			return i;
		}
	}
	//Nowy gracz
	if (players < 2) {
		int ID = randomInt();
		if (players == 1) {
			while(ID == Clients[0].second)
				ID = randomInt();
		}
		Clients.push_back(make_pair(Client, ID));
		return i;
	}
	cout << "Serwer jest pelny wiadomosc od trzeciego gracza " << i << endl;
	return i;
}

// Metoda obslugujaca datagramy
void handlePacket(SOCKET s, sockaddr_in Client, char packet[4], packetStruct PS, int who) {

	//-----------------------------------------------
	// INICJALIZACJA
	//Client ¿¹da ID
	if (PS.OP == HELLO && PS.ODP == REQ_ID) {
		// Odsylam potwierdzenie
		PS.ODP = ACK_H;
		SendPacket(s, Client, PS, packet, 16);
		// Odsylam ID
		PS.ODP = OK_ID;
		PS.DANE = Clients.back().second;
		SendPacket(s, Client, PS, packet, 16);
		players++;
		return;
	}

	//Client potwierdza dostarczenie id
	if (PS.OP == HELLO && PS.ODP == ACK_H) {
		cout << "Nowy gracz dodany do rozgrywki jego ID " << Clients[who].second << endl;
		synchro[who] = true;
		if (synchro[0] == true && synchro[1] == true) {
			sendStart(s, packet, PS);
			synchro[0] = false;
			synchro[1] = false;
		}
		return;
	}

	//Client potwierdza otrzymania wiadomosci start
	if (PS.OP == START && PS.ODP == ACK_S) {
		cout << "Otrzymalem potwierdzenie dostarczenia startu" << endl;
		synchro[who] = true;
		if (synchro[0] == true && synchro[1] == true) {

			// Jezeli dwoch graczy dostarczylo potwierdzenie startu uruchamiamy timer oraz losujemy liczbe
			Liczba = randomInt();
			cout << "!!START!!" << endl;
			cout << "Wylosowany numer: " << Liczba << endl;
			synchro[0] = false;
			synchro[1] = false;

			//URUCHAMIAM TIMER
			thread timer(Timer, s, packet,PS);
			timer.detach();
			
		}
		return;
	}

	//-----------------------------------------------
	//Rozgrywka
	//Client przyslal numer
	if (PS.OP == GUESS && PS.ODP == SEND_N) {
		synchro[who] = true;
		//Odsylam potwierdzenie
		PS.OP = GUESS;
		PS.ODP = ACK_G;
		SendPacket(s, Client, PS, packet, 16);
		cout << "Gracz " << who << " przeslal numer: " << PS.DANE.to_ulong() << endl;

		// Sprwadzamy czy zgadl
		if (PS.DANE.to_ullong() == Liczba && !gameOver) {
			gameOver = true;
			cout << "Gracz " << who << " zgadl numer" << endl;
			Liczba = -1;
			winner = who;
		}

		// Nikt nie zgadl
		if (synchro[0] == true && synchro[1] == true && winner == -1) {
			cout << "Nikt nie zgadl" << endl;
			PS.ID = Clients[0].second;
			PS.OP = RESULTS;
			PS.ODP = MISS;
			PS.DANE = 0;
			SendPacket(s, Clients[0].first, PS, packet, 16);

			PS.ID = Clients[1].second;
			PS.OP = RESULTS;
			PS.ODP = MISS;
			PS.DANE = 0;
			SendPacket(s, Clients[1].first, PS, packet, 16);
			synchro[0] = false;
			synchro[1] = false;
		}

		//Ktos zgadl
		if (winner == 0 || winner == 1) {
			PS.ID = Clients[who].second;
			PS.OP = RESULTS;
			PS.ODP = WIN;
			PS.DANE = 0;
			SendPacket(s, Clients[who].first, PS, packet, 16);

			PS.ID = Clients[1].second;
			PS.OP = RESULTS;
			PS.ODP = LOSS;
			PS.DANE = 0;
			if (winner == 0)
				SendPacket(s, Clients[1].first, PS, packet, 16);
			else {
				PS.ID = Clients[0].second;
			SendPacket(s, Clients[0].first, PS, packet, 16);
			}
			synchro[0] = false;
			synchro[1] = false;
			return;
		}
	}

	//Client otrzymal wynik
	if (PS.OP == RESULTS && PS.ODP == ACK_R) {
		cout << "Klient " << who << " otrzymal wynik." << endl;
		if (gameOver) {
			synchro[who] = true;
			cout << "MOGE ZAKONCZYC PRACE SERWERA!" << endl;
		}
		return;
	}

	//Klient otrzymal wiadomosc o czasie
	if (PS.OP == TIME && PS.ODP == ACK_T) {
		cout << "Klient " << who << " otrzymal informacje o czasie." << endl;
	}
}

int main()
{
	int iResult = 0;
	WSADATA wsaData;

	SOCKET RecvSocket;
	unsigned short Port = 64000;
	char packet[4];
	packetStruct PS;

	//ustawienia klientow
	sockaddr_in RecvAddr;
	int ClientAddrSize = sizeof(RecvAddr);

	//-----------------------------------------------
	// Inicjalizacja
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		printf("WSAStartup failed with error %d\n", iResult);
		return 1;
	}

	// Tworze gniazdo
	RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (RecvSocket == INVALID_SOCKET) {
		printf("socket failed with error %d\n", WSAGetLastError());
		return 1;
	}

	//Przypisuje do gniazda adres ip oraz port
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(Port);
	RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	iResult = ::bind(RecvSocket, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));
	if (iResult != 0) {
		wprintf(L"bind failed with error %d\n", WSAGetLastError());
		return 1;
	}
	
	//-----------------------------------------------
	// Serwer gotowy do pracy :)
	while (!gameOver) {
		RecievePacket(RecvSocket, RecvAddr, PS, packet, ClientAddrSize);
		int Player = whoSendDatagram(Clients,RecvAddr, PS);
		if (Player < 2) {
			// Obs³uga otrzymanego datagramu
			handlePacket(RecvSocket, Clients[Player].first, packet, PS, Player);
		}
		else {
			// Nie ma miejsca 
			cout << "NIE MA MIEJSCA !" << endl;
			PS.ODP = ACK_H;
			SendPacket(RecvSocket, RecvAddr, PS, packet, ClientAddrSize);
			PS.ODP = SERVER_FULL;
			SendPacket(RecvSocket, RecvAddr, PS, packet, ClientAddrSize);
		}
	}

	while (synchro[0] == false && synchro[1] == false) {
		RecievePacket(RecvSocket, RecvAddr, PS, packet, ClientAddrSize);
		int Player = whoSendDatagram(Clients, RecvAddr, PS);
		handlePacket(RecvSocket, Clients[Player].first, packet, PS, Player);
	}

	//-----------------------------------------------
	// Zamykanie gniazda
	printf("Finished working. Closing socket.\n");
	iResult = closesocket(RecvSocket);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"closesocket failed with error %d\n", WSAGetLastError());
		return 1;
	}
	printf("Exiting.\n");
	WSACleanup();
	system("PAUSE");
	return 0;
}

