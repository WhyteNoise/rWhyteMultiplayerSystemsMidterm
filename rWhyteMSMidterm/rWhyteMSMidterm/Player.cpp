#include "Player.h"

PlayerChar::PlayerChar()
{
	userName = "Peasant";
	thisType = Peasant;
}

PlayerChar::PlayerChar(std::string name, ClassType classType, int uid)
{
	userName = name;
	thisType = classType;
	userID = uid;

	if (classType == BattleMaster)
	{
		health = 10;
		attack = 3;
		healVal = 2;
	}
	else if (classType == Monk)
	{
		health = 10;
		attack = 2;
		healVal = 5;
	}
	else if (classType == Rogue)
	{
		health = 5;
		attack = 5;
		healVal = 1;
	}
	else if (classType == Peasant)
	{
		health = 1;
		attack = 1;
		healVal = 0;
	}
}

std::string ConvertClassType(PlayerChar::ClassType value)
{
	if (value == PlayerChar::BattleMaster)
	{
		return "BattleMaster";
	}
	else if (value == PlayerChar::Monk)
	{
		return "Monk";
	}
	else if (value == PlayerChar::Rogue)
	{
		return "Rogue";
	}
	else if (value == PlayerChar::Peasant)
	{
		return "Peasant";
	}
	else
	{
		return "Invalid Class";
	}
}

void PlayerChar::Damage(int value)
{
	health = health - value;
}

std::string PlayerChar::GetName()
{
	return userName;
}

void PlayerChar::SetName(std::string value)
{
	userName = value;
}

void PlayerChar::SetAddress(RakNet::SystemAddress value)
{
	address = value;
}

RakNet::SystemAddress PlayerChar::GetAddress()
{
	return address;
}

PlayerChar::ClassType PlayerChar::GetClassType()
{
	return thisType;
}

void PlayerChar::SetClassType(PlayerChar::ClassType value)
{
	thisType = value;
}

int PlayerChar::GetPower()
{
	return attack;
}

int PlayerChar::GetHealth()
{
	return health;
}

void PlayerChar::HealSelf()
{
	health = health + healVal;
}

std::string PlayerChar::GetStats()
{
	return "User Name:" + userName + "\nClass: " + ConvertClassType(thisType) + "\nAttack: " + std::to_string(attack) + "\nCurrent Health: " + std::to_string(health) + "\nUser ID: " + std::to_string(userID) + "\n";
}