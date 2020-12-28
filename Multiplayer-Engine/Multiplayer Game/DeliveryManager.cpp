#include "Networks.h"
#include "DeliveryManager.h"

// TODO(you): Reliability on top of UDP lab session 

Delivery* DeliveryManager::WriteSequenceNumber(OutputMemoryStream& packet)
{
	// Create the delivery 

	Delivery* delivery = new Delivery();

	delivery->sequenceNumber = nextOutSequenceNumber++;
	delivery->dispatchTime = Time.time;

	// Store it in the manager list 
	pendingDeliveries.push_back(delivery);

	// Write in the packet the sequence number
	packet << delivery->sequenceNumber;

	//LOG("Writing delivery with sequence number: %i", delivery->sequenceNumber);
	return delivery;
}

bool DeliveryManager::ProcessSequenceNumber(const InputMemoryStream& packet)
{
	uint32 sequenceNumber;
	packet >> sequenceNumber;

	if (sequenceNumber >= nextExpectedSequenceNumber)
	{
		// push it to the list of pending acks
		pendingAcks.push_back(sequenceNumber);

		// change the next expected seq number
		nextExpectedSequenceNumber = sequenceNumber + 1; 
		return true;
	}

	WLOG("Sequence number out of order. Skipping. Expected seq number: %i. Recieved seq number: %i", nextExpectedSequenceNumber, sequenceNumber);
	return false;
}

bool DeliveryManager::hasSequenceNumbersPendingAck()
{
	return false;
}

void DeliveryManager::WriteSequenceNumbersPendingAck(OutputMemoryStream& packet)
{
	// Writing how many acks to deserialize 
	packet << (uint32)pendingAcks.size();

	// Write all the sequence numbers pending to ack
	for (std::list<uint32>::iterator iter = pendingAcks.begin(); iter != pendingAcks.end(); ++iter)
	{
		packet << (*iter);
		//LOG("Writing ack with sequence number: %i", (*iter));
	}

	// Clear the list
	pendingAcks.clear();
}

void DeliveryManager::ProcessAckdSequenceNumbers(const InputMemoryStream& packet)
{
	uint32 size; 
	packet >> size; 

	for (uint32 i = 0; i < size; ++i)
	{
		uint32 sequenceNumberAckd = 0; 
		packet >> sequenceNumberAckd;

		// Find the delivery in the pendingDeliveries list
		for (std::list<Delivery*>::iterator iter = pendingDeliveries.begin(); iter != pendingDeliveries.end();)
		{
			if ((*iter)->sequenceNumber == sequenceNumberAckd)
			{
				//LOG("Found acked sequence number in pending deliveries: %i", sequenceNumberAckd);
				// Invoke its callbacks success
				if((*iter)->deliveryDelegate != nullptr)
					(*iter)->deliveryDelegate->OnDeliverySuccess(this);

				// Remove it from the list
				delete (*iter);
				iter = pendingDeliveries.erase(iter);
				continue;
			}
			++iter;
		}

	}
}

bool DeliveryManager::ProcessTimedOutPackets(ReplicationManagerServer* replication)
{
	bool ret = false; 

	for (std::list<Delivery*>::iterator iter = pendingDeliveries.begin(); iter != pendingDeliveries.end(); )
	{
		if ((Time.time - (*iter)->dispatchTime) >= PACKET_DELIVERY_TIMEOUT_SECONDS)
		{
			// Call on failure 
			if ((*iter)->deliveryDelegate != nullptr)
			{
				WLOG("Timed out a package with a delivery delegate!");
				(*iter)->deliveryDelegate->OnDeliveryFailure(this, replication);
				ret = true; 
			}

			// Remove it
			delete (*iter);
			iter = pendingDeliveries.erase(iter);
			continue;
		}
		++iter;
	}
	return ret; 
}

void DeliveryMustSend::OnDeliverySuccess(DeliveryManager* deliveryManager, ReplicationManagerServer* replication)
{
	// Nothing i guess
}

void DeliveryMustSend::OnDeliveryFailure(DeliveryManager* deliveryManager, ReplicationManagerServer* replication)
{
	WLOG("Sending an IMPORTANT failed package again!");

	// Add the list of required sends in the delivery to the replication manager, it will send them later
	// https://stackoverflow.com/questions/1449703/how-to-append-a-listt-object-to-another
	replication->mustReSendList.splice(replication->mustReSendList.end(), delivery->mustSendCommands);

}
