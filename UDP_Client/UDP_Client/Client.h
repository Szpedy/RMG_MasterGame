// pole operacji o d³ugoœci 6 bitów, // OPERACJA : ACK, PRZESYLAM_LICZBE, PRZESYAM ID, ERROR
// pole odpowiedzi o d³ugoœci 4 bitów, // 
// pole identyfikatora o d³ugoœci 8 bitów, // przesylany ID
// 18 bitow = 3 bajty
//	     ID      OP      ODP  ZAR	   DANE
//  [00000000][000000|00][00|000000][00000000]
//	   [0]       [1]       [2]			[3]
// ID WYLOSOWANE PO POLACZENIU c-client s-server
// OPERACJE : HELLO(c,s), GUESS(c) ,TIME(s),Result(s); 
// ODP_HELLO: REQ_ID(c), OK_ID(s), SERVER_FULL(s),ACK(c,s);
// ODP_GUESS: SEND_N(c),ACK(s,c); Miss(s);
// ODP_TIME: TIME(s),ACK(c);
// ODP_Result:  Win(s), Loss(s),ACK(c);

/* HOW
1. Klient ³¹czy sie z serwerem poprzez wyslanie komunikatu HELLO
2. Serwer wysy³a potwierdzenie oraz pakiet HELLO z polem ID
3. Klient wysyla potwierdzenie i oczekuje na pakiet start
4. Serwer czeka na drugiego klienta postepuje tak samo z nim jak z pierwszym
5. Po podlaczeniu i nadaniu ID drugiemu klientowi serwer losuje liczbe i wysyla komunikat START wraz z czasem do zakonczenia;
6. Klienci potwierdzaja odbiur start i rozpoczynaja gre
7. Klienci wysylaja liczbê do odgadniecia, serwer czeka na obu klientow z odpowiedzia
8. Serwer po otrzymkaniu liczby przesyla potwierdzenie
9. Po otrzymaniu dwoch liczb serwer wysyla informacje o wyniku rozgrywki:

TRYAGAIN -> klient musi przeslac liczbe ponownie (nie zgadl)
WIN  -> klient wygral
LOSS -> klient przegral
*/


#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <bitset>
#include <thread>
#include <limits>
// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
#pragma once

enum OP { HELLO, START, GUESS, TIME, RESULTS };
enum ODP_HELLO { REQ_ID, OK_ID, SERVER_FULL, ACK_H };
enum ODP_GUESS { SEND_N, ACK_G };
enum ODP_START { S_START, ACK_S };
enum ODP_TIME { OK_TIME, ACK_T };
enum ODP_RESULTS { MISS, WIN, LOSS, ACK_R };

//struktura pakietu
struct packetStruct
{
	std::bitset<8> ID;  // identyfikator nadany przez serwer
	std::bitset<6> OP;  // 0 - HELLO, 1 - GUESS, 6 - ACK 
	std::bitset<4> ODP; // Odpowiednie dla operacji
	std::bitset<6> ZAR; // NIE UZYWANE :(
	std::bitset<8> DANE; // LICZBY LUB CZAS
};

void decodeMSG(packetStruct &PS, char *in);
void encodeMSG(packetStruct &PS, char *out);
void printMSG(const packetStruct &PS);
void recieve(SOCKET s, sockaddr_in RecvAddr, addrinfo *SendAddr, packetStruct PS, char packet[4]);


