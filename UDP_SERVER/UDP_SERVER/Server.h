#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <bitset>
#include <random>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
#pragma once

//struktura pakietu
struct packetStruct
{
	std::bitset<8> ID;  // identyfikator nadany przez serwer
	std::bitset<6> OP;  // 0 - HELLO, 1 - GUESS, 6 - ACK 
	std::bitset<4> ODP; // Odpowiednie dla operacji
	std::bitset<6> ZAR; // NIE UZYWANE :(
	std::bitset<8> DANE; // LICZBY LUB CZAS
};

enum OP { HELLO, START, GUESS, TIME, RESULTS };
enum ODP_HELLO { REQ_ID, OK_ID, SERVER_FULL, ACK_H };
enum ODP_GUESS { SEND_N, ACK_G };
enum ODP_START { S_START, ACK_S };
enum ODP_TIME { OK_TIME, ACK_T };
enum ODP_RESULTS { MISS, WIN, LOSS, ACK_R };

void Clientinfo(sockaddr_in Client);
void decodeMSG(packetStruct &PS, char *in);
void encodeMSG(packetStruct &PS, char *out);
void printMSG(const packetStruct &PS);
void handlePacket(SOCKET s, sockaddr_in Client, char packet[4], packetStruct PS , int who);
void Timer(SOCKET s, char packet[4], packetStruct PS);

int randomInt();
int SendPacket(SOCKET s, sockaddr_in SendAddr, packetStruct &PS, char packet[4], int SendAddrSize);
int RecievePacket(SOCKET s, sockaddr_in &RecvAddr, packetStruct &PS, char(&packet)[4], int RecvAddrSize);
int whoSendDatagram( std::vector<std::pair<sockaddr_in, int> > &Clients, sockaddr_in Client, packetStruct &PS);

