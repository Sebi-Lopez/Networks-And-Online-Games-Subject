#include "Networks.h"
#include "DeliveryManager.h"

// TODO(you): Reliability on top of UDP lab session 
// TODO(you): Reliability on top of UDP lab session

Delivery* DeliveryManager::WriteSequenceNumber(OutputMemoryStream& packet)
{
	// Create the delivery 

	Delivery* delivery = new Delivery();

	delivery->sequenceNumber = nextOutSequenceNumber++;
	delivery->dispatchTime = Time.time;
	//delivery->delegate->OnDeliveryFailure() = this;

	// Store it in the manager list 
	pendingDeliveries.push_back(delivery);

	// Write in the packet the sequence number
	packet << delivery->sequenceNumber;

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
		//nextExpectedSequenceNumber = sequenceNumber + 1; 
		return true;
	}

	LOG("Sequence number out of order. Expected seq number: %i. Recieved seq number: %i", nextExpectedSequenceNumber, sequenceNumber);
	return false;
}

bool DeliveryManager::hasSequenceNumbersPendingAck()
{
	return false;
}

void DeliveryManager::WriteSequenceNumbersPendingAck(OutputMemoryStream& packet)
{
	// Write all the sequence numbers pending to ack
	for (std::list<uint32>::iterator iter = pendingAcks.begin(); iter != pendingAcks.end(); ++iter)
	{
		packet << (*iter);
	}

	// Clear the list
	pendingAcks.clear();
}