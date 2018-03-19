#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "BitStream.h"
#include <iostream>
#include <thread>         // std::thread
#include <chrono>
#include <map>
#include "Player.h"
#include <vector>
#include <algorithm>

static int SERVER_PORT = 65000;
static int CLIENT_PORT = 65001;
const static int MAX_CONNECTIONS = 4;

RakNet::RakPeerInterface* g_rakPeerInterface = nullptr;

bool isServer = false;
bool isRunning = true;
int numPlayers = 0;

enum
{
	ID_THEGAME_LOBBY = ID_USER_PACKET_ENUM,
	ID_THEGAME_ACTION,
	ID_ADD_USER,
	ID_EXISTING,
	ID_CREATED,
	ID_CHECK_NAME,
	ID_IS_AVAILABLE,
	ID_READY,
	ID_START,
	ID_STAT,
	ID_ALL_STAT,
	ID_HEAL,
	ID_ATTACK,
	ID_GET_ID,
	ID_MESSAGE,
	ID_QUIT,
};

RakNet::SystemAddress g_serverAddress;
RakNet::SystemAddress g_clientAddress;

std::vector<PlayerChar> playerChars; // So we can push new players to the end, and loop through all players

// Lobby Variables

bool joined = false;
bool availName = false;
bool playersSuccessfullyCreated[MAX_CONNECTIONS];
int numReady = 0;

// Game Variables 

bool playing = false;		//
bool nameExists = false;	// We will possibly come back to these if they aren't used
int IDs = -1;				//

int currentID = -1;

enum NetworkStates
{
	NS_Decision = 0,
	NS_CreateSocket,
	NS_PendingConnection,
	NS_Connected,
	NS_Running,
	NS_Joined,
	NS_Lobby,
	NS_Game,
};

NetworkStates g_networkState = NS_Decision;

void OnIncomingConnection(RakNet::Packet* packet)
{
	if (!isServer)
	{
		assert(0);
	}
	numPlayers++;
	unsigned short numConnections = g_rakPeerInterface->NumberOfConnections();
	std::cout << "Total player(s): " << numPlayers << "\nNum connection(s): " << numConnections << std::endl;
	RakNet::BitStream bitStream;
	bitStream.Write(RakNet::MessageID(ID_GET_ID));
	int id(numPlayers - 1);
	bitStream.Write(id);
	g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
}

void OnConnectionAccepted(RakNet::Packet* packet)
{
	if (isServer)
	{
		assert(0); // Server doesn't request connections
	}

	g_networkState = NS_Lobby;
	g_serverAddress = packet->systemAddress;
	g_clientAddress = g_rakPeerInterface->GetExternalID(packet->systemAddress);
}

PlayerChar::ClassType ToClass(std::string& value)
{
	if (value.compare("BATTLEMASTER") == 0 || value.compare("BattleMaster") == 0 || value.compare("battleMaster") == 0 || value.compare("battlemaster") == 0 || value.compare("Battle Master") == 0 || value.compare("battle master") == 0)
	{
		return PlayerChar::BattleMaster;
	}
	else if (value.compare("Monk") == 0 || value.compare("monk") == 0 || value.compare("MONK") == 0)
	{
		return PlayerChar::Monk;
	}
	else if (value.compare("Rogue") == 0 || value.compare("rogue") == 0 || value.compare("ROGUE") == 0)
	{
		return PlayerChar::Rogue;
	}
	else if (value.compare("Peasant") == 0 || value.compare("peasant") == 0 || value.compare("PEASANT") == 0)
	{
		return PlayerChar::Peasant;
	}
	else
	{
		return PlayerChar::Broken;
	}
}

std::string ToString(PlayerChar::ClassType& value)
{
	switch (value)
	{
	case PlayerChar::BattleMaster:
		return "Battle Master";
		break;
	case PlayerChar::Monk:
		return "Monk";
		break;
	case PlayerChar::Rogue:
		return "Rogue";
		break;
	case PlayerChar::Peasant:
		return "Peasant";
		break;
	default:
		break;

	}
}

void InputHandler()
{
	while (isRunning)
	{
		std::string input;
		if (g_networkState = NS_Decision)
		{
			std::cout << "Press (s) for server, (c) for client" << std::endl;
			std::cin >> input;
			isServer = input.at(0) == 's';
			g_networkState = NS_CreateSocket;
		}
		else if (g_networkState = NS_Lobby)
		{
			if (!joined)
			{
				std::cout << "To continue, enter a name." << std::endl;
				std::cout << "To quit, type quit." << std::endl;
				joined = true;
			}

			while (true)
			{
				std::cin >> input;
				if (!nameExists)
				{
					if (input.compare("quit") == 0 || input.compare("Quit") == 0 || input.compare("QUIT") == 0)
					{
						assert(0);
					}
					RakNet::BitStream bitStream;
					bitStream.Write((RakNet::MessageID)ID_CHECK_NAME);
					RakNet::SystemAddress sysAddress(g_clientAddress);
					bitStream.Write(sysAddress);
					RakNet::RakString uInput(input.data());
					bitStream.Write(uInput);
					g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false);
				}
				if (nameExists)
				{
					break;
				}
			}

			RakNet::BitStream bitStream;
			bitStream.Write((RakNet::MessageID)ID_ADD_USER);
			RakNet::SystemAddress sysAddress(g_clientAddress);
			bitStream.Write(sysAddress);
			RakNet::RakString uInput(input.data());
			bitStream.Write(uInput);

			std::cout << "Please Choose A Class: \nBattle Master \nMonk \nRogue \nPeasant" << std::endl;
			std::string classType;
			PlayerChar::ClassType chosenClass;

			while (true)
			{
				std::cin >> classType;
				chosenClass = ToClass(classType);
				if (chosenClass == PlayerChar::BattleMaster || chosenClass == PlayerChar::Monk || chosenClass == PlayerChar::Rogue || chosenClass == PlayerChar::Peasant)
				{
					break;
				}
				else
				{
					std::cout << "That was not a valid Class.  Please Choose Again." << std::endl;
				}
			}

			bitStream.Write(chosenClass);
			bitStream.Write(int(IDs));
			g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false);
			g_networkState = NS_Game;
			joined = false;
		}
		else if (g_networkState = NS_Game)
		{
			std::string input;

			if (!joined)
			{
				std::cout << "Welcome to the Lobby! \nGame will begin once everyone is ready. \nType start to get ready. \nType quit at any time to quit." << std::endl;
				joined = true;


				while (true)
				{
					std::cin >> input;

					if (input.compare("ready") == 0 || input.compare("Ready") == 0 || input.compare("READY") == 0)
					{
						std::cout << "You are Ready" << std::endl;
						break;
					}
					else if (input.compare("quit") == 0 || input.compare("Quit") == 0 || input.compare("QUIT") == 0)
					{
						std::cout << "Goodbye." << std::endl;
						assert(0);
					}
				}

				RakNet::BitStream bitStream;
				bitStream.Write(RakNet::MessageID(ID_READY));
				g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false);
			}

			if (playing)
			{
					std::cin >> input;

					if (input.substr(0,6).compare("attack") == 0 || input.substr(0, 6).compare("Attack") == 0 || input.substr(0, 6).compare("ATTACK") == 0)
					{
						std::string attackedPlayer = input.substr(input.find_first_of(' ') + 1, input.length());

						std::cout << "You Swung At: " << attackedPlayer << std::endl;
						RakNet::BitStream bitStream;
						bitStream.Write(RakNet::MessageID(ID_ATTACK));
						int thisPlayer(int(IDs));
						bitStream.Write(thisPlayer);
						RakNet::RakString playerToAttack(attackedPlayer.data());
						bitStream.Write(playerToAttack);
						g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false);
					}
					else if (input.compare("heal") == 0 || input.compare("Heal") == 0 || input.compare("HEAL") == 0)
					{
						RakNet::BitStream bitStream;
						bitStream.Write(RakNet::MessageID(ID_HEAL));
						bitStream.Write(int(IDs));
						g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false);
					}
					else if (input.compare("stats") == 0 || input.compare("Stats") == 0 || input.compare("STATS") == 0)
					{
						RakNet::BitStream bitStream;
						bitStream.Write(RakNet::MessageID(ID_STAT));
						bitStream.Write(int(IDs));
						bitStream.Write(RakNet::SystemAddress(g_clientAddress));
						g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false);
					}
					else if (input.compare("all stats") == 0 || input.compare("All Stats") == 0 || input.compare("ALL STATS") == 0)
					{
						RakNet::BitStream bitStream;
						bitStream.Write(RakNet::MessageID(ID_ALL_STAT));			
						bitStream.Write(RakNet::SystemAddress(g_clientAddress));
						g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false);
					}
					else if (input.compare("help") == 0 || input.compare("Help") == 0 || input.compare("HELP") == 0)
					{
						std::cout << "Here are your Commands: \nattack PlayerName: This attacks the character whose name you use. \nheal: Heals you for your heal value. \nstats: Gets your current stats \nall stats: Gets everyone's current stats \nhelp: Shows all available commands \nquit: Quits the game." << std::endl;
					}
					else if (input.compare("quit") == 0 || input.compare("Quit") == 0 || input.compare("QUIT") == 0)
					{
						RakNet::BitStream bitStream;
						bitStream.Write(RakNet::MessageID(ID_QUIT));
						g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false);
					}
			}
		}
	}
}

unsigned char GetPacketIdentifier(RakNet::Packet* packet)
{
	if (packet == nullptr)
		return 255;

	if ((unsigned char)packet->data[0] == ID_TIMESTAMP)
	{
		RakAssert(packet->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
		return (unsigned char)packet->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
	}
	return (unsigned char)packet->data[0];
}

bool HandleLowLevelPacket(RakNet::Packet* packet)
{
	bool isHandled = true;
	unsigned char packetIdentifier = GetPacketIdentifier(packet);
	switch (packetIdentifier)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		// Connection lost normally
		printf("ID_DISCONNECTION_NOTIFICATION\n");
		break;
	case ID_ALREADY_CONNECTED:
		// Connection lost normally
		printf("ID_ALREADY_CONNECTED with guid %" PRINTF_64_BIT_MODIFIER "u\n", packet->guid);
		break;
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
		break;
	case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
		break;
	case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_CONNECTION_LOST\n");
		break;
	case ID_NEW_INCOMING_CONNECTION:
	case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
		OnIncomingConnection(packet);
		break;
	case ID_CONNECTION_BANNED: // Banned from this server
		printf("We are banned from this server.\n");
		break;
	case ID_CONNECTION_ATTEMPT_FAILED:
		printf("Connection attempt failed\n");
		break;
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		// Sorry, the server is full.  I don't do anything here but
		// A real app should tell the user
		printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
		break;

	case ID_INVALID_PASSWORD:
		printf("ID_INVALID_PASSWORD\n");
		break;

	case ID_CONNECTION_LOST:
		// Couldn't deliver a reliable packet - i.e. the other system was abnormally
		// terminated
		printf("ID_CONNECTION_LOST\n");
		break;

	case ID_CONNECTION_REQUEST_ACCEPTED:
		// This tells the client they have connected
		printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", packet->systemAddress.ToString(true), packet->guid.ToString());
		printf("My external address is %s\n", g_rakPeerInterface->GetExternalID(packet->systemAddress).ToString(true));
		OnConnectionAccepted(packet);
		break;
	case ID_CONNECTED_PING:
	case ID_UNCONNECTED_PING:
		printf("Ping from %s\n", packet->systemAddress.ToString(true));
		break;
	default:
		isHandled = false;
		break;
	}
	return isHandled;
}

void PacketHandler()
{
	while (isRunning)
	{
		for (RakNet::Packet* packet = g_rakPeerInterface->Receive(); packet != nullptr; g_rakPeerInterface->DeallocatePacket(packet), packet = g_rakPeerInterface->Receive())
		{
			if (!HandleLowLevelPacket(packet))
			{
				unsigned char packetID = GetPacketIdentifier(packet);
				switch (packetID)
				{
					case ID_THEGAME_LOBBY: // Done
					{
						RakNet::BitStream myStream(packet->data, packet->length, false);
						RakNet::MessageID messageID;
						myStream.Read(messageID);
						std::string userName;
						myStream.Read(userName);
						std::cout << userName << " Joined the Fight! " << std::endl;
					}
						break;
					case ID_ADD_USER: // Done
					{
						RakNet::BitStream myStream(packet->data, packet->length, false);
						RakNet::MessageID messageID;
						myStream.Read(messageID);
						RakNet::SystemAddress sysAddress;
						myStream.Read(sysAddress);
						std::string test;
						myStream.Read(test);
						PlayerChar::ClassType playerClass;
						myStream.Read(playerClass);
						int ID;
						myStream.Read(ID);
						PlayerChar holder = PlayerChar(test, playerClass, ID);
						std::cout << "New player: " << holder.GetName() << "\nType is: " << ToString(playerClass) << "\nNumber of Players: " << playerChars.size() << std::endl;
					}
						break;
					case ID_EXISTING: // Done
					{
						std::cout << "Name Already Exists; Please Try a New Name\n ";
					}
						break;
					case ID_CREATED: // Done
					{
						printf("Created player.\n");
						playersSuccessfullyCreated[g_rakPeerInterface->GetIndexFromSystemAddress(g_clientAddress)] = true;
						std::cout << "Type ready to begin game." << std::endl;
					}
						break;
					case ID_CHECK_NAME: // Done
					{
						RakNet::BitStream myBitStream(packet->data, packet->length, false);
						RakNet::MessageID messageID;
						myBitStream.Read(messageID);
						RakNet::SystemAddress sysAddress;
						myBitStream.Read(sysAddress);
						std::string test;
						myBitStream.Read(test);

						RakNet::BitStream bitStream;

						for each (PlayerChar player in playerChars)
						{
							if (player.GetName() == test)
							{								
								bitStream.Write((RakNet::MessageID)ID_EXISTING);
							}
						}

						bitStream.Write((RakNet::MessageID)ID_IS_AVAILABLE);
						bitStream.Write(bool(true));
						g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, sysAddress, false);
					}
						break;
					case ID_IS_AVAILABLE: // Done
					{
						std::cout << "User Name Okay" << std::endl;
						availName = true;
					}
						break;
					case ID_READY: // Done
					{
						++numReady;
						std::cout << "Number of players: " << numReady << " number of  connections: " << g_rakPeerInterface->NumberOfConnections() << std::endl;
						if (numReady == g_rakPeerInterface->NumberOfConnections())
						{
							++currentID;
							RakNet::BitStream bs;
							bs.Write(RakNet::MessageID(ID_START));
							std::string name = "The game has begun! it is " + playerChars[currentID].GetName() + "'s turn!\n";
							RakNet::RakString str(name.data());
							bs.Write(str);
							for each (PlayerChar player in playerChars)
							{
								g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player.GetAddress(), false);
							}

						}
						else
						{
							RakNet::BitStream bs;
							bs.Write(RakNet::MessageID(ID_MESSAGE));
							int numCon = g_rakPeerInterface->NumberOfConnections();
							std::string name = std::to_string(numReady) + "/" + std::to_string(numCon) + " players ready.";
							RakNet::RakString str(name.data());
							bs.Write(str);
							for each (PlayerChar player in playerChars)
							{
								g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player.GetAddress(), false);
							}
						}
					}
						break;
					case ID_START: // Done
					{
						RakNet::BitStream myBitStream(packet->data, packet->length, false);
						RakNet::MessageID m_ID;
						myBitStream.Read(m_ID);
						std::cout << playerChars[m_ID].userID;
						playing = true;
					}
						break;
					case ID_STAT: // Done
					{

					}
						break;
					case ID_ALL_STAT: // Done
					{
						RakNet::BitStream myStream(packet->data, packet->length, false);
						RakNet::MessageID m_ID;
						myStream.Read(m_ID);
						RakNet::SystemAddress address;
						myStream.Read(address);
						RakNet::BitStream bitStream;
						bitStream.Write(RakNet::MessageID(ID_MESSAGE));

						std::cout << "----CURRENT PLAYER STATS----\n";
						for each (PlayerChar player in playerChars)
						{
							std::cout << player.GetStats();
						}
						std::cout << "----------------------------";

						g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, address, false);
					}
						break;
					case ID_MESSAGE: // Done
					{
						RakNet::BitStream myStream(packet->data, packet->length, false);
						RakNet::MessageID messageID;
						myStream.Read(messageID);
						std::string test;
						myStream.Read(test);
						std::cout << test << std::endl;
					}
						break;
					case ID_HEAL: // Done
					{
						RakNet::BitStream myStream(packet->data, packet->length, false);
						RakNet::MessageID messageID;
						myStream.Read(messageID);
						int playerID;
						myStream.Read(playerID);

						if (playerID == currentID)
						{
							playerChars[currentID].HealSelf();
						}
						else
						{
							std::cout << "It is not your turn!" << std::endl;
						}
					}
						break;
					case ID_ATTACK: // Done
					{
						RakNet::BitStream myStream(packet->data, packet->length, false);
						RakNet::MessageID messageID;
						myStream.Read(messageID);
						int attackingID;
						myStream.Read(attackingID);
						std::string attackedName;
						myStream.Read(attackedName);

						if (attackingID == currentID && playerChars[attackingID].GetName() != attackedName)
						{
							for each (PlayerChar player in playerChars)
							{
								if (player.GetName() == attackedName) 
								{
									player.Damage(playerChars[attackingID].attack);

									if (player.GetHealth() <= 0)
									{
										RakNet::BitStream bitStream;
										bitStream.Write(RakNet::MessageID(ID_MESSAGE));
										bitStream.Write(std::cout << player.GetName() << " has Died! \nThey were killed by: " << playerChars[attackingID].GetName());
										g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

										player.alive = false;

										g_rakPeerInterface->CloseConnection(player.GetAddress(), true, 0);
										
										auto it = find_if(playerChars.begin(), playerChars.end(), [&attackedName](const PlayerChar &obj) { return obj.GetName() == std::string(attackedName); });

										if (it == playerChars.begin())
										{
											playerChars.erase(playerChars.begin());
										}
										else
										{
											playerChars.erase(it--);
										}

										int counter = -1;
										for each (PlayerChar player in playerChars)
										{
											counter = counter++;
											RakNet::BitStream bitStream;
											bitStream.Write(RakNet::MessageID(ID_GET_ID));
											bitStream.Write(int(counter));
											playerChars[counter].userID = counter;
											g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player.GetAddress(), false);
										}
										if (playerChars.size() > 1)
										{
											currentID = (currentID + 1) % g_rakPeerInterface->NumberOfConnections();
											RakNet::BitStream bitStream;
											bitStream.Write(RakNet::MessageID(ID_MESSAGE));
											bitStream.Write(std::cout << "It is now " << playerChars[currentID].GetName()<< "'s turn!");
											g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

										}
										else
										{
											RakNet::BitStream bitStream;
											bitStream.Write(RakNet::MessageID(ID_MESSAGE));
											bitStream.Write(RakNet::RakString("Battle Over! \nYou Reign Supreme!"));
											g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
											g_rakPeerInterface->CloseConnection(playerChars[0].GetAddress(), true, 0);
										}
									}
									else
									{
										RakNet::BitStream bitStream;
										bitStream.Write(RakNet::MessageID(ID_MESSAGE));
										bitStream.Write(std::cout << player.GetName() << " was assaulted by " << playerChars[attackingID].GetName() << "\nTheir Health Remaining is: " << player.GetHealth());
										g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
										currentID = (currentID + 1) % g_rakPeerInterface->NumberOfConnections();									
										bitStream.Write(RakNet::MessageID(ID_MESSAGE));
										bitStream.Write(std::cout << "It is now " << playerChars[currentID].GetName() << "'s turn!");
										g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
									}
								} 
								// Attack Was Successful
								else
								{
									RakNet::BitStream bitStream;
									bitStream.Write(RakNet::MessageID(ID_MESSAGE));
									bitStream.Write(std::cout << "You Swing At Nothing. \nTry again." << std::endl);
									g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
								}
								// Attack was at non-existent player
							}
						}
						else if (attackingID == currentID && playerChars[attackingID].GetName() == attackedName)
						{
							RakNet::BitStream bitStream;
							bitStream.Write(RakNet::MessageID(ID_MESSAGE));
							bitStream.Write(std::cout << "You try to end your fight early. \nIt does not work. \nTry again." << std::endl);
							g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
						} // Attack yourself fails
						else
						{
							RakNet::BitStream bitStream;
							bitStream.Write(RakNet::MessageID(ID_MESSAGE));
							bitStream.Write(std::cout << "It is not your turn. \nDexterity is nothing to joke about." << std::endl);
							g_rakPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
						} // Attack out of order
					}
						break;
					case ID_GET_ID: // Done
					{
						RakNet::BitStream bitStream(packet->data, packet->length, false);
						RakNet::MessageID messageID;
						bitStream.Read(messageID);
						int ID;
						bitStream.Read(ID);
						IDs = ID;
					}
						break;
					case ID_QUIT: // Done
					{
						RakNet::BitStream thisBitStream(packet->data, packet->length, false);
						g_rakPeerInterface->CloseConnection(packet->systemAddress, true, 0);					
					}
					break;
					default:
						break;
				}
			}
		}
	}
}

int main()
{
	g_rakPeerInterface = RakNet::RakPeerInterface::GetInstance();

	std::thread inputHandler(InputHandler);
	std::thread packetHandler(PacketHandler);

	PlayerChar players[MAX_CONNECTIONS];

	while (isRunning)
	{
		if (g_networkState == NS_CreateSocket)
		{
			if (isServer)
			{
				RakNet::SocketDescriptor socketDescriptors[1];
				socketDescriptors[0].port = SERVER_PORT;
				socketDescriptors[0].socketFamily = AF_INET; 
				bool isSuccess = g_rakPeerInterface->Startup(MAX_CONNECTIONS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
				assert(isSuccess);
				g_rakPeerInterface->SetMaximumIncomingConnections(MAX_CONNECTIONS);
				g_networkState = NS_PendingConnection;
				std::cout << "Server waiting on connections.." << std::endl;
			}
			else
			{
				RakNet::SocketDescriptor socketDescriptor(CLIENT_PORT, nullptr);
				socketDescriptor.socketFamily = AF_INET;

				while (RakNet::IRNS2_Berkley::IsPortInUse(socketDescriptor.port, socketDescriptor.hostAddress, socketDescriptor.socketFamily, SOCK_DGRAM) == true)
					socketDescriptor.port++;

				g_rakPeerInterface->Startup(8, &socketDescriptor, 1);

				RakNet::ConnectionAttemptResult car = g_rakPeerInterface->Connect("192.168.236.1", SERVER_PORT, nullptr, 0);
				RakAssert(car == RakNet::CONNECTION_ATTEMPT_STARTED);
				std::cout << "client attempted connection..waiting for response" << std::endl;
				g_networkState = NS_PendingConnection;
			}
		}
	}

	inputHandler.join();
	packetHandler.join();
	return 0;
}