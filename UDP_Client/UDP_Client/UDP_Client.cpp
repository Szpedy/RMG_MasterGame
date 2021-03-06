#include "Client.h"
using namespace std;
//Zmienne :|
bool gameOver = false;
bool serverReady = false;

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
	cout << PS.ID << " " << PS.OP << " " << PS.ODP << " " << PS.ZAR << " " << PS.DANE << endl;
	cout << PS.ID.to_ullong() << " " << PS.OP.to_ullong() << " " << PS.ODP.to_ullong() << " " << PS.ZAR.to_ullong() << " " << PS.DANE.to_ullong() << endl;
}

int SendPacket(SOCKET s, addrinfo *SendAddr, packetStruct &PS, char packet[4]) {
	//	cout << "WYSYLAM:" << endl;
	//	printMSG(PS);
	encodeMSG(PS, packet);
	int iResult = sendto(s, packet, 4, 0, SendAddr->ai_addr, SendAddr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		throw 0;
	}
	return iResult;
}

int RecievePacket(SOCKET s, sockaddr_in &RecvAddr, packetStruct &PS, char packet[4], int RecvAddrSize) {

	int iResult = recvfrom(s, packet, 4, 0, (SOCKADDR *)& RecvAddr, &RecvAddrSize);
	if (iResult == SOCKET_ERROR) {
		throw 1;
	}
	decodeMSG(PS, packet);
	return iResult;
}

void recieve(SOCKET s, sockaddr_in RecvAddr, addrinfo *SendAddr, packetStruct PS, char packet[4]) {
	//Odbieramy datagram
	while (true) {
		RecievePacket(s, RecvAddr, PS, packet, 16);
		//Dzialamy na datagramie
		if (PS.OP == TIME && PS.ODP == OK_TIME) {
			if (PS.DANE.to_ulong() == 0) {
				cout << "Koniec Czasu !" << endl;
			}

			cout << "Czas do konca: " << PS.DANE.to_ulong() << "s" << endl;
			PS.ODP = ACK_T;
			SendPacket(s, SendAddr, PS, packet);
			continue;
		}

		if (PS.OP == GUESS && PS.ODP == ACK_G) {
			//cout << "Otrzymano potwierdzenie dostarczenia liczby"<< endl;
			continue;
		}

		if (PS.OP == RESULTS && PS.ODP == MISS) {
			cout << "Nie trafiles sproboj jeszce raz" << endl;
			PS.DANE = 0;
			PS.OP = RESULTS;
			PS.ODP = ACK_R;
			SendPacket(s, SendAddr, PS, packet);
			serverReady = true;
			continue;
		}

		if (PS.OP == RESULTS && PS.ODP == LOSS) {
			PS.DANE = 0;
			PS.OP = RESULTS;
			PS.ODP = ACK_R;
			SendPacket(s, SendAddr, PS, packet);
			cout << "Przegrales !" << endl;
			gameOver = true;
			serverReady = true;
			break;
		}

		if (PS.OP == RESULTS && PS.ODP == WIN) {
			PS.DANE = 0;
			PS.OP = RESULTS;
			PS.ODP = ACK_R;
			cout << "Wygrales !" << endl;
			gameOver = true;
			serverReady = true;
			break;
		}
	}
}

int main()
{
	int iResult;
	WSADATA wsaData;

	SOCKET SendSocket = INVALID_SOCKET;
	sockaddr_in RecvAddr;	//struktura do odbierania
	int RecvAddrSize = sizeof(RecvAddr);
	struct addrinfo *SendAddr = NULL, hints; // struktura do wysylania

	string IP;
	packetStruct PS;
	char packet[4];
	int Liczba = 0;

	//-----------------------------------------------
	// Inicjalizacja

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	// Tworze gniazdo
	SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (SendSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// Tworzenie struktury z adresami ip
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	//sprawdzam poprawnosc adresu IP
	cout << "Podaj adres ip serwera "; cin >> IP;
	iResult = getaddrinfo(IP.c_str(), "64000", &hints, &SendAddr);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	//---------------------------------------------
	// Klient gotowy do dzialania :)
	try {
		SendPacket(SendSocket, SendAddr, PS, packet);
		RecievePacket(SendSocket, RecvAddr, PS, packet, RecvAddrSize);
		RecievePacket(SendSocket, RecvAddr, PS, packet, RecvAddrSize);
		if (PS.OP == HELLO && PS.ODP == OK_ID) {
			cout << "Polaczono z serwerem!" << endl;
			cout << "Dostalem ID: " << PS.DANE.to_ullong() << endl;
			PS.ODP = ACK_H;
			PS.ID = PS.DANE.to_ullong();
			SendPacket(SendSocket, SendAddr, PS, packet);
		}
		else {
			cout << "Nie udalo sie polaczyc z serwerem";
			return 1;
		}

		cout << "Czekam na start ..." << endl;
		RecievePacket(SendSocket, RecvAddr, PS, packet, RecvAddrSize);
		if (PS.OP == START && PS.ODP == S_START) {
			PS.OP = START;
			PS.ODP = ACK_S;
			PS.DANE = 0;
			SendPacket(SendSocket, SendAddr, PS, packet);
			cout << "Zaczynamy gre !" << endl;
		}
		//---------------------------------------------
		/// START ROZGRYWKI
		// Watek dla odbiornika ze wzgledu na odbiur czasu
		thread reciever(recieve, SendSocket, RecvAddr, SendAddr, PS, packet);
		reciever.detach();
		while (!gameOver) {
			cout << "Podaj liczbe [0-255]: " << endl;
			cin >> Liczba;
			while (!cin.good() || Liczba < 0 || Liczba > 255 || Liczba < INT_MIN || Liczba  > INT_MAX) {
				cin.clear();
				cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
				cout << "Podaj liczbe z dobrego zakresu: " << flush;	cin >> Liczba;
			}

			PS.OP = GUESS;
			PS.ODP = SEND_N;
			PS.DANE = Liczba;
			if (!gameOver) {
				SendPacket(SendSocket, SendAddr, PS, packet);
				cout << "Czekam na drugiego gracza...." << endl;
			}
			while (!serverReady) {}
			serverReady = false;
		}
	}
	catch (int ERR) {
		if (ERR == 0)
			printf("sendto failed with error: %d\n", WSAGetLastError());

		if (ERR == 1)
			printf("recvfrom failed with error %d\n", WSAGetLastError());

	}
	//---------------------------------------------
	// Zamykanie gniazda 
	printf("Finished working. Closing socket.\n");
	iResult = closesocket(SendSocket);
	if (iResult == SOCKET_ERROR) {
		printf("closesocket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	printf("Exiting.\n");
	WSACleanup();
	system("PAUSE");
	return 0;
}