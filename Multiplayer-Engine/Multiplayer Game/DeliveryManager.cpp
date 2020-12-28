#include "Networks.h"
#include "DeliveryManager.h"

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

	LOG("Writing delivery with sequence number: %i", delivery->sequenceNumber);
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

	LOG("Sequence number out of order. Skipping. Expected seq number: %i. Recieved seq number: %i", nextExpectedSequenceNumber, sequenceNumber);
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
		LOG("Writing ack with sequence number: %i", (*iter));
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
				LOG("Found acked sequence number in pending deliveries: %i", sequenceNumberAckd);
				// Invoke its callbacks success
				(*iter)->delegate->OnDeliverySuccess(this);

				// Remove it from the list
				iter = pendingDeliveries.erase(iter);
				continue;
			}
			++iter;
		}

	}
}

void DeliveryManager::ProcessTimedOutPackets()
{
	for (std::list<Delivery*>::iterator iter = pendingDeliveries.begin(); iter != pendingDeliveries.end(); )
	{
		if ((Time.time - (*iter)->dispatchTime) >= PACKET_DELIVERY_TIMEOUT_SECONDS)
		{
			LOG("Timed out: %i", (*iter)->sequenceNumber);
			// Call on failure 
			(*iter)->delegate->OnDeliveryFailure(this);

			// Remove it
			iter = pendingDeliveries.erase(iter);
			continue;
		}
		++iter;
	}
}
