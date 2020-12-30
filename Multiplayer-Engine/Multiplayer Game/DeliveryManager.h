#pragma once

// TODO(you): Reliability on top of UDP lab session 
// TODO(you): Reliability on top of UDP lab session

#include <list>

class DeliveryManager;
class Delivery;
class ReplicationCommand;
class ReplicationManagerServer; 

class DeliveryDelegate {

public:

	virtual void OnDeliverySuccess(DeliveryManager* deliveryManager, ReplicationManagerServer* replication = nullptr) = 0;
	virtual void OnDeliveryFailure(DeliveryManager* deliveryManager, ReplicationManagerServer* replication = nullptr) = 0;

	Delivery* delivery = nullptr; 
};

class DeliveryMustSend : public DeliveryDelegate {

public:
	DeliveryMustSend(Delivery* myDelivery) 
	{
		delivery = myDelivery;
	}

	void OnDeliverySuccess(DeliveryManager* deliveryManager, ReplicationManagerServer* replication) override;
	void OnDeliveryFailure(DeliveryManager* deliveryManager, ReplicationManagerServer* replication) override;

};

struct Delivery {

	~Delivery() {
		if (deliveryDelegate != nullptr)
		{
			delete deliveryDelegate;
			deliveryDelegate = nullptr;
		}
	}

	uint32 sequenceNumber = 0;
	double dispatchTime = 0.0;
	DeliveryDelegate* deliveryDelegate = nullptr;

	// Aditional Info to be able to do something with the failed/success deliveries

	// We save a list of commands that must be sended no matter what, 
	// so we can send them later if the packet is dropped
	std::list<ReplicationCommand> mustSendCommands; 
};

class DeliveryManager {

public:

	// For senders to write a nwe seq. number into a packet 
	Delivery* WriteSequenceNumber(OutputMemoryStream& packet);

	// For Receivers to proccess the seq. number from an incoming packet 
	bool ProcessSequenceNumber(const InputMemoryStream& packet);

	// For receivers to write acknowladged seq. numbers into a packet 
	bool hasSequenceNumbersPendingAck();
	void WriteSequenceNumbersPendingAck(OutputMemoryStream& packet);

	// For senders to process acknowladged seq. numbers from a packet 
	void ProcessAckdSequenceNumbers(const InputMemoryStream& packet);
	bool ProcessTimedOutPackets(ReplicationManagerServer* replication);

	void Clear();

private:

	// Senders
	uint32 nextOutSequenceNumber = 0;
	std::list<Delivery*> pendingDeliveries;

	// Recievers
	uint32 nextExpectedSequenceNumber = 0;
	std::list<uint32> pendingAcks;
};