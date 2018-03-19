#include <iostream>
#include <string>
#include <RakNetTypes.h>
#include <RakString.h>


class PlayerChar
{
public:

	enum ClassType {BattleMaster, Monk, Rogue, Peasant, Broken};

	PlayerChar();

	PlayerChar(std::string name, ClassType classType, int uid);

	int userID = -1;
	bool alive = true;

	int healVal;
	int attack;
	int health;

	void Damage(int value);

	std::string GetName() const;

	void SetName(std::string value);

	void SetAddress(RakNet::SystemAddress value);

	RakNet::SystemAddress GetAddress();

	ClassType GetClassType();

	void SetClassType(ClassType value);

	int GetPower();

	int GetHealth();

	void HealSelf();

	std::string GetStats();

private:

	ClassType thisType;

	std::string userName;

	RakNet::SystemAddress address;	
};

