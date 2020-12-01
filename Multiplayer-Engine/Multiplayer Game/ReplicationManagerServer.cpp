#include "Networks.h"
#include "ReplicationManagerServer.h"

// TODO(you): World state replication lab session

void ReplicationManagerServer::CreateObject(uint32 networkID)
{
	replication_list.push_back(ReplicationCommand(ReplicationAction::Create_Obj, networkID));
}

void ReplicationManagerServer::UpdateObject(uint32 networkID)
{
	replication_list.push_back(ReplicationCommand(ReplicationAction::Update_Obj, networkID));
}

void ReplicationManagerServer::DestroyObject(uint32 networkID)
{
	replication_list.push_back(ReplicationCommand(ReplicationAction::Destroy_Obj, networkID));
}

void ReplicationManagerServer::WriteReplication(OutputMemoryStream& packet)
{
	for (std::vector<ReplicationCommand>::iterator iter = replication_list.begin(); iter != replication_list.end();)
	{
		// Send which object is going to be updated and what type of udpate is
		packet << (*iter).networkID;
		packet << (*iter).action;
		
		// Depending on the action, serialize different fields
		switch ((*iter).action)
		{
			case ReplicationAction::Update_Obj:
			{
				GameObject* object = App->modLinkingContext->getNetworkGameObject((*iter).networkID);

				packet << object->position.x;
				packet << object->position.y;

				packet << object->size.x;
				packet << object->size.y;

				packet << object->angle;

				LOG("Sent to update Object with ID: %i", object->networkId);
			} break;

			case ReplicationAction::Create_Obj:
			{
				GameObject* object = App->modLinkingContext->getNetworkGameObject((*iter).networkID);

				packet << object->position.x;
				packet << object->position.y;

				packet << object->size.x;
				packet << object->size.y;

				packet << object->angle;

				packet << object->type;

				LOG("Sent to create Object with ID: %i", object->networkId);

			} break;

			case ReplicationAction::Destroy_Obj:
			{
				// Nothing to do here, just telling them to delete it
				LOG("Sent to Destroy Object with ID: %i", (*iter).networkID);
			} break;

			case ReplicationAction::None:
			{
			} break;

			default:
				break;
		}

		// Delete the replication command from the vector
		// This is done assuming we didn't lose packets there....
		iter = replication_list.erase(iter);
	}
}
